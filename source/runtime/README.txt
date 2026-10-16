Optional FG runtime DLLs for a local build (CMake POST_BUILD copies them next
to bb-probe.exe when sl.interposer.dll is here):

  sl.interposer.dll
  sl.common.dll
  sl.dlss_g.dll
  sl.pcl.dll
  sl.reflex.dll
  nvngx_dlssg.dll
  nvngx_dlss.dll
  NvLowLatencyVk.dll
  amd_fidelityfx_vk.dll

They are not stored in this source snapshot (large NVIDIA/AMD redistributables).
Copy them from the BBPort-1.8.2-FrameGen overlay pack, or from NVIDIA Streamline
SDK 2.14.1 bin/x64 plus FidelityFX SDK PrebuiltSignedDLL.

Use Streamline 2.14.1 with the headers in ../streamline/include. Do not mix
those headers with a much newer sl.interposer.dll.
