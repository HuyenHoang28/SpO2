#include "finger_detector.h"

#include <stdint.h>
#include <stdio.h>

static int test_open_then_finger(void)
{
    FingerDetector d;
    int i;
    FingerDetector_Reset(&d);

    for (i = 0; i < 40; ++i) {
        uint32_t ir = 110U + (uint32_t)(i % 3);
        uint32_t red = 70U + (uint32_t)(i % 2);
        if (FingerDetector_Update(&d, ir, red)) return 1;
    }
    for (i = 0; i < 40; ++i) {
        uint32_t pulse = (uint32_t)((i % 10) < 5 ? (i % 5) * 8 : (10 - (i % 10)) * 8);
        uint32_t ir = 850U + pulse;
        uint32_t red = 620U + pulse / 2U;
        if (FingerDetector_Update(&d, ir, red)) return 0;
    }
    return 2;
}

static int test_finger_present_at_start(void)
{
    FingerDetector d;
    int i;
    FingerDetector_Reset(&d);

    for (i = 0; i < 50; ++i) {
        uint32_t pulse = (uint32_t)((i % 12) < 6 ? (i % 6) * 15 : (12 - (i % 12)) * 15);
        if (FingerDetector_Update(&d, 3200U + pulse, 2300U + pulse / 2U)) {
            return 0;
        }
    }
    return 3;
}

static int test_low_signal_finger(void)
{
    FingerDetector d;
    int i;
    FingerDetector_Reset(&d);

    for (i = 0; i < 30; ++i) {
        if (FingerDetector_Update(&d, 80U + (uint32_t)(i & 1),
                                  45U + (uint32_t)(i & 1))) return 4;
    }
    for (i = 0; i < 60; ++i) {
        uint32_t pulse = (uint32_t)((i % 10) < 5 ? (i % 5) * 4 : (10 - (i % 10)) * 4);
        if (FingerDetector_Update(&d, 340U + pulse, 220U + pulse / 2U)) {
            return 0;
        }
    }
    return 5;
}

static int test_constant_ambient_not_finger(void)
{
    FingerDetector d;
    int i;
    FingerDetector_Reset(&d);
    for (i = 0; i < 100; ++i) {
        if (FingerDetector_Update(&d, 1500U + (uint32_t)(i & 1),
                                  900U + (uint32_t)(i & 1))) return 6;
    }
    return 0;
}

int main(void)
{
    int result;
    result = test_open_then_finger();
    if (result) return result;
    result = test_finger_present_at_start();
    if (result) return result;
    result = test_low_signal_finger();
    if (result) return result;
    result = test_constant_ambient_not_finger();
    if (result) return result;
    puts("finger detector tests passed");
    return 0;
}
