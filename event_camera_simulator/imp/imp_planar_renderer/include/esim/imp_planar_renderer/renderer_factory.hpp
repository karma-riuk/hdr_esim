#pragma once

#include <esim/rendering/renderer_base.hpp>
#include <gflags/gflags.h>

namespace event_camera_simulator {

    bool loadPreprocessedImage(const std::string& path_to_img, cv::Mat* img);
    Renderer::Ptr loadRendererFromGflags();

} // namespace event_camera_simulator
