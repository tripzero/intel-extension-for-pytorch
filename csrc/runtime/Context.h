#pragma once

#include <runtime/DPCPPUtils.h>
#include <runtime/Device.h>

namespace xpu {
namespace dpcpp {

void clearDeviceContext();

DPCPP::context getDeviceContext(DeviceId device_id = 0);

} // namespace dpcpp
} // namespace xpu