# FrameMonitor

**Video-based Respiration Monitoring of Newborn Babies** — a Windows desktop application
implementing the method presented at the
[Ninth Hungarian Conference on Computer Graphics and Geometry (HCCGG'18), Budapest, 2018](https://eprints.sztaki.hu/9608/1/Morse_209_30380005_ny.pdf).

> **Repository scope:** This repository contains **Gregory Morse's implementation only**.
> The paper was co-authored with Dmitry Chetverikov and Daniel Egyed, whose independent
> implementations were developed separately and have not been publicly released.

## Authors

The paper was co-authored by:

| Name | Affiliation | Contact |
|------|-------------|---------|
| Gregory Morse | Eötvös Loránd University (ELTE), Budapest | gregory.morse@live.com |
| Dmitry Chetverikov | Institute for Computer Science and Control (SZTAKI), Budapest | csetverikov@sztaki.hu |
| Daniel Egyed | ELTE / SZTAKI | — |

Code in this repository was written by Gregory Morse.

## Overview

Premature babies (born before 37 weeks) require continuous respiratory monitoring.
Traditional wired contact sensors (similar to EEG/polysomnographic devices) are expensive,
cause motion artifacts, and are stressful for babies and parents.
This project explores **non-contact, video-based respiratory rate (RR) estimation**
directly from a camera observing the baby in an incubator.

The paper describes two complementary approaches. This repository implements the **fusion
of three signals** method:

- **Fusion of three signals** (Morse, implemented here) — combines KLT optical-flow
  tracking, MOG2 background-subtraction motion detection, and brightness monitoring over
  an elliptic ROI; normalises and smooths each signal, fuses them, and counts breaths from
  peaks and troughs of the combined signal.
- **KLT feature-point tracking** (Egyed) — selects and tracks feature points on the baby,
  identifies the reference point with maximal horizontal displacement, and counts peaks in
  the resulting displacement signal. *This method was developed separately by Daniel Egyed
  and is not included in this repository.*

> **Note:** Newborn respiration is irregular (30–40 breaths/min) with frequent pauses,
> making classical FFT-based methods unreliable. These methods operate in the time domain.

## Platform

This is a **Windows-only** application built with
[Microsoft Foundation Classes (MFC)](https://learn.microsoft.com/en-us/cpp/mfc/mfc-desktop-applications).
The primary build system is **MSBuild** (via the included `.sln`/`.vcxproj`).
A `CMakeLists.txt` is also provided as an alternative build path.

## Dependencies

| Dependency | Purpose | Managed by |
|------------|---------|------------|
| [OpenCV 4](https://opencv.org/) + contrib | Video I/O, KLT, MOG2, xfeatures2d | vcpkg |
| MFC (Dynamic) | Windows GUI | Visual Studio workload |
| DirectShow (`strmiids`) | Camera device enumeration | Windows SDK |

## Getting Started

### Prerequisites

- **Windows 10/11**
- **[Visual Studio 2019 or later](https://visualstudio.microsoft.com/)** with the
  *Desktop development with C++* workload **and** the *MFC* optional component installed
- **[vcpkg](https://github.com/microsoft/vcpkg)** with the `VCPKG_ROOT` environment variable set

### 1 — Clone

```powershell
git clone https://github.com/GregoryMorse/FrameMonitor.git
cd FrameMonitor
```

### 2 — Install vcpkg (first time only)

```powershell
git clone https://github.com/microsoft/vcpkg.git $env:VCPKG_ROOT
& "$env:VCPKG_ROOT\bootstrap-vcpkg.bat"
vcpkg integrate install   # global MSBuild / Visual Studio integration
```

> Set `VCPKG_ROOT` as a permanent user environment variable so Visual Studio picks it up
> automatically on every build.

### 3a — Build with Visual Studio (recommended)

Open `FrameRepeat.sln`.
On the first build Visual Studio will invoke vcpkg to install OpenCV + contrib into
`vcpkg_installed\` automatically (vcpkg manifest mode is enabled in the project).
Select the **x64 | Debug** or **x64 | Release** configuration and press **Build**.

### 3b — Build with MSBuild from the command line

```powershell
msbuild FrameRepeat.sln /p:Configuration=Release /p:Platform=x64
```

### 3c — Build with CMake (alternative)

Requires CMake ≥ 3.17 and Ninja (`winget install Ninja-build.Ninja`).

```powershell
cmake --preset windows-x64-release
cmake --build --preset windows-x64-release
```

Presets are defined in `CMakePresets.json`.
To open as a CMake project in Visual Studio use **File → Open → Folder** instead of
opening the `.sln`.

### 4 — Configure the video source

On first launch the application looks for a file named `framemonitor.cfg` in the
working directory and, if not found there, in the same directory as the executable.

Copy the supplied template and edit it:

```powershell
Copy-Item framemonitor.cfg.example framemonitor.cfg
notepad framemonitor.cfg
```

Then uncomment **one** entry:

| Entry | Meaning |
|-------|---------|
| `video=C:\path\to\recording.mts` | Process a video file |
| `camera=0` | Capture from a live camera (index 0 = first device) |

If no active entry is found, a **file-open dialog** appears automatically on startup
so you can select a video interactively.
The application exits cleanly if the dialog is cancelled.

> `framemonitor.cfg` is excluded from version control (see `.gitignore`) because it
> contains machine-specific paths.  Commit `framemonitor.cfg.example` instead.

## Test Data

The algorithms were developed and validated on clinical video recordings of premature
neonates in incubators, captured at the collaborating hospital during the study
described in the citation below.

**These recordings are not included in this repository** for the following reasons:

- The infants depicted are identifiable patients; sharing the footage without
  explicit ethics-board approval and informed-consent documentation would be
  inappropriate regardless of any research exemption.
- The recordings may be subject to local data-protection legislation (e.g.,
  Hungarian Act XLVII of 1997 on the Protection of Health and Related Personal Data).

**Using your own video:**  
Any video file that OpenCV can open (`.mts`, `.mp4`, `.avi`, `.mkv`, `.mov`, …) and
that shows a neonate or similar subject with visible respiratory chest motion should
work as a drop-in replacement.  A static camera angle with good contrast between the
subject and background gives the best results.

**Requesting the original test data:**  
Researchers who wish to replicate the published results and can demonstrate
appropriate institutional ethics approval should contact the corresponding author
(Gregory Morse) via the paper URL listed in the Citation section.

## Citation

If you use this code in academic work, please cite the original paper:

```bibtex
@inproceedings{morse2018videobased,
  title     = {Video-based Respiration Monitoring of Newborn Babies: a Feasibility Study},
  author    = {Morse, Gregory and Chetverikov, Dmitry and Egyed, Daniel},
  booktitle = {Proceedings of the Ninth Hungarian Conference on Computer Graphics and Geometry (HCCGG'18)},
  year      = {2018},
  address   = {Budapest, Hungary},
  url       = {https://eprints.sztaki.hu/9608/1/Morse_209_30380005_ny.pdf}
}
```

## License

This project is licensed under the MIT License — see [LICENSE](LICENSE) for details.