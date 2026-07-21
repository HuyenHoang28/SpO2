# MAX30102 stability fix v11

## Symptom

The display initially showed `PLACE FINGER`, then changed to `MAX30102 MISSING` after about three seconds. This means the device answered during initialization, but one later FIFO read failed and the old firmware immediately treated that transient I2C error as a physically removed sensor.

## Fixes

- A single FIFO read error no longer changes the status to `MISSING`.
- The address `0x57` must fail several consecutive probes before the UI reports `MAX30102 MISSING`.
- Register and FIFO transactions are retried up to three times.
- If the sensor still ACKs but FIFO stops, it is reconfigured in place and returns to `PLACE FINGER`.
- MAX30102 gets bus priority; RTC access is performed afterwards.
- RTC probing is delayed and rate-limited. A bad RTC now falls back to the build-time software clock instead of disturbing MAX30102.
- Finger threshold is reduced to 5000 for low-output breakout boards.

## Hardware isolation test

First disconnect Tiny RTC and test only MAX30102:

- PA8 -> SCL
- PC9 -> SDA
- 3V -> VCC/VIN
- GND -> GND
- INT not connected

If the sensor works alone but fails when RTC is connected, the RTC module or its pull-ups are disturbing the shared I2C bus. A DS1307 board powered at 5 V must not pull SDA/SCL to 5 V. Prefer DS3231 at 3.3 V or use a bidirectional level shifter.
