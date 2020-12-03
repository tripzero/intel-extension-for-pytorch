#include <ATen/ATen.h>
#include <ATen/Config.h>
#include <ATen/NativeFunctions.h>

#include <core/DPCPPUtils.h>
#include <core/Runtime.h>

#include <utils/ParamUtils.h>

#include <ATen/aten_ipex_type_dpcpp.h>

using namespace dnnl;
using namespace at::dpcpp;
using namespace at::native;

namespace at {
namespace AtenIpexTypeDPCPP {

using namespace impl;

at::Tensor quantized_max_pool2d(
    const Tensor& qx,
    IntArrayRef kernel_size,
    IntArrayRef stride,
    IntArrayRef padding,
    IntArrayRef dilation,
    bool ceil_mode) {
  auto output_and_indices = at::max_pool2d_with_indices(
      qx, kernel_size, stride, padding, dilation, ceil_mode);
  auto output = std::get<0>(output_and_indices);

  return output;
}

} // namespace AtenIpexTypeDPCPP
} // namespace at