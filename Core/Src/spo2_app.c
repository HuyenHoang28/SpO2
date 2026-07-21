#include "spo2_app.h"
#include "spo2_algorithm.h"
#include "finger_detector.h"

#include <math.h>
#include <string.h>

#define SENSOR_RETRY_INTERVAL_MS       3000U
#define SENSOR_IO_ERROR_LIMIT           8U
#define SENSOR_MISSING_CONFIRM_LIMIT    3U
#define SENSOR_STALL_TIMEOUT_MS         1500U
#define SENSOR_RECONFIGURE_INTERVAL_MS  1000U
#define RTC_READ_INTERVAL_MS            1000U
#define RTC_RETRY_INTERVAL_MS           15000U
#define RTC_STARTUP_DELAY_MS            5000U
#define MAX_SAMPLES_PER_TICK            8U
#define SENSOR_INIT_ATTEMPTS            3U
#define SENSOR_LED_CURRENT_DEFAULT       0x3CU
#define SENSOR_LED_CURRENT_MIN           0x20U
#define SENSOR_LED_CURRENT_MAX           0x7FU
#define SENSOR_GAIN_ADJUST_INTERVAL_MS   2000U
#define SENSOR_SIGNAL_TOO_LOW            120U
#define SENSOR_SIGNAL_SATURATED          245000U
#define SENSOR_DEBUG_INTERVAL_MS            250U
#define SENSOR_FORCE_START_INTERVAL_MS      1000U

static MAX30102_Handle sensor;
static TinyRTC_Handle rtc;
static SpO2AppSnapshot current;
static I2C_HandleTypeDef *shared_i2c;
static TinyRTC_Kind configured_rtc_kind;

static uint32_t ir_ring[SPO2_APP_BUFFER_LENGTH];
static uint32_t red_ring[SPO2_APP_BUFFER_LENGTH];
static uint32_t ir_linear[SPO2_APP_BUFFER_LENGTH];
static uint32_t red_linear[SPO2_APP_BUFFER_LENGTH];
static uint16_t ring_write_index;
static uint16_t ring_count;
static uint16_t new_samples_since_calculation;
static FingerDetector finger_detector;
static uint8_t sensor_led_current;
static uint32_t last_gain_adjust_tick;
static uint32_t last_sensor_retry_tick;
static uint32_t last_sensor_data_tick;
static uint32_t last_sensor_reconfigure_tick;
static uint32_t last_sensor_debug_tick;
static uint32_t last_force_start_tick;
static uint8_t stream_restart_attempts;
static uint8_t consecutive_sensor_io_errors;
static uint8_t consecutive_sensor_missing_checks;
static uint32_t last_rtc_read_tick;
static float average_heart_rate;
static float waveform_dc;
static float waveform_amplitude;
static TinyRTC_DateTime software_date_time;
static uint32_t software_clock_tick;
static uint32_t app_start_tick;

/* V16 keeps optical contact from two independent sources:
 * - detector_contact: fast DC/span detector used for UI responsiveness
 * - algorithm_contact: latched when the PPG algorithm finds a valid result
 *
 * The previous version gated all sample buffering on detector_contact. If the
 * detector missed a particular breakout board's signal range, the algorithm
 * never received a full window and BPM/SpO2 could never become valid.
 */
static bool detector_contact;
static bool algorithm_contact;
static uint8_t invalid_algorithm_windows;

static void reset_finger_detector(void);

static uint8_t month_from_text(const char *month)
{
    static const char names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
    uint8_t i;

    for (i = 0U; i < 12U; ++i) {
        const char *candidate = &names[i * 3U];
        if ((month[0] == candidate[0]) &&
            (month[1] == candidate[1]) &&
            (month[2] == candidate[2])) {
            return (uint8_t)(i + 1U);
        }
    }
    return 1U;
}

static uint8_t weekday_from_date(uint16_t year,
                                 uint8_t month,
                                 uint8_t day)
{
    /* Sakamoto algorithm. Return 1=Monday ... 7=Sunday. */
    static const uint8_t offsets[12] = {
        0U, 3U, 2U, 5U, 0U, 3U, 5U, 1U, 4U, 6U, 2U, 4U
    };
    uint16_t y = year;
    uint8_t sunday_based;

    if (month < 3U) {
        --y;
    }
    sunday_based = (uint8_t)((y + y / 4U - y / 100U + y / 400U +
                              offsets[month - 1U] + day) % 7U);
    return (sunday_based == 0U) ? 7U : sunday_based;
}

static TinyRTC_DateTime build_date_time(void)
{
    const char *date = __DATE__; /* "Mmm dd yyyy" */
    const char *time = __TIME__; /* "hh:mm:ss" */
    TinyRTC_DateTime dt;

    dt.month = month_from_text(date);
    dt.day = (uint8_t)(((date[4] == ' ') ? 0 : (date[4] - '0')) * 10 +
                       (date[5] - '0'));
    dt.year = (uint16_t)((date[7] - '0') * 1000 +
                         (date[8] - '0') * 100 +
                         (date[9] - '0') * 10 +
                         (date[10] - '0'));
    dt.hour = (uint8_t)((time[0] - '0') * 10 + (time[1] - '0'));
    dt.minute = (uint8_t)((time[3] - '0') * 10 + (time[4] - '0'));
    dt.second = (uint8_t)((time[6] - '0') * 10 + (time[7] - '0'));
    dt.weekday = weekday_from_date(dt.year, dt.month, dt.day);
    return dt;
}

static int compare_date_time(const TinyRTC_DateTime *a,
                             const TinyRTC_DateTime *b)
{
#define CMP_FIELD(field) do { \
    if (a->field < b->field) return -1; \
    if (a->field > b->field) return 1; \
} while (0)
    CMP_FIELD(year);
    CMP_FIELD(month);
    CMP_FIELD(day);
    CMP_FIELD(hour);
    CMP_FIELD(minute);
    CMP_FIELD(second);
#undef CMP_FIELD
    return 0;
}

static bool is_leap_year(uint16_t year)
{
    return ((year % 4U) == 0U) &&
           (((year % 100U) != 0U) || ((year % 400U) == 0U));
}

static uint8_t days_in_month(uint16_t year, uint8_t month)
{
    static const uint8_t days[12] = {
        31U, 28U, 31U, 30U, 31U, 30U,
        31U, 31U, 30U, 31U, 30U, 31U
    };

    if ((month == 2U) && is_leap_year(year)) {
        return 29U;
    }
    return days[month - 1U];
}

static void increment_one_second(TinyRTC_DateTime *dt)
{
    if (++dt->second < 60U) return;
    dt->second = 0U;
    if (++dt->minute < 60U) return;
    dt->minute = 0U;
    if (++dt->hour < 24U) return;
    dt->hour = 0U;
    dt->weekday = (dt->weekday >= 7U) ? 1U : (uint8_t)(dt->weekday + 1U);
    if (++dt->day <= days_in_month(dt->year, dt->month)) return;
    dt->day = 1U;
    if (++dt->month <= 12U) return;
    dt->month = 1U;
    ++dt->year;
}

static void update_software_clock(void)
{
    uint32_t now = HAL_GetTick();

    while ((now - software_clock_tick) >= 1000U) {
        software_clock_tick += 1000U;
        increment_one_second(&software_date_time);
    }
    current.date_time = software_date_time;
    current.rtc_time_valid = true;
}

static void increment_sequence(void)
{
    ++current.sequence;
    if (current.sequence == 0U) {
        current.sequence = 1U;
    }
}

static void reset_measurement(void)
{
    ring_write_index = 0U;
    ring_count = 0U;
    new_samples_since_calculation = 0U;
    average_heart_rate = 0.0f;
    current.heart_rate_bpm = 0;
    current.spo2_percent = 0;
    current.signal_quality = 0U;
    current.heart_rate_valid = false;
    current.spo2_valid = false;
    current.measurement_valid = false;
    current.buffered_samples = 0U;
    current.low_spo2 = false;
    current.abnormal_heart_rate = false;
    waveform_dc = 0.0f;
    waveform_amplitude = 1000.0f;
}

static void update_sensor_diagnostic(uint32_t now)
{
    MAX30102_Diagnostic diagnostic;
    if ((now - last_sensor_debug_tick) < SENSOR_DEBUG_INTERVAL_MS) {
        return;
    }
    last_sensor_debug_tick = now;
    if (MAX30102_ReadDiagnostic(&sensor, &diagnostic) == MAX30102_OK) {
        /* Reuse existing diagnostic fields so the TouchGFX data contract does
           not need another generated-code change. */
        current.finger_threshold =
            ((uint32_t)diagnostic.mode_config << 24U) |
            ((uint32_t)diagnostic.fifo_config << 16U) |
            ((uint32_t)diagnostic.spo2_config << 8U) |
            (uint32_t)diagnostic.led1_current;
        current.ir_span =
            ((uint32_t)diagnostic.fifo_write_pointer << 16U) |
            ((uint32_t)diagnostic.fifo_overflow_counter << 8U) |
            (uint32_t)diagnostic.fifo_read_pointer;
        increment_sequence();
    }
}

static void try_initialize_sensor(void)
{
    MAX30102_Status status;
    uint8_t attempt;

    if (shared_i2c == NULL) {
        return;
    }

    status = MAX30102_ERROR_NOT_FOUND;
    for (attempt = 0U; attempt < SENSOR_INIT_ATTEMPTS; ++attempt) {
        status = MAX30102_InitDefault(&sensor, shared_i2c);
        if (status == MAX30102_OK) {
            break;
        }

        /* Recover from a peripheral state-machine error without disturbing
           TouchGFX. The pin-level 9-clock recovery is performed once in
           main.c before SpO2App_Init(). */
        (void)HAL_I2C_DeInit(shared_i2c);
        HAL_Delay(2U);
        (void)HAL_I2C_Init(shared_i2c);
        HAL_Delay(10U);
    }

    current.sensor_error = status;
    current.sensor_ok = (status == MAX30102_OK);
    reset_finger_detector();
    consecutive_sensor_io_errors = 0U;
    consecutive_sensor_missing_checks = 0U;
    last_sensor_data_tick = HAL_GetTick();
    last_sensor_reconfigure_tick = HAL_GetTick();
    sensor_led_current = SENSOR_LED_CURRENT_DEFAULT;
    current.led_current = sensor_led_current;
    last_gain_adjust_tick = HAL_GetTick();
    reset_measurement();
    current.status = current.sensor_ok
                         ? SPO2_APP_PLACE_FINGER
                         : SPO2_APP_SENSOR_ERROR;
    last_sensor_retry_tick = HAL_GetTick();
    increment_sequence();
}

static void try_initialize_rtc(void)
{
    bool valid = false;
    bool previous_ok = current.rtc_ok;
    bool previous_valid = current.rtc_time_valid;

    current.rtc_ok = (TinyRTC_Init(&rtc,
                                   shared_i2c,
                                   configured_rtc_kind) == TINY_RTC_OK);
    if (current.rtc_ok) {
        TinyRTC_DateTime build = build_date_time();
        TinyRTC_DateTime readback = {0};
        TinyRTC_Status read_status = TinyRTC_GetDateTime(&rtc, &readback);

        if ((TinyRTC_IsTimeValid(&rtc, &valid) != TINY_RTC_OK) ||
            (read_status != TINY_RTC_OK)) {
            valid = false;
        }

        /* A fresh DS1307/DS3231 normally contains an invalid or year-2000
           timestamp. Set it once from the local CubeIDE build time. On later
           resets the RTC is already later than __DATE__/__TIME__, so it is
           not pushed backwards. */
        if ((!valid) || (compare_date_time(&readback, &build) < 0)) {
            if (TinyRTC_SetDateTime(&rtc, &build) == TINY_RTC_OK) {
                readback = build;
                valid = true;
            }
        }

        current.date_time = readback;
        current.rtc_time_valid = valid;
    } else {
        current.rtc_time_valid = false;
    }

    if ((previous_ok != current.rtc_ok) ||
        (previous_valid != current.rtc_time_valid)) {
        increment_sequence();
    }
}

static void update_rtc(void)
{
    uint32_t now = HAL_GetTick();

    /* The software clock is always maintained. A missing or electrically
       unstable RTC must never be allowed to take the MAX30102 off the bus. */
    if (!current.rtc_ok) {
        update_software_clock();

        /* Defer RTC probing until the optical sensor has been stable for a
           while. This also lets the project work with MAX30102 alone. */
        if (current.sensor_ok &&
            ((now - app_start_tick) >= RTC_STARTUP_DELAY_MS) &&
            ((now - last_rtc_read_tick) >= RTC_RETRY_INTERVAL_MS)) {
            last_rtc_read_tick = now;
            try_initialize_rtc();
        }
        return;
    }

    if ((now - last_rtc_read_tick) >= RTC_READ_INTERVAL_MS) {
        last_rtc_read_tick = now;
        bool valid = false;
        if ((TinyRTC_GetDateTime(&rtc, &current.date_time) == TINY_RTC_OK) &&
            (TinyRTC_IsTimeValid(&rtc, &valid) == TINY_RTC_OK)) {
            current.rtc_time_valid = valid;
            software_date_time = current.date_time;
            software_clock_tick = now;
            increment_sequence();
        } else {
            /* Fall back to the software clock. Do not reset I2C and do not
               change the MAX30102 state because of an RTC transaction. */
            current.rtc_ok = false;
            current.rtc_time_valid = true;
            software_date_time = current.date_time;
            software_clock_tick = now;
            increment_sequence();
        }
    }
}

static void reset_finger_detector(void)
{
    FingerDetector_Reset(&finger_detector);
    detector_contact = false;
    algorithm_contact = false;
    invalid_algorithm_windows = 0U;
    current.finger_present = false;
    current.finger_threshold = FingerDetector_GetThresholdIR(&finger_detector);
    current.ir_span = 0U;
}

static bool detect_finger(uint32_t ir, uint32_t red)
{
    detector_contact = FingerDetector_Update(&finger_detector, ir, red);
    current.finger_threshold =
        FingerDetector_GetThresholdIR(&finger_detector);
    current.ir_span = FingerDetector_GetSpanIR(&finger_detector);
    current.finger_present = detector_contact || algorithm_contact;
    return detector_contact;
}

static bool auto_adjust_led_current(uint32_t now)
{
    uint32_t mean_ir = FingerDetector_GetMeanIR(&finger_detector);
    uint8_t new_current = sensor_led_current;

    if ((now - last_gain_adjust_tick) < SENSOR_GAIN_ADJUST_INTERVAL_MS ||
        finger_detector.sample_count < 8U) {
        return false;
    }

    if ((mean_ir >= SENSOR_SIGNAL_SATURATED) &&
        (sensor_led_current > SENSOR_LED_CURRENT_MIN)) {
        new_current = (sensor_led_current > 0x10U + SENSOR_LED_CURRENT_MIN)
                          ? (uint8_t)(sensor_led_current - 0x10U)
                          : SENSOR_LED_CURRENT_MIN;
    } else if (!detector_contact && !algorithm_contact &&
               (mean_ir < SENSOR_SIGNAL_TOO_LOW) &&
               (sensor_led_current < SENSOR_LED_CURRENT_MAX)) {
        uint16_t raised = (uint16_t)sensor_led_current + 0x20U;
        new_current = (raised > SENSOR_LED_CURRENT_MAX)
                          ? SENSOR_LED_CURRENT_MAX
                          : (uint8_t)raised;
    }

    last_gain_adjust_tick = now;
    if (new_current == sensor_led_current) {
        return false;
    }

    if (MAX30102_SetLedCurrent(&sensor, new_current, new_current) ==
        MAX30102_OK) {
        sensor_led_current = new_current;
        current.led_current = new_current;
        (void)MAX30102_ClearFIFO(&sensor);
        reset_finger_detector();
        reset_measurement();
        current.status = SPO2_APP_PLACE_FINGER;
        increment_sequence();
        return true;
    }
    return false;
}

static int16_t normalize_waveform(uint32_t ir)
{
    float ac;
    float normalized;

    if (waveform_dc == 0.0f) {
        waveform_dc = (float)ir;
        waveform_amplitude = 1000.0f;
    }

    waveform_dc += ((float)ir - waveform_dc) * 0.02f;
    ac = (float)ir - waveform_dc;
    waveform_amplitude += (fabsf(ac) - waveform_amplitude) * 0.05f;
    if (waveform_amplitude < 100.0f) {
        waveform_amplitude = 100.0f;
    }

    normalized = 50.0f + (40.0f * ac / (3.0f * waveform_amplitude));
    if (normalized < 0.0f) {
        normalized = 0.0f;
    } else if (normalized > 100.0f) {
        normalized = 100.0f;
    }
    return (int16_t)lroundf(normalized);
}

static void push_sample(uint32_t ir, uint32_t red)
{
    ir_ring[ring_write_index] = ir;
    red_ring[ring_write_index] = red;
    ring_write_index = (uint16_t)((ring_write_index + 1U) %
                                  SPO2_APP_BUFFER_LENGTH);

    if (ring_count < SPO2_APP_BUFFER_LENGTH) {
        ++ring_count;
    }
    ++new_samples_since_calculation;
    current.buffered_samples = ring_count;
}

static void linearize_buffers(void)
{
    uint16_t oldest = (ring_count < SPO2_APP_BUFFER_LENGTH)
                          ? 0U
                          : ring_write_index;
    uint16_t i;

    for (i = 0U; i < ring_count; ++i) {
        uint16_t source = (uint16_t)((oldest + i) %
                                     SPO2_APP_BUFFER_LENGTH);
        ir_linear[i] = ir_ring[source];
        red_linear[i] = red_ring[source];
    }
}

static void calculate_measurement(void)
{
    SpO2AlgorithmResult result;
    int16_t low_hr;
    int16_t high_hr;
    bool any_valid;

    if (ring_count < SPO2_APP_BUFFER_LENGTH) {
        current.status = current.finger_present
                             ? SPO2_APP_MEASURING
                             : SPO2_APP_PLACE_FINGER;
        return;
    }

    linearize_buffers();
    SpO2Algorithm_Compute(ir_linear,
                          red_linear,
                          SPO2_APP_BUFFER_LENGTH,
                          SPO2_APP_FIFO_SAMPLE_RATE_HZ,
                          0U,
                          &result);

    ++current.measurement_sequence;
    if (current.measurement_sequence == 0U) {
        current.measurement_sequence = 1U;
    }

    current.signal_quality = result.signal_quality;
    current.heart_rate_valid = result.heart_rate_valid;
    current.spo2_valid = result.spo2_valid;
    current.measurement_valid = current.heart_rate_valid &&
                                current.spo2_valid;
    current.heart_rate_bpm = current.heart_rate_valid
                                 ? result.heart_rate_bpm : 0;
    current.spo2_percent = current.spo2_valid
                               ? result.spo2_percent : 0;

    any_valid = current.heart_rate_valid || current.spo2_valid;
    if (any_valid) {
        /* A valid periodic PPG result is stronger evidence of contact than the
           lightweight threshold detector. Keep contact latched between
           recalculation windows so the UI cannot immediately fall back to
           PLACE FINGER on the next raw sample. */
        algorithm_contact = true;
        invalid_algorithm_windows = 0U;
    } else {
        if (invalid_algorithm_windows < 255U) {
            ++invalid_algorithm_windows;
        }
        if (!detector_contact && (invalid_algorithm_windows >= 3U)) {
            algorithm_contact = false;
        }
    }
    current.finger_present = detector_contact || algorithm_contact;

    if (!any_valid) {
        current.low_spo2 = false;
        current.abnormal_heart_rate = false;
        current.status = current.finger_present
                             ? SPO2_APP_INVALID_SIGNAL
                             : SPO2_APP_PLACE_FINGER;
        return;
    }

    /* Keep collecting and show a valid partial result while waiting for the
       other channel. BPM uses IR; SpO2 requires both Red and IR. */
    if (!current.measurement_valid) {
        current.low_spo2 = false;
        current.abnormal_heart_rate = false;
        current.status = SPO2_APP_MEASURING;
        return;
    }

    if (average_heart_rate <= 0.0f) {
        average_heart_rate = (float)current.heart_rate_bpm;
    } else {
        average_heart_rate = 0.90f * average_heart_rate +
                             0.10f * (float)current.heart_rate_bpm;
    }

    if (average_heart_rate < 80.0f) {
        low_hr = 45;
        high_hr = 105;
    } else {
        low_hr = 50;
        high_hr = 110;
    }

    current.low_spo2 = current.spo2_percent <
                       SPO2_APP_LOW_SPO2_THRESHOLD;
    current.abnormal_heart_rate =
        (current.heart_rate_bpm < low_hr) ||
        (current.heart_rate_bpm > high_hr);

    if (current.low_spo2) {
        current.status = SPO2_APP_LOW_SPO2;
    } else if (current.heart_rate_bpm < low_hr) {
        current.status = SPO2_APP_LOW_HEART_RATE;
    } else if (current.heart_rate_bpm > high_hr) {
        current.status = SPO2_APP_HIGH_HEART_RATE;
    } else {
        current.status = SPO2_APP_NORMAL;
    }
}

static void process_sample(const MAX30102_Sample *sample)
{
    bool detector_before = detector_contact;
    bool finger_now;

    current.raw_ir = sample->ir;
    current.raw_red = sample->red;
    finger_now = detect_finger(sample->ir, sample->red);

    if (auto_adjust_led_current(HAL_GetTick())) {
        return;
    }

    /* Start a clean acquisition window when the fast detector sees a new
       finger. If the detector never triggers on a low-signal clone module, the
       continuously rolling buffer below still lets the PPG algorithm decide. */
    if (finger_now && !detector_before) {
        reset_measurement();
    }

    current.waveform = normalize_waveform(sample->ir);

    /* V16 always buffers sensor samples. V15 returned here whenever the
       detector said "no finger", which made it logically impossible for the
       independent BPM/SpO2 algorithm to rescue a missed detection. */
    push_sample(sample->ir, sample->red);

    current.finger_present = detector_contact || algorithm_contact;
    if (current.finger_present) {
        current.status = SPO2_APP_MEASURING;
    } else {
        current.status = SPO2_APP_PLACE_FINGER;
    }

    if ((ring_count == SPO2_APP_BUFFER_LENGTH) &&
        (new_samples_since_calculation >=
         SPO2_APP_RECALCULATE_SAMPLES)) {
        new_samples_since_calculation = 0U;
        calculate_measurement();
    }
    increment_sequence();
}

void SpO2App_Init(I2C_HandleTypeDef *hi2c, TinyRTC_Kind rtc_kind)
{
    memset(&sensor, 0, sizeof(sensor));
    memset(&rtc, 0, sizeof(rtc));
    memset(&current, 0, sizeof(current));
    memset(ir_ring, 0, sizeof(ir_ring));
    memset(red_ring, 0, sizeof(red_ring));

    shared_i2c = hi2c;
    configured_rtc_kind = rtc_kind;
    current.status = SPO2_APP_SENSOR_ERROR;
    current.sensor_error = MAX30102_STATUS_CHECKING;
    current.sequence = 1U;
    current.measurement_sequence = 0U;
    current.waveform = 50;
    current.raw_ir = 0U;
    current.raw_red = 0U;
    current.finger_threshold = 0U;
    current.ir_span = 0U;
    current.buffered_samples = 0U;
    current.led_current = SENSOR_LED_CURRENT_DEFAULT;
    waveform_dc = 0.0f;
    waveform_amplitude = 1000.0f;
    software_date_time = build_date_time();
    software_clock_tick = HAL_GetTick();
    app_start_tick = software_clock_tick;
    current.date_time = software_date_time;
    current.rtc_time_valid = true;

    consecutive_sensor_io_errors = 0U;
    consecutive_sensor_missing_checks = 0U;
    last_sensor_data_tick = HAL_GetTick();
    last_sensor_reconfigure_tick = HAL_GetTick();
    last_sensor_debug_tick = 0U;
    last_force_start_tick = 0U;
    stream_restart_attempts = 0U;
    try_initialize_sensor();
    update_sensor_diagnostic(HAL_GetTick());

    /* Start with the build-time software clock. Probe the external RTC only
       after the MAX30102 has proved stable, so a bad RTC module cannot make
       the optical sensor disappear. */
    current.rtc_ok = false;
    last_rtc_read_tick = HAL_GetTick();
}

static void handle_sensor_read_error(MAX30102_Status status, uint32_t now)
{
    current.sensor_error = status;
    if (consecutive_sensor_io_errors < 255U) {
        ++consecutive_sensor_io_errors;
    }

    /* A single NACK/BERR can occur because of wiring capacitance or another
       device on the shared bus. Do not report the sensor as physically
       missing until the address probe fails repeatedly. */
    if (consecutive_sensor_io_errors < SENSOR_IO_ERROR_LIMIT) {
        return;
    }

    if (HAL_I2C_IsDeviceReady(shared_i2c,
                              MAX30102_I2C_ADDRESS_HAL,
                              2U,
                              20U) == HAL_OK) {
        consecutive_sensor_missing_checks = 0U;

        if ((now - last_sensor_reconfigure_tick) >=
            SENSOR_RECONFIGURE_INTERVAL_MS) {
            MAX30102_Status reconfigure_status;
            last_sensor_reconfigure_tick = now;
            reconfigure_status = MAX30102_InitDefault(&sensor, shared_i2c);
            if (reconfigure_status == MAX30102_OK) {
                consecutive_sensor_io_errors = 0U;
                current.sensor_error = MAX30102_OK;
                current.sensor_ok = true;
                current.status = SPO2_APP_PLACE_FINGER;
                sensor_led_current = SENSOR_LED_CURRENT_DEFAULT;
                current.led_current = sensor_led_current;
                last_gain_adjust_tick = now;
                last_sensor_data_tick = now;
                reset_measurement();
                reset_finger_detector();
                increment_sequence();
            }
        }
        return;
    }

    if (consecutive_sensor_missing_checks < 255U) {
        ++consecutive_sensor_missing_checks;
    }

    if (consecutive_sensor_missing_checks >=
        SENSOR_MISSING_CONFIRM_LIMIT) {
        current.sensor_error = MAX30102_DiagnoseBus(shared_i2c);
        if (current.sensor_error == MAX30102_OK) {
            current.sensor_error = MAX30102_ERROR_I2C;
        }
        current.sensor_ok = false;
        current.status = SPO2_APP_SENSOR_ERROR;
        last_sensor_retry_tick = now;
        reset_measurement();
        increment_sequence();
    }
}

void SpO2App_Process(void)
{
    MAX30102_Sample samples[MAX_SAMPLES_PER_TICK];
    uint8_t sample_count = 0U;
    uint8_t i;
    MAX30102_Status status;
    uint32_t now = HAL_GetTick();

    if (!current.sensor_ok) {
        update_rtc();
        if ((now - last_sensor_retry_tick) >=
            SENSOR_RETRY_INTERVAL_MS) {
            try_initialize_sensor();
        }
        return;
    }

    /* Give MAX30102 first access to the shared bus. RTC reads are performed
       afterwards and can fall back to the software clock independently. */
    status = MAX30102_ReadAvailable(&sensor,
                                    samples,
                                    MAX_SAMPLES_PER_TICK,
                                    &sample_count);
    if (status != MAX30102_OK) {
        handle_sensor_read_error(status, now);
        update_rtc();
        return;
    }

    consecutive_sensor_io_errors = 0U;
    consecutive_sensor_missing_checks = 0U;
    current.sensor_error = MAX30102_OK;

    if (sample_count > 0U) {
        last_sensor_data_tick = now;
        stream_restart_attempts = 0U;
        for (i = 0U; i < sample_count; ++i) {
            process_sample(&samples[i]);
        }
    } else {
        update_sensor_diagnostic(now);

        if ((now - last_sensor_data_tick) >= SENSOR_STALL_TIMEOUT_MS &&
            (now - last_force_start_tick) >=
                SENSOR_FORCE_START_INTERVAL_MS) {
            MAX30102_Status restart_status;
            last_force_start_tick = now;
            restart_status = MAX30102_ForceMeasurement(&sensor,
                                                       sensor_led_current);
            if (restart_status == MAX30102_OK) {
                if (stream_restart_attempts < 255U) {
                    ++stream_restart_attempts;
                }
                current.status = SPO2_APP_PLACE_FINGER;
                current.sensor_error = MAX30102_OK;
                last_gain_adjust_tick = now;
                reset_measurement();
                reset_finger_detector();
                update_sensor_diagnostic(HAL_GetTick());
            } else {
                handle_sensor_read_error(restart_status, now);
            }
        }
    }

    update_rtc();
}

void SpO2App_GetSnapshot(SpO2AppSnapshot *snapshot)
{
    if (snapshot != NULL) {
        uint32_t primask = __get_PRIMASK();
        __disable_irq();
        *snapshot = current;
        if (primask == 0U) {
            __enable_irq();
        }
    }
}

TinyRTC_Status SpO2App_SetDateTime(const TinyRTC_DateTime *date_time)
{
    TinyRTC_Status status;

    if (!current.rtc_ok) {
        return TINY_RTC_ERROR_NOT_FOUND;
    }

    status = TinyRTC_SetDateTime(&rtc, date_time);
    if (status == TINY_RTC_OK) {
        current.date_time = *date_time;
        current.rtc_time_valid = true;
        increment_sequence();
    }
    return status;
}
