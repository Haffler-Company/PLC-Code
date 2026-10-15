# Build, Version, And Rollback

## Versioning

- Canonical version file: `VERSION` (semver `MAJOR.MINOR.PATCH`, UTF-8)
- Firmware header: `GasFilling_HMI/version.h` (auto-written; label `vX.Y.Z`)
- Splash label uses `FW_VERSION_LABEL`
- Serial `PING` / `STAT` include the version string

Each `program.ps1` run bumps the **patch** by default and archives binaries under `artifacts/builds/vX.Y.Z/`.

## Technical knowledge (mandatory after upload)

```powershell
.\scripts\program.ps1 -Port COM3 -Knowledge "what you learned this session"
.\scripts\upload.ps1 -Port COM3 -SkipBuild -Knowledge none
```

`-Knowledge` is required. Use `none` when nothing new was learned. Text appends to `docs/GAS_FILLING_TECHNICAL_KNOWLEDGE.md`.

## Fast workflow

```powershell
.\scripts\doctor.ps1
.\scripts\program.ps1 -Port COM3 -Knowledge "..."
.\scripts\program.ps1 -Port COM3 -Force -Knowledge "sketch-only rebuild"
.\scripts\program.ps1 -Port COM3 -NoBump -Knowledge none
.\scripts\build.ps1
.\scripts\build.ps1 -SkipIfUnchanged
.\scripts\upload.ps1 -Port COM3 -SkipBuild -Knowledge none
.\scripts\list-versions.ps1
.\scripts\rollback.ps1 -Version 1.2.1 -Port COM3
.\scripts\rollback.ps1 -Version 1.2.1 -Port COM3 -RestoreSource
```

## Speed / reliability rules

- Fixed `--build-path artifacts/build-cache` + `--jobs 0`.
- Upload uses `--input-dir` (never recompiles during flash).
- **`-Force` / `-CleanSketch`**: clear sketch objects only; keep `core/` + `libraries/` (fast).
- **`-Clean`**: full cache wipe (slow cold rebuild). Use only if corrupt.
- Compile failure auto-retries once after CleanSketch.
- Upload retries up to 3 times with COM re-detect (USB-CDC).
- Scripts wait for a free port; close serial monitors / `hmi_agent` before flash.
- Fingerprint skip: `build.ps1 -SkipIfUnchanged` when sources unchanged.
- Archives default to binaries only; add `-ArchiveSource` for `tar.gz` of sketch.
- Last 15 archives kept. Build lock prevents concurrent compiles.
- Always use `%LOCALAPPDATA%\Arduino15\arduino-cli.yaml` (never a yaml pointing at another Windows user).

## Environment

- Optional: `$env:GASFILLING_PORT = "COM3"`
