// SPDX-FileCopyrightText: Copyright 2026 IFreemz / BBPortFG
// SPDX-License-Identifier: GPL-2.0-or-later

#include "vk_bb_frame_gen_capture.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstring>
#include <memory>
#include <mutex>

#include "common/logging/log.h"
#include "video_core/renderer_vulkan/vk_camera_motion.h"
#include "video_core/renderer_vulkan/vk_instance.h"
#include "video_core/renderer_vulkan/vk_platform.h"
#include "video_core/texture_cache/image.h"

namespace Vulkan::FrameGen {

namespace {

using Mat = std::array<double, 16>;

constexpr Mat Identity{1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

Mat Multiply(const Mat& a, const Mat& b) {
    Mat r{};
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            for (int k = 0; k < 4; ++k) {
                r[i * 4 + j] += a[i * 4 + k] * b[k * 4 + j];
            }
        }
    }
    return r;
}

Mat Invert(Mat m) {
    Mat r = Identity;
    for (int c = 0; c < 4; ++c) {
        int pivot = c;
        for (int i = c + 1; i < 4; ++i) {
            if (std::abs(m[i * 4 + c]) > std::abs(m[pivot * 4 + c])) {
                pivot = i;
            }
        }
        for (int j = 0; j < 4; ++j) {
            std::swap(m[c * 4 + j], m[pivot * 4 + j]);
            std::swap(r[c * 4 + j], r[pivot * 4 + j]);
        }
        const double d = m[c * 4 + c];
        if (d == 0.0) {
            return Identity;
        }
        for (int j = 0; j < 4; ++j) {
            m[c * 4 + j] /= d;
            r[c * 4 + j] /= d;
        }
        for (int i = 0; i < 4; ++i) {
            if (i == c) {
                continue;
            }
            const double f = m[i * 4 + c];
            for (int j = 0; j < 4; ++j) {
                m[i * 4 + j] -= f * m[c * 4 + j];
                r[i * 4 + j] -= f * r[c * 4 + j];
            }
        }
    }
    return r;
}

// Streamline wants row-vector matrices: the transposes of the column-vector ones.
std::array<float, 16> Out(const Mat& m) {
    std::array<float, 16> r{};
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            r[i * 4 + j] = float(m[j * 4 + i]);
        }
    }
    return r;
}

Mat Projection(const std::array<float, 4>& p) {
    return Mat{p[0], 0, 0, 0, 0, p[1], 0, 0, 0, 0, p[2], p[3], 0, 0, 1, 0};
}

bool DepthFormat(vk::Format format) {
    switch (format) {
    case vk::Format::eD16Unorm:
    case vk::Format::eX8D24UnormPack32:
    case vk::Format::eD32Sfloat:
    case vk::Format::eD16UnormS8Uint:
    case vk::Format::eD24UnormS8Uint:
    case vk::Format::eD32SfloatS8Uint:
        return true;
    default:
        return false;
    }
}

vk::ImageAspectFlags DepthAspect(vk::Format format) {
    switch (format) {
    case vk::Format::eD16UnormS8Uint:
    case vk::Format::eD24UnormS8Uint:
    case vk::Format::eD32SfloatS8Uint:
        return vk::ImageAspectFlagBits::eDepth;
    default:
        return DepthFormat(format) ? vk::ImageAspectFlagBits::eDepth
                                   : vk::ImageAspectFlagBits::eColor;
    }
}

void Barrier(vk::CommandBuffer command, vk::Image image, vk::ImageAspectFlags aspect,
             vk::ImageLayout old_layout, vk::PipelineStageFlags2 src_stage,
             vk::AccessFlags2 src_access, vk::ImageLayout new_layout,
             vk::PipelineStageFlags2 dst_stage, vk::AccessFlags2 dst_access) {
    const vk::ImageMemoryBarrier2 barrier{
        .srcStageMask = src_stage,
        .srcAccessMask = src_access,
        .dstStageMask = dst_stage,
        .dstAccessMask = dst_access,
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .image = image,
        .subresourceRange = {aspect, 0, 1, 0, 1},
    };
    command.pipelineBarrier2({.imageMemoryBarrierCount = 1, .pImageMemoryBarriers = &barrier});
}

struct Slot {
    VideoCore::UniqueImage depth;
    VideoCore::UniqueImage motion;
    vk::UniqueImageView depth_view;
    vk::UniqueImageView motion_view;
    vk::Format depth_format{};
    vk::Extent2D extent{};
};

struct Owner {
    Slot slot;
};

std::mutex mutex;
std::array<std::shared_ptr<Owner>, 3> ring;
u32 ring_index{};
std::optional<FrameInputs> pending;

} // namespace

void Capture(const Instance& instance, vk::CommandBuffer command, vk::Image depth,
             vk::Format depth_format, vk::Image motion, vk::Extent2D render,
             const CameraMotion& camera, bool reset, std::array<float, 2> jitter) {
    if (!Active() || !camera.Ready() || !depth || !motion || render.width == 0 ||
        render.height == 0) {
        if (Active()) {
            static auto last = std::chrono::steady_clock::time_point{};
            const auto now = std::chrono::steady_clock::now();
            if (now - last > std::chrono::seconds{2}) {
                last = now;
                LOG_WARNING(Render_Vulkan,
                            "[FRAME-GEN] Capture skipped: camera={} depth={} motion={} {}x{}",
                            camera.Ready(), bool(depth), bool(motion), render.width,
                            render.height);
            }
        }
        return;
    }
    const auto& current = camera.CurrentCamera();
    const auto& previous = camera.PreviousCamera();
    if (!current.valid || !previous.valid) {
        return;
    }

    auto owner = [&] {
        std::scoped_lock lock{mutex};
        auto& slot = ring[ring_index];
        ring_index = (ring_index + 1) % ring.size();
        if (!slot || slot.use_count() != 1) {
            slot = std::make_shared<Owner>();
        }
        return slot;
    }();
    auto& s = owner->slot;
    const auto device = instance.GetDevice();
    const auto allocator = instance.GetAllocator();
    using U = vk::ImageUsageFlagBits;
    if (!s.depth || s.extent != render || s.depth_format != depth_format) {
        s.depth_view.reset();
        s.motion_view.reset();
        s.depth = VideoCore::UniqueImage(device, allocator);
        s.depth.Create(vk::ImageCreateInfo{
            .imageType = vk::ImageType::e2D,
            .format = depth_format,
            .extent = {render.width, render.height, 1},
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = vk::SampleCountFlagBits::e1,
            .tiling = vk::ImageTiling::eOptimal,
            .usage = U::eSampled | U::eTransferDst | U::eTransferSrc,
            .initialLayout = vk::ImageLayout::eUndefined,
        });
        s.motion = VideoCore::UniqueImage(device, allocator);
        s.motion.Create(vk::ImageCreateInfo{
            .imageType = vk::ImageType::e2D,
            .format = vk::Format::eR16G16Sfloat,
            .extent = {render.width, render.height, 1},
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = vk::SampleCountFlagBits::e1,
            .tiling = vk::ImageTiling::eOptimal,
            .usage = U::eSampled | U::eTransferDst | U::eTransferSrc,
            .initialLayout = vk::ImageLayout::eUndefined,
        });
        const auto depth_aspect = DepthAspect(depth_format);
        s.depth_view = Check(device.createImageViewUnique({
            .image = vk::Image(s.depth),
            .viewType = vk::ImageViewType::e2D,
            .format = depth_format,
            .subresourceRange = {depth_aspect, 0, 1, 0, 1},
        }));
        s.motion_view = Check(device.createImageViewUnique({
            .image = vk::Image(s.motion),
            .viewType = vk::ImageViewType::e2D,
            .format = vk::Format::eR16G16Sfloat,
            .subresourceRange = {vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1},
        }));
        s.depth_format = depth_format;
        s.extent = render;
    }

    const auto depth_aspect = DepthAspect(depth_format);
    const auto all = vk::PipelineStageFlagBits2::eAllCommands;
    Barrier(command, depth, depth_aspect, vk::ImageLayout::eGeneral, all,
            vk::AccessFlagBits2::eShaderRead, vk::ImageLayout::eTransferSrcOptimal,
            vk::PipelineStageFlagBits2::eCopy, vk::AccessFlagBits2::eTransferRead);
    Barrier(command, motion, vk::ImageAspectFlagBits::eColor, vk::ImageLayout::eGeneral, all,
            vk::AccessFlagBits2::eShaderRead, vk::ImageLayout::eTransferSrcOptimal,
            vk::PipelineStageFlagBits2::eCopy, vk::AccessFlagBits2::eTransferRead);
    Barrier(command, vk::Image(s.depth), depth_aspect, vk::ImageLayout::eUndefined, all,
            vk::AccessFlagBits2::eNone, vk::ImageLayout::eTransferDstOptimal,
            vk::PipelineStageFlagBits2::eCopy, vk::AccessFlagBits2::eTransferWrite);
    Barrier(command, vk::Image(s.motion), vk::ImageAspectFlagBits::eColor,
            vk::ImageLayout::eUndefined, all, vk::AccessFlagBits2::eNone,
            vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits2::eCopy,
            vk::AccessFlagBits2::eTransferWrite);
    const vk::ImageCopy depth_region{
        .srcSubresource = {.aspectMask = depth_aspect, .layerCount = 1},
        .dstSubresource = {.aspectMask = depth_aspect, .layerCount = 1},
        .extent = {render.width, render.height, 1},
    };
    const vk::ImageCopy motion_region{
        .srcSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor, .layerCount = 1},
        .dstSubresource = {.aspectMask = vk::ImageAspectFlagBits::eColor, .layerCount = 1},
        .extent = {render.width, render.height, 1},
    };
    command.copyImage(depth, vk::ImageLayout::eTransferSrcOptimal, vk::Image(s.depth),
                      vk::ImageLayout::eTransferDstOptimal, depth_region);
    command.copyImage(motion, vk::ImageLayout::eTransferSrcOptimal, vk::Image(s.motion),
                      vk::ImageLayout::eTransferDstOptimal, motion_region);
    Barrier(command, vk::Image(s.depth), depth_aspect, vk::ImageLayout::eTransferDstOptimal,
            vk::PipelineStageFlagBits2::eCopy, vk::AccessFlagBits2::eTransferWrite,
            vk::ImageLayout::eShaderReadOnlyOptimal, all, vk::AccessFlagBits2::eShaderRead);
    Barrier(command, vk::Image(s.motion), vk::ImageAspectFlagBits::eColor,
            vk::ImageLayout::eTransferDstOptimal, vk::PipelineStageFlagBits2::eCopy,
            vk::AccessFlagBits2::eTransferWrite, vk::ImageLayout::eShaderReadOnlyOptimal, all,
            vk::AccessFlagBits2::eShaderRead);
    Barrier(command, depth, depth_aspect, vk::ImageLayout::eTransferSrcOptimal,
            vk::PipelineStageFlagBits2::eCopy, vk::AccessFlagBits2::eTransferRead,
            vk::ImageLayout::eGeneral, all, vk::AccessFlagBits2::eShaderRead);
    Barrier(command, motion, vk::ImageAspectFlagBits::eColor, vk::ImageLayout::eTransferSrcOptimal,
            vk::PipelineStageFlagBits2::eCopy, vk::AccessFlagBits2::eTransferRead,
            vk::ImageLayout::eGeneral, all, vk::AccessFlagBits2::eShaderRead);

    Mat view_to_prev{};
    const auto& a = previous.view;
    const auto& b = current.inv_view;
    for (int r = 0; r < 3; ++r) {
        for (int col = 0; col < 4; ++col) {
            double value = a[r * 4 + 0] * b[0 * 4 + col] + a[r * 4 + 1] * b[1 * 4 + col] +
                           a[r * 4 + 2] * b[2 * 4 + col];
            if (col == 3) {
                value += a[r * 4 + 3];
            }
            view_to_prev[r * 4 + col] = value;
        }
    }
    view_to_prev[15] = 1.0;
    const Mat view_to_clip = Projection(current.proj);
    const Mat clip_to_view = Invert(view_to_clip);
    const Mat clip_to_prev_clip =
        Multiply(Multiply(Projection(previous.proj), view_to_prev), clip_to_view);

    FrameInputs inputs{};
    inputs.resources = owner;
    inputs.depth = vk::Image(s.depth);
    inputs.depth_view = *s.depth_view;
    inputs.motion = vk::Image(s.motion);
    inputs.motion_view = *s.motion_view;
    inputs.depth_format = depth_format;
    inputs.render = render;
    inputs.view_to_clip = Out(view_to_clip);
    inputs.clip_to_view = Out(clip_to_view);
    inputs.clip_to_prev_clip = Out(clip_to_prev_clip);
    inputs.prev_clip_to_clip = Out(Invert(clip_to_prev_clip));
    for (int i = 0; i < 3; ++i) {
        inputs.right[i] = b[i * 4 + 0];
        inputs.up[i] = b[i * 4 + 1];
        inputs.forward[i] = b[i * 4 + 2];
        inputs.position[i] = b[i * 4 + 3];
    }
    inputs.near_plane = camera.Near();
    inputs.far_plane = 3000.0f;
    inputs.fov_y = camera.VerticalFov();
    inputs.aspect = current.proj[1] != 0.0f ? current.proj[1] / current.proj[0] : 16.0f / 9.0f;
    inputs.jitter = jitter;
    inputs.depth_inverted = false;
    inputs.reset = reset;

    std::scoped_lock lock{mutex};
    pending = std::move(inputs);
}

std::optional<FrameInputs> Take() {
    std::scoped_lock lock{mutex};
    auto taken = std::move(pending);
    pending.reset();
    return taken;
}

void ShutdownCapture() {
    std::scoped_lock lock{mutex};
    pending.reset();
    for (auto& slot : ring) {
        slot.reset();
    }
}

} // namespace Vulkan::FrameGen
