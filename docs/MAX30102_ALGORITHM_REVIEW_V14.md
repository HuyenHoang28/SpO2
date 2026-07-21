# MAX30102 algorithm review — v14

## Root causes found in v13

### 1. Startup calibration could learn the finger as open-air baseline

The v13 detector averaged the first 25 IR samples unconditionally. The UI already
showed `PLACE FINGER` during this interval. If the finger was placed immediately,
the finger level was included in the no-finger baseline and the threshold became
higher than the live finger signal.

### 2. Finger presence was checked twice with different state

The application detector could accept the finger, but `SpO2Algorithm_Compute()`
received the adaptive threshold and independently rejected the same 100-sample
window. This caused the state to return to `PLACE FINGER`.

### 3. LED current had been raised too far

v13 used `0x7F`. The reference configuration uses brightness 60 (`0x3C`). Excess
current can push inexpensive modules near ADC saturation. v14 starts at `0x3C`
and adjusts only when the measured raw level is too low or saturated.

## v14 changes

- New `finger_detector.c/.h`, independently unit-tested.
- Uses a 16-sample Red/IR window.
- Detects contact from:
  - a simultaneous Red/IR DC step from the learned open-air baseline;
  - correlated Red/IR variation compatible with a PPG signal;
  - a strong optical level with non-constant IR data.
- Never learns a sample as open-air while it already looks like finger contact.
- Supports a finger being present from the very first sensor samples.
- Requires sustained loss before returning to `PLACE FINGER`.
- Removes the second adaptive-threshold finger rejection inside the BPM/SpO2
  calculation.
- Starts with the reference LED current `0x3C` and automatically adjusts between
  `0x20` and `0x7F` when necessary.
- Status text now displays the raw IR value.
- During acquisition it displays the number of buffered samples as a percentage.

## Expected display sequence

```text
PLACE FINGER IR:xxx
MEASURING 1% IR:xxxxx
...
MEASURING 100% IR:xxxxx
NORMAL / LOW SPO2 / WEAK SIGNAL
```

At 25 FIFO samples per second, collecting 100 samples takes about four seconds.

## How to interpret the raw IR value

- IR stays at 0: FIFO/data path is not producing optical samples.
- IR changes strongly when the finger is placed: optical hardware works.
- Status changes to `MEASURING`: finger detector works.
- `MEASURING` reaches 100% and then shows `WEAK SIGNAL`: acquisition works; the
  waveform is too noisy/flat for BPM or SpO2 validation.
- IR is near 262143: the ADC is saturated; v14 automatically lowers LED current.

## Validation performed

Host-side tests:

- no-finger then finger;
- finger already present at startup;
- low-signal finger;
- constant ambient light must not be classified as a finger;
- 75 BPM / 97% synthetic PPG;
- low-amplitude synthetic PPG.

C and C++ syntax checks were also run against the STM32 HAL and TouchGFX headers.
A complete ARM link/flash still needs STM32CubeIDE on the target computer.
