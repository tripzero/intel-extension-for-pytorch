#include <ATen/ATen.h>
#include <ATen/AtenIpexTypeXPU.h>
#include <ATen/native/TensorIterator.h>

#include <utils/DPCPP.h>
#include "comm/Numerics.h"
#include "comm/Pairwise.h"
#include "comm/Pointwise.h"

#include <oneDNN/oneDNN.h>
#include "Loops.h"

using namespace xpu::dpcpp;

namespace at {
namespace AtenIpexTypeXPU {

IPEX_OUT_FLOAT_UNARY_FUNC_OPS(rsqrt_out, Numerics<scalar_t>::rsqrt, Real);
IPEX_OUT_FLOAT_UNARY_FUNC_OPS(sqrt, Numerics<scalar_t>::sqrt, Real);

Tensor& sqrt_out(Tensor& result, const Tensor& self) {
  checkBackend("sqrt_out", {self}, Backend::XPU);
  TORCH_CHECK(
      at::isFloatingType(self.scalar_type()),
      "unsupported dtype for self:",
      self.scalar_type());
  result = at::empty_like(self);
  if (self.dim() > 0 && self.scalar_type() != ScalarType::Double) {
    xpu::oneDNN::eltwise<dnnl::algorithm::eltwise_sqrt>(
        result, self, 0.0f, 0.0f);
  } else {
    sqrt(self, result);
  }
  return result;
}

} // namespace AtenIpexTypeXPU
} // namespace at
