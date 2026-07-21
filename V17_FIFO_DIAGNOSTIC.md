# V17 – FIFO diagnostics and force-start

V16.1 showing `N=0, I=0, R=0` means no optical samples reached the application. The BPM/SpO2 algorithm therefore never ran.

V17:
- replays a minimal known-good MAX30102 measurement configuration when the FIFO stalls;
- reads back MODE, FIFO_CONFIG, SPO2_CONFIG, LED current, FIFO write and read pointers;
- shows those values on the LCD while the FIFO is empty.

Expected healthy empty-to-running transition:
`V17 FIFO M:3 ... F:5f S:27 L:3c` then `V17 PF N:... I:... R:...`.

Interpretation:
- `M` is MODE_CONFIG; expected `3`.
- `W` is FIFO write pointer; it must change.
- `R` is FIFO read pointer.
- `F` is FIFO_CONFIG; expected `5f`.
- `S` is SPO2_CONFIG; expected `27`.
- `L` is LED current; normally `3c`.

If M/F/S/L are correct but W remains 0 after repeated force-start attempts, the MAX30102 digital interface is alive but its conversion/FIFO engine is not producing data. That points to module power/AFE/oscillator failure rather than a finger-detection or BPM algorithm problem.
