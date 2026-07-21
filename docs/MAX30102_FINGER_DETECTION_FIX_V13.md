# MAX30102 finger detection and status cleanup — v13

## What was fixed

1. The tail of `BUS OK, NO 0x57` could remain visible after the state changed to
   `PLACE FINGER`. The old status widget was resized to a smaller rectangle and
   only that smaller rectangle was invalidated. v13 invalidates the complete
   status panel before drawing the new status.
2. Finger detection is now adaptive. The first 25 FIFO samples learn the
   open-air IR baseline. A finger is accepted after three consecutive samples
   rise sufficiently above that baseline; removal uses a separate lower
   threshold and one-second debounce.
3. MAX30102 Red/IR LED current is increased from `0x3F` to `0x7F` to improve
   optical return on low-output GY-MAX30102 clone boards.
4. Once a finger is found, the status changes immediately to `MEASURING...`.
   The first BPM/SpO2 estimate needs about four seconds of stable data.

## Expected display sequence

```
CHECKING SENSOR
PLACE FINGER
MEASURING...
NORMAL / LOW SPO2 / ...
```

Hold the fleshy part of the fingertip still over the optical window for 5–8
seconds. Do not press hard. This algorithm is for a course project and is not a
medical device.
