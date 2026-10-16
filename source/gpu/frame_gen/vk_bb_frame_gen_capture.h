// SPDX-FileCopyrightText: Copyright 2026 IFreemz / BBPortFG
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <optional>

#include "vk_bb_frame_gen.h"

namespace Vulkan {

class Instance;
class CameraMotion;

namespace FrameGen {

// Copies this frame's depth and motion into host-owned images that stay valid until present,
// and fills Streamline/FFX camera matrices from CameraMotion.
void Capture(const Instance& instance, vk::CommandBuffer command, vk::Image depth,
             vk::Format depth_format, vk::Image motion, vk::Extent2D render,
             const CameraMotion& camera, bool reset, std::array<float, 2> jitter);

// Latest capture, if any. Presenter takes it once per presented game frame.
std::optional<FrameInputs> Take();

void ShutdownCapture();

} // namespace FrameGen
} // namespace Vulkan
