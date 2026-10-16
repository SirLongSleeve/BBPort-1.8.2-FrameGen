BBPort 1.8.2 FrameGen overlay
=============================

Source for this build (GitHub):
  ..\BBPort-1.8.2-FrameGen-Source
  See FRAMEGEN.md in that folder.

Copy the contents of this folder on top of a BBPort 1.8.2 install
(the folder that has Play Bloodborne.exe, bin\, and bbport.ini).

What this replaces
------------------
- bin\bb-probe.exe          FrameGen native probe (Streamline DLSS-G / FSR 3.1 FG)
- bin\bb-probe-native.exe   same binary (for dual-mode with a BBSC stub as bb-probe.exe)
- bin\libc++.dll            clang 23 libc++ (needed for this probe)
- bin\libunwind.dll
- bin\libwinpthread-1.dll
- bin\sl.interposer.dll, sl.common.dll, sl.dlss_g.dll, sl.pcl.dll, sl.reflex.dll
- bin\nvngx_dlssg.dll, nvngx_dlss.dll, NvLowLatencyVk.dll
- bin\amd_fidelityfx_vk.dll
- bbport.ini                stock 1.8.2 keys plus frame_gen=1 (merge the four FG lines
                            if you want to keep your existing ini)
- PLAY BBPORT FG.cmd        launches Play Bloodborne.exe after you overlay this folder

Enable
------
Insert / L3+R3 → Frame generation. First enable needs a full restart so
Streamline can load at Vulkan instance create. Uncap the launcher frame
limit while FG is on. Hardware-accelerated GPU scheduling is required
for DLSS-G.

HUD
---
Green "90 FPS -> 181 shown DLSS FrameGen On" means generated frames
are reaching the display. Yellow "FG idle" is expected on the title
and menus (no G-buffer camera). White "X FPS Y ms DLSS" means FG is off.

Do not mix this probe with the stock 1.8.2 17 MB bb-probe.exe.
Leave F:\BloodborneClean untouched unless you copy onto a dedicated copy.
