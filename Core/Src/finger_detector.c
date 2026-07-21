#include "finger_detector.h"

#include <stddef.h>
#include <string.h>

#define DETECTOR_MIN_READY_SAMPLES       8U
#define DETECTOR_FOUND_CONFIRM_SAMPLES   2U
#define DETECTOR_LOST_CONFIRM_SAMPLES    20U
#define DETECTOR_IR_DC_MIN               100U
#define DETECTOR_IR_SPAN_MIN             4U
#define DETECTOR_BASELINE_STEP_MIN       50U
#define DETECTOR_BASELINE_STEP_DIVISOR   10U
#define DETECTOR_BASELINE_EMA_DIVISOR    32U
#define DETECTOR_STRONG_IR_LEVEL         3000U
#define DETECTOR_SATURATION_LEVEL        260000U

static uint32_t abs_diff_u32(uint32_t a, uint32_t b)
{
    return (a >= b) ? (a - b) : (b - a);
}

static uint32_t max_u32(uint32_t a, uint32_t b)
{
    return (a > b) ? a : b;
}

static void update_statistics(FingerDetector *detector)
{
    uint64_t ir_sum = 0U;
    uint64_t red_sum = 0U;
    uint32_t ir_min = UINT32_MAX;
    uint32_t red_min = UINT32_MAX;
    uint32_t ir_max = 0U;
    uint32_t red_max = 0U;
    uint8_t i;

    for (i = 0U; i < detector->sample_count; ++i) {
        uint32_t ir = detector->ir_window[i];
        uint32_t red = detector->red_window[i];
        ir_sum += ir;
        red_sum += red;
        if (ir < ir_min) ir_min = ir;
        if (ir > ir_max) ir_max = ir;
        if (red < red_min) red_min = red;
        if (red > red_max) red_max = red;
    }

    if (detector->sample_count == 0U) {
        detector->mean_ir = 0U;
        detector->mean_red = 0U;
        detector->span_ir = 0U;
        detector->span_red = 0U;
        return;
    }

    detector->mean_ir = (uint32_t)(ir_sum / detector->sample_count);
    detector->mean_red = (uint32_t)(red_sum / detector->sample_count);
    detector->span_ir = ir_max - ir_min;
    detector->span_red = red_max - red_min;
}

void FingerDetector_Reset(FingerDetector *detector)
{
    if (detector == NULL) {
        return;
    }
    memset(detector, 0, sizeof(*detector));
    detector->threshold_ir = DETECTOR_IR_DC_MIN;
}

bool FingerDetector_Update(FingerDetector *detector,
                           uint32_t ir,
                           uint32_t red)
{
    bool dc_valid;
    bool pulse_candidate;
    bool step_candidate = false;
    bool strong_candidate;
    bool candidate;
    uint32_t ir_span_required;
    uint32_t step_required;

    if (detector == NULL) {
        return false;
    }

    detector->ir_window[detector->write_index] = ir;
    detector->red_window[detector->write_index] = red;
    detector->write_index = (uint8_t)((detector->write_index + 1U) %
                                      FINGER_DETECTOR_WINDOW_SIZE);
    if (detector->sample_count < FINGER_DETECTOR_WINDOW_SIZE) {
        ++detector->sample_count;
    }
    update_statistics(detector);

    if (detector->sample_count < DETECTOR_MIN_READY_SAMPLES) {
        return detector->finger_present;
    }

    /* Dynamic span limits reject near-constant open-air readings while still
       accepting low-amplitude PPG signals from inexpensive breakout boards. */
    ir_span_required = max_u32(DETECTOR_IR_SPAN_MIN,
                               detector->mean_ir / 2000U); /* 0.05% */
    /* Finger presence must be decided from the IR channel. Requiring the
       Red channel here prevented measurement on modules whose Red LED or Red
       optical path is weaker, even though the IR PPG signal was usable for
       BPM. Red is still processed later for SpO2 validity. */
    dc_valid = (detector->mean_ir >= DETECTOR_IR_DC_MIN) &&
               (detector->mean_ir < DETECTOR_SATURATION_LEVEL);

    pulse_candidate = dc_valid &&
                      (detector->span_ir >= ir_span_required);

    if (detector->baseline_valid) {
        step_required = max_u32(DETECTOR_BASELINE_STEP_MIN,
                                detector->baseline_ir /
                                DETECTOR_BASELINE_STEP_DIVISOR);
        detector->threshold_ir = detector->baseline_ir + step_required;
        step_candidate = dc_valid &&
                         (abs_diff_u32(detector->mean_ir,
                                       detector->baseline_ir) >= step_required);
    } else {
        detector->threshold_ir = DETECTOR_IR_DC_MIN;
    }

    strong_candidate = dc_valid &&
                       (detector->mean_ir >= DETECTOR_STRONG_IR_LEVEL) &&
                       (detector->span_ir >= 2U);

    candidate = pulse_candidate || step_candidate || strong_candidate;

    if (!detector->finger_present) {
        if (candidate) {
            if (detector->found_count < 255U) {
                ++detector->found_count;
            }
            if (detector->found_count >= DETECTOR_FOUND_CONFIRM_SAMPLES) {
                detector->finger_present = true;
                detector->lost_count = 0U;
                return true;
            }
        } else {
            detector->found_count = 0U;

            /* Learn only samples that still look like open air. The v13
               implementation averaged the first second unconditionally, so
               placing a finger immediately taught the detector that the
               finger level itself was the no-finger baseline. */
            if (!detector->baseline_valid) {
                detector->baseline_ir = detector->mean_ir;
                detector->baseline_red = detector->mean_red;
                detector->baseline_valid = true;
            } else {
                detector->baseline_ir =
                    (uint32_t)(((uint64_t)detector->baseline_ir *
                                (DETECTOR_BASELINE_EMA_DIVISOR - 1U) +
                                detector->mean_ir) /
                               DETECTOR_BASELINE_EMA_DIVISOR);
                detector->baseline_red =
                    (uint32_t)(((uint64_t)detector->baseline_red *
                                (DETECTOR_BASELINE_EMA_DIVISOR - 1U) +
                                detector->mean_red) /
                               DETECTOR_BASELINE_EMA_DIVISOR);
            }
        }
        return false;
    }

    /* Once contact is established, do not drop it because of one quiet part
       of the pulse waveform. Require a sustained return to the learned
       baseline or a near-zero optical signal. */
    {
        bool near_zero =
            (detector->mean_ir < (DETECTOR_IR_DC_MIN / 2U));
        bool near_baseline = false;

        if (detector->baseline_valid) {
            uint32_t close_limit = max_u32(80U,
                                            detector->baseline_ir / 10U);
            near_baseline =
                (abs_diff_u32(detector->mean_ir,
                              detector->baseline_ir) <= close_limit) &&
                (detector->span_ir < ir_span_required);
        }

        if (near_zero || near_baseline) {
            if (detector->lost_count < 255U) {
                ++detector->lost_count;
            }
            if (detector->lost_count >= DETECTOR_LOST_CONFIRM_SAMPLES) {
                detector->finger_present = false;
                detector->found_count = 0U;
                return false;
            }
        } else {
            detector->lost_count = 0U;
        }
    }

    return true;
}

uint32_t FingerDetector_GetMeanIR(const FingerDetector *detector)
{
    return (detector != NULL) ? detector->mean_ir : 0U;
}

uint32_t FingerDetector_GetMeanRed(const FingerDetector *detector)
{
    return (detector != NULL) ? detector->mean_red : 0U;
}

uint32_t FingerDetector_GetThresholdIR(const FingerDetector *detector)
{
    return (detector != NULL) ? detector->threshold_ir : 0U;
}

uint32_t FingerDetector_GetSpanIR(const FingerDetector *detector)
{
    return (detector != NULL) ? detector->span_ir : 0U;
}
