# AGENTS.md

This file guides coding agents working in the `nwinfo` repository.

## 1. Project Summary

- Project: `NWinfo` (Win32 hardware and system information tool).
- Primary outputs:
- `nwinfo.exe`: CLI.
- `gnwinfo.exe`: GUI.
- `libnw`, `libcdi`, `libcpuid`, `liblhm`: libraries and helper/test binaries.
- Platform and toolchain:
- Windows (Win32/x64/ARM64).
- Visual Studio 2022 + MSBuild (`v143`).
- C17/C++20, with `TreatWarningAsError=true` in most projects.

## 2. Repository Map

- `nwinfo.c`: CLI entry and argument parsing.
- `gnwinfo/`: GUI implementation (Nuklear + GDI+) and language resources.
- `libnw/`: core data collection, formatting, and public API.
- `libnw/cpu/`, `libnw/gpu/`, `libnw/sensor/`: hardware domain modules.
- `ioctl/`: low-level driver and hardware access layer.
- `docs/README.md`: CLI documentation source (also used to generate README PDF).
- `.github/workflows/msbuild.yml`: CI build and packaging workflow.

## 3. Build (Follow CI First)

0. Use Visual Studio 2022 Developer PowerShell when possible.
   If `msbuild` is not in `PATH`, resolve it first:

```powershell
$msbuild = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" `
  -latest -products * -requires Microsoft.Component.MSBuild `
  -find MSBuild\**\Bin\MSBuild.exe
```

1. Restore dependencies (CI step):

```powershell
nuget restore .
```

   If `nuget` is unavailable but `packages\` is already present, continue to build.

2. Minimum local verification build (validated):

```powershell
& $msbuild /m:1 /p:Configuration=Release /p:Platform=x64 nwinfo.sln
```

3. CI/release matrix builds:

```powershell
msbuild /m /p:Configuration=Release /p:Platform=x64 nwinfo.sln
msbuild /m /p:Configuration=Release /p:Platform=x86 nwinfo.sln
msbuild /m /p:Configuration=DLLRelease /p:Platform=x64 nwinfo.sln
msbuild /m /p:Configuration=DLLRelease /p:Platform=x86 nwinfo.sln
```

   Build from the solution entry only (`nwinfo.sln`). Do not compile child project files (`*.vcxproj`) directly.
   If `/m` behaves unreliably in restricted terminals, retry with `/m:1`.
   Forbidden example:

```powershell
msbuild libnw\libnw.vcxproj /p:Configuration=Release /p:Platform=x64
```

4. Packaging resources (when release flow is touched):

```powershell
.\copy_res.ps1 -TargetFolder PKG
```

## 4. Style and Change Constraints

- Keep SPDX header in new source files when applicable:
- `// SPDX-License-Identifier: Unlicense`
- Preserve local code style:
- return type and function name on separate lines.
- tab-based indentation in existing C/C++ files.
- Use `CRLF` line endings for both edited and newly created files.
- Never run compiled programs from this repository (for example `nwinfo.exe`, `gnwinfo.exe`, or other built binaries).
- Avoid over-defensive programming. For `static` functions, prioritize performance and do not add parameter checks.
- naming, macros, and error handling consistent with nearby code.
- Avoid unrelated refactors: no broad reformatting/reordering/renaming.
- Do not modify third-party folders (`nuklear/`, `stb/`, `libcpuid/`) unless required for the task; explain why if changed.

## 5. Coupled Changes

- If CLI options are added/changed, update at least:
- `nwinfo.c` (option table, parsing, help text).
- `docs/README.md` (CLI docs).
- `hw_report.ps1` if output shape or option semantics change.
- If public APIs change, update declarations and callsites consistently (for example `libnw/libnw.h`).
- If GUI text changes, check `gnwinfo/lang/*.h` for localization consistency.

## 6. Pre-Submit Checklist

- Only task-related files are changed.
- At least one impacted configuration is built (minimum: `x64 Release`).
- Do not commit generated artifacts or temp files (`x64/`, `Win32/`, `PKG/`, `report.json`, etc.).
- Final notes must state: what changed, why, and how it was checked.

## 7. Default Working Principles

- Prefer small, focused changes over broad rewrites.
- Prefer CI-consistent commands and paths.
- Call out potential hardware impact when touching low-level access paths.
