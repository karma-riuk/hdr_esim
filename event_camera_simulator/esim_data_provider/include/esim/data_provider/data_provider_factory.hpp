#pragma once

#include <esim/data_provider/data_provider_base.hpp>
#include <gflags/gflags.h>

namespace event_camera_simulator {

    DataProviderBase::Ptr loadDataProviderFromGflags();

} // namespace event_camera_simulator
