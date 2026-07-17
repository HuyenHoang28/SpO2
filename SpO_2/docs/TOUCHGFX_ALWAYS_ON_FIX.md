# TouchGFX always-on fix

## Root cause

The overridden `TouchGFXHAL::taskEntry()` only waited for VSYNC in an infinite loop. It never called the TouchGFX framework event loop, so no screen, widget, model tick, or draw operation was processed.

## Changes

- `TouchGFXHAL::taskEntry()` now calls `TouchGFXGeneratedHAL::taskEntry()`.
- Startup RGB bars are disabled (`LCD_STARTUP_TEST = 0`).
- GUI task priority is `osPriorityAboveNormal`; sensor task is `osPriorityBelowNormal`.
- MAX30102/RTC initialization starts 500 ms after the scheduler so the first GUI frame is rendered first.
- Touch controller access is fail-safe if STMPE811 initialization fails.
- Shared I2C touch timeout reduced to 50 ms to prevent long GUI stalls.

After flashing, the first screen should appear directly without the RGB test pattern.
