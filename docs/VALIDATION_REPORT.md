# Validation report — SpO₂ TouchGFX v8

## Static target checks

Result: PASS

Files checked:

- `TouchGFX/target/TouchGFXHAL.cpp`
- `TouchGFX/target/generated/TouchGFXGeneratedHAL.cpp`
- `TouchGFX/target/generated/TouchGFXConfiguration.cpp`
- `TouchGFX/target/generated/STM32DMA.cpp`
- `TouchGFX/gui/src/screen1_screen/Screen1View.cpp`
- `Core/Src/main.c`
- `Core/Src/stm32f4xx_it.c`

The files pass syntax validation with Clang 17 using the project's HAL, CMSIS,
FreeRTOS and TouchGFX include directories.

## Linker placement probe

Result: PASS

Expected framebuffer:

```text
Address: 0xD0000000
Size:    0x25800 (153600 bytes)
Format:  240 x 320 x RGB565
```

The linker script contains hard `ASSERT` checks for both address and size.

## Full TouchGFX render path

Result: PASS

A headless test linked the project's actual generated GUI, `FrontendHeap`, fonts,
texts and TouchGFX 4.26.1 framework. It invoked the real application tick path for
five frames. Marker pixel totals were:

```text
cyan=1200
green=7244
blue=7424
status=11142
white=421
```

The output contains the full Screen1 UI and is saved alongside the delivered
archive as `TouchGFX_Screen1_Validated_v8.png`.

## Hardware limitation

The project could not be flashed to the user's physical STM32F429I-DISCO from the
packaging environment. Final on-board confirmation must therefore use the v8 LCD
diagnostic colors and the post-build map file.
