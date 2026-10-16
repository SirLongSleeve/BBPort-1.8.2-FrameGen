# BBPort 1.8.2 Frame Generation

This tree is [BBPort `windows-v1.8.2`](https://github.com/Supermedo/bloodborne_pc) plus DLSS-G / FSR 3.1 frame generation (IFreemz Streamline path).

The matching overlay pack (prebuilt `bb-probe.exe` and Streamline DLLs) is `BBPort-1.8.2-FrameGen`. Copy that onto a BBPort 1.8.2 install. Do not copy it onto a live `BloodborneClean` tree unless you mean to.

## What changed

- `gpu/frame_gen/` — Streamline DLSS-G and FSR 3.1 FG
- Vulkan instance / swapchain / presenter / upscaler hooks
- Flip tags (`CurrentGameFrame` / `SetPresentedGameFrame`) so gameplay presents generate
- Insert menu: Frame generation, backend, multiplier, Reflex
- HUD while generating: `90 FPS -> 181 shown DLSS FrameGen On`

1.8.2-only behaviour is kept: DLSS device-extension retry, ultrawide `hud_position`, `quit_to_desktop`, `TaaDepthParameters`, 0x0 minimized-window skip, EOP flip-label wait.

## Build (Windows, MSYS2 CLANG64)

1. Install MSYS2 with the CLANG64 toolchain (see `packaging/windows/README.md`).
2. If MSYS2 is not at `C:\msys64`, set `MSYS2_ROOT` (this machine uses `/d/msys64`).
3. Put Streamline **2.14.1** headers in `streamline/include/` (already in this snapshot).
4. Optional: put `sl.*.dll`, `nvngx_dlssg.dll`, `nvngx_dlss.dll`, `NvLowLatencyVk.dll`, `amd_fidelityfx_vk.dll` in `runtime/` so CMake copies them next to `bb-probe.exe`. Those files also ship in the overlay pack.
5. From this directory:

```
export MSYS2_ROOT=/d/msys64   # if needed
./build.sh
```

Output: `out/bb-probe.exe` (RelWithDebInfo, LTO off on Windows).

Do not mix Streamline 2.14.1 headers with a much newer `sl.interposer.dll`. Hardware-accelerated GPU scheduling is required for DLSS-G. Uncap the launcher frame limit while FG is on. First enable needs a full restart so Streamline can load at `vkCreateInstance`.

## Layout

| Path | Role |
| --- | --- |
| `gpu/frame_gen/` | FG sources (outside the video_core glob) |
| `gpu/third_party/ffx-api/` | AMD FFX API headers for FSR FG |
| `streamline/include/` | NVIDIA Streamline 2.14.1 headers |
| `runtime/` | Optional FG DLLs + licenses |

Third-party trees under `gpu/third_party/` (imgui, fsr-vulkan, magic_enum, miniz, xbyak) and `third_party/LibAtrac9` are vendored in this snapshot so a clone builds without `git submodule update`. `.gitmodules` records the upstream URLs.

## Licenses

- BBPort / shadPS4 GPU code: GPL-2.0 (`LICENSE`)
- Streamline headers: NVIDIA MIT (`streamline/license.txt`)
- FidelityFX / FSR FG: MIT (`runtime/LICENSES/`)
- DLSS runtime redistributables: NVIDIA DLSS SDK (overlay pack)

No game files are included. You need your own decrypted Bloodborne dump (CUSA03173 1.09).
