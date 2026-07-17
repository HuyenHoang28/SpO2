#include "spo2_app.h"
#include "spo2_algorithm.h"

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
#define FINGER_LOST_SAMPLE_COUNT        15U
#define SENSOR_INIT_ATTEMPTS            3U

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
static uint8_t finger_lost_count;
static uint32_t last_sensor_retry_tick;
static uint32_t last_sensor_data_tick;
static uint32_t last_sensor_reconfigure_tick;
static uint8_t consecutive_sensor_io_errors;
static uint8_t consecutive_sensor_missing_checks;
static uint32_t last_rtc_read_tick;
static float average_heart_rate;
static float waveform_dc;
static float waveform_amplitude;
static TinyRTC_DateTime software_date_time;
static uint32_t software_clock_tick;
static uint32_t app_start_tick;

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
    current.measurement_valid = false;
    current.low_spo2 = false;
    current.abnormal_heart_rate = false;
    waveform_dc = 0.0f;
    waveform_amplitude = 1000.0f;
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
    finger_lost_count = 0U;
    consecutive_sensor_io_errors = 0U;
    consecutive_sensor_missing_checks = 0U;
    last_sensor_data_tick = HAL_GetTick();
    last_sensor_reconfigure_tick = HAL_GetTick();
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

    if (ring_count < SPO2_APP_BUFFER_LENGTH) {
        current.status = SPO2_APP_MEASURING;
        return;
    }

    linearize_buffers();
    SpO2Algorithm_Compute(ir_linear,
                          red_linear,
                          SPO2_APP_BUFFER_LENGTH,
                          SPO2_APP_FIFO_SAMPLE_RATE_HZ,
                          SPO2_APP_FINGER_IR_THRESHOLD,
                          &result);

    ++current.measurement_sequence;
    if (current.measurement_sequence == 0U) {
        current.measurement_sequence = 1U;
    }

    current.signal_quality = result.signal_quality;
    current.finger_present = result.finger_present;
    current.measurement_valid = result.spo2_valid &&
                                result.heart_rate_valid;

    if (!current.finger_present) {
        current.status = SPO2_APP_PLACE_FINGER;
        reset_measurement();
        return;
    }

    if (!current.measurement_valid) {
        current.heart_rate_bpm = 0;
        current.spo2_percent = 0;
        current.low_spo2 = false;
        current.abnormal_heart_rate = false;
        current.status = SPO2_APP_INVALID_SIGNAL;
        return;
    }

    current.heart_rate_bpm = result.heart_rate_bpm;
    current.spo2_percent = result.spo2_percent;

    if (average_heart_rate <= 0.0f) {
        average_heart_rate = (float)current.heart_rate_bpm;
    } else {
        average_heart_rate = 0.90f * average_heart_rate +
                             0.10f * (float)current.heart_rate_bpm;
    }

    /* Preserve the adaptive thresholds used by the reference project. */
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
    current.raw_ir = sample->ir;
    current.raw_red = sample->red;

    if (sample->ir < SPO2_APP_FINGER_IR_THRESHOLD) {
        current.waveform = 50;
        if (finger_lost_count < 255U) {
            ++finger_lost_count;
        }
        if (finger_lost_count >= FINGER_LOST_SAMPLE_COUNT) {
            current.finger_present = false;
            current.status = SPO2_APP_PLACE_FINGER;
            reset_measurement();
        }
        increment_sequence();
        return;
    }

    current.waveform = normalize_waveform(sample->ir);
    finger_lost_count = 0U;
    current.finger_present = true;
    if (current.status == SPO2_APP_PLACE_FINGER) {
        current.status = SPO2_APP_MEASURING;
    }

    push_sample(sample->ir, sample->red);
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
    try_initialize_sensor();

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
                last_sensor_data_tick = now;
                reset_measurement();
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
        for (i = 0U; i < sample_count; ++i) {
            process_sample(&samples[i]);
        }
    } else if ((now - last_sensor_data_tick) >= SENSOR_STALL_TIMEOUT_MS &&
               (now - last_sensor_reconfigure_tick) >=
                   SENSOR_RECONFIGURE_INTERVAL_MS) {
        /* Device still ACKs but FIFO stopped advancing. Reconfigure in place;
           this is a stalled stream, not a physically missing sensor. */
        MAX30102_Status reconfigure_status;
        last_sensor_reconfigure_tick = now;
        reconfigure_status = MAX30102_InitDefault(&sensor, shared_i2c);
        if (reconfigure_status == MAX30102_OK) {
            current.status = SPO2_APP_PLACE_FINGER;
            current.sensor_error = MAX30102_OK;
            last_sensor_data_tick = now;
            reset_measurement();
            increment_sequence();
        } else {
            handle_sensor_read_error(reconfigure_status, now);
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
