# BBPort 1.8.2 FrameGen

[BBPort `windows-v1.8.2`](https://github.com/Supermedo/bloodborne_pc) plus DLSS-G / FSR 3.1 frame generation (Streamline interposer path).

| Folder | What it is |
| --- | --- |
| `source/` | Buildable source snapshot (vendored third_party, Streamline 2.14.1 headers). See `source/FRAMEGEN.md`. |
| `release/` | Overlay pack: copy onto a BBPort 1.8.2 install (the folder with `Play Bloodborne.exe` and `bin\`). |

No game files are included. You need your own decrypted Bloodborne dump (CUSA03173 1.09).

## Overlay install

1. Install stock BBPort 1.8.2.
2. Copy the contents of `release/` on top of that install.
3. Launch with `PLAY BBPORT FG.cmd`, or start `Play Bloodborne.exe` as usual.
4. Insert / L3+R3 â†’ Frame generation. First enable needs a full restart so Streamline can load at Vulkan instance create.
5. Uncap the launcher frame limit while FG is on. Hardware-accelerated GPU scheduling is required for DLSS-G.

HUD while generating: `90 FPS -> 181 shown DLSS FrameGen On`. Yellow `FG idle` is expected on the title and menus.

Do not mix this probe with the stock 1.8.2 ~17 MB `bb-probe.exe`.

## Build from source

See `source/FRAMEGEN.md`. Windows build uses MSYS2 CLANG64 and `./build.sh`. Streamline headers are in `source/streamline/include/`. Optional FG DLLs can sit in `source/runtime/` (they already ship in `release/bin/`).

Large binaries (`bb-probe.exe` / `bb-probe-native.exe`, `nvngx_dlss.dll`) are stored with Git LFS. Clone with Git LFS installed, or GitHub Desktop.

## Licenses

- BBPort / shadPS4 GPU code: GPL-2.0 (`LICENSE`)
- Streamline headers: NVIDIA MIT (`source/streamline/license.txt`)
- FidelityFX / FSR FG: MIT (`source/runtime/LICENSES/`)
- DLSS runtime redistributables: NVIDIA DLSS SDK (in the overlay pack)
