# SpO2 v15 – IR-only finger detection and partial result display

Changes:
- Finger detection now depends on IR only. A weak Red channel no longer blocks BPM acquisition.
- BPM and SpO2 validity are independent. A valid BPM is shown while SpO2 is still waiting for a usable Red waveform.
- Added peak-interval fallback when autocorrelation cannot resolve BPM.
- LCD shows compact raw I (IR) and R (Red) values for physical diagnosis.
- No fabricated SpO2 value: SpO2 remains `--` when the Red signal is not usable.
