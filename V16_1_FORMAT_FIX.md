# V16.1 TouchGFX raw-value formatting fix

TouchGFX `Unicode::snprintf()` supports `%u`, `%i`, `%d`, `%o`, and `%x`, but it does not support the C length modifier `%l`. Therefore `%lu` was rendered literally as `%lu` instead of substituting `rawIr` and `rawRed`.

Changes:

- Replaced every `%lu` with `%u`.
- Cast `uint32_t` raw values to `unsigned`.
- Added `N:` (buffered sample count) to the PLACE FINGER and HOLD status screens.
- Added the visible version marker `V16.1`.

Expected display:

```text
V16.1 PF N:100
I:54321 R:42110
```

`N` increasing proves FIFO samples are reaching the application. BPM and SpO2 remain `--` until their independent validity checks pass; the firmware does not fabricate measurements.
