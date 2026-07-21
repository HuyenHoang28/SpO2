# TouchGFX blue/cyan screen fix — v9

## Symptom observed on the physical board

The ILI9341 backlight and LTDC output are active, but the display shows one
uniform light-blue/cyan color and no TouchGFX widgets.

The v8 firmware intentionally overwrote the framebuffer with a diagnostic
color when the GUI task had entered its loop but the expected Screen1 marker
pixels were not found. Because the panel/RGB wiring can alter apparent channel
order, the diagnostic yellow value can appear blue/cyan on the panel.

## Root causes found in the project

### 1. Wrong GPIO pins used as diagnostic LEDs

The v8 GUI loop used PD12 and PD13 as status outputs. On the STM32F429I-DISCO
board these pins belong to the ILI9341 command interface. In this project,
PD13 is explicitly driven by `LCD_IO_WriteReg()` and `LCD_IO_WriteData()` as
the LCD WRX/D-CX signal. Pulling it low from the GUI task corrupts the LCD
control state.

The actual board LEDs are:

- LD3 green: PG13
- LD4 red: PG14

v9 never uses PD12/PD13 for diagnostics. They are kept high after LCD
initialization, except while the LCD driver intentionally sends a command.

### 2. Custom GUI loop bypassed the framework event loop

v8 manually called `vSync()`, `backPorchExited()` and `frontPorchEntered()`.
Although useful for isolated testing, this does not reproduce the full
TouchGFX 4.26 task/semaphore/VSYNC state machine on the target.

v9 restores the framework event loop by calling the generated/base
`taskEntry()`. The LTDC line callback signals the VSYNC queue, and the official
TouchGFX loop performs screen creation, model ticks, invalidation and rendering.

## Stable v9 display architecture

- One RGB565 framebuffer only
- Framebuffer fixed by the linker at `0xD0000000`
- LTDC layer scans the same framebuffer
- `touchgfx::NoDMA` during display bring-up
- No animation framebuffer in SDRAM
- No diagnostic color overwrite
- Touch input disabled temporarily to avoid I2C3 contention
- USER button PA0 changes screens
- MAX30102/RTC starts 1.5 seconds after the GUI task

## Runtime indications

- PG13 toggles after every 30 completed TouchGFX framebuffer flushes.
- PG14 turns on continuously in `Error_Handler()` or `HardFault_Handler()`.
- PD12/PD13 are never used as LEDs.

## Required build procedure

1. Extract v9 into a new directory.
2. Import `SpO_2/STM32CubeIDE` as an existing project.
3. Delete any `Debug` and `Release` directories before building.
4. Run Project > Clean.
5. Build and flash.
6. Press the hardware RESET button.
7. Do not regenerate code from the `.ioc` file.

For the first test, disconnect MAX30102 and Tiny RTC. Verify that Screen1 is
visible before reconnecting the external I2C devices to PA8/PC9.
