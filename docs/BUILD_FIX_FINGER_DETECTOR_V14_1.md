# Build fix: missing FingerDetector symbols (v14.1)

## Symptom

The linker reports undefined references to:

- `FingerDetector_Reset`
- `FingerDetector_Update`
- `FingerDetector_GetThresholdIR`
- `FingerDetector_GetSpanIR`
- `FingerDetector_GetMeanIR`

## Root cause

`Core/Src/finger_detector.c` existed in the package, but the STM32CubeIDE project
uses linked resources and the file was not listed in `STM32CubeIDE/.project`.
Therefore CubeIDE did not compile `finger_detector.c`, so no
`finger_detector.o` was placed in `objects.list`.

## Fix

The following linked resource has been added:

```xml
<link>
    <name>Application/User/finger_detector.c</name>
    <type>1</type>
    <locationURI>$%7BPARENT-1-PROJECT_LOC%7D/Core/Src/finger_detector.c</locationURI>
</link>
```

## Rebuild procedure

1. Import v14.1 into a new workspace location.
2. Delete the old `Debug` and `Release` folders if they exist.
3. In CubeIDE, use **Project > Clean**.
4. Build again.
5. Verify the console contains a compile command for `finger_detector.c` and
   `objects.list` contains `Application/User/finger_detector.o`.
