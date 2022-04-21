#include <ATen/ATen.h>
#include <ATen/native/TensorIterator.h>

#include <utils/DPCPP.h>
#include "comm/ATDispatch.h"
#include "comm/Numerics.h"
#include "comm/Pairwise.h"
#include "comm/Pointwise.h"

#include "Loops.h"

using namespace xpu::dpcpp;

namespace at {
namespace AtenIpexTypeXPU {

IPEX_OUT_FLOAT_UNARY_FUNC_OPS(expm1_out, Numerics<scalar_t>::expm1, Real);

Tensor& exp_out(const Tensor& self, Tensor& out) {
  auto iter = TensorIterator::unary_float_op(out, self);
  IPEX_DISPATCH_FLOATING_AND_COMPLEX_TYPES_AND2(
      ScalarType::Half,
      ScalarType::BFloat16,
      iter.common_dtype(),
      "exp",
      [&]() {
        dpcpp_kernel_for_tensor_iter(iter, [](scalar_t a) -> scalar_t {
          return Numerics<scalar_t>::exp(a);
        });
      });
  return out;
}

} // namespace AtenIpexTypeXPU
} // namespace at
