IFreemz shadPS4-Bloodborne-DLSS-FSR frame-gen sources, ported to BBPort 1.8.2.

vk_bb_frame_gen.*     DLSS-G via NVIDIA Streamline 2.14.1
vk_bb_fsr_frame_gen.* FSR 3.1 FG via amd_fidelityfx_vk.dll

Compiled into bbgpu on Windows (gpu/CMakeLists.txt) when Streamline 2.14.1
headers are at streamline/include (next to gpu/). Runtime DLLs copy next to
bb-probe.exe from runtime/ when present.

When generation actually starts, the overlay shows a green
"Frame generation on (DLSS/FSR)" banner for five seconds (even if the FPS
counter is off). While it stays generating, the FPS counter turns green:
"X FPS -> Y shown DLSS FrameGen On".
