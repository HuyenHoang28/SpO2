# SpO2 v16 - continuous acquisition and visible raw diagnostics

## Root causes found in v15

1. `process_sample()` returned immediately whenever the finger detector was false. Therefore the 100-sample ring buffer stayed empty and the BPM/SpO2 algorithm could never run if the detector missed the signal.
2. The status string containing `PLACE FINGER I:... R:...` was wider than the 240 px LCD. The code resized and centered that oversized widget, pushing the raw values outside the visible area.

## V16 changes

- Buffer Red/IR continuously whenever MAX30102 FIFO produces samples.
- Use the finger detector only for fast UI feedback, not as a hard gate.
- Let a valid PPG result latch optical contact.
- Clear the contact latch only after three consecutive invalid analysis windows.
- Render status in two fixed lines and show `V16`, raw IR and raw Red.
- Recalculate every 25 new FIFO samples after the 100-sample window is full.
- Relax the minimum IR AC/DC ratio from 0.015% to 0.005% for low-amplitude breakout boards.

Expected status after flashing:

```
V16 PLACE FINGER
I:... R:...
```

With a finger, the rolling buffer can reach 100% even if the threshold detector fails:

```
V16 MEAS 100%
I:... R:...
```
