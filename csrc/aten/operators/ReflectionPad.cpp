#include <ATen/ATen.h>
#include <runtime/Utils.h>
#include <core/Memory.h>
#include <core/detail/IndexUtils.h>
#include "comm/Atomics.h"
#include "comm/Numerics.h"
#include "comm/ATDispatch.h"


using namespace xpu::dpcpp;

namespace at {
namespace AtenIpexTypeXPU {
namespace impl {

DPCPP_DEF_K2(ReflectionPad1d, typename scalar_t);
DPCPP_DEF_K2(ReflectionPad1dBackward, typename scalar_t);

inline std::pair<int64_t, int64_t> get_index_mapping1d(
    int64_t input_w,
    int64_t output_w,
    int64_t output_x,
    int64_t pad_l,
    const DPCPP::nd_item<3> item) {
  auto input_offset =
      (item.get_group(1) + item.get_group(2) * item.get_group_range(1)) *
      input_w;
  auto output_offset =
      (item.get_group(1) + item.get_group(2) * item.get_group_range(1)) *
      output_w;

  auto i_start_x = Max(int64_t(0), -pad_l);
  auto o_start_x = Max(int64_t(0), pad_l);

  int64_t input_x = Numerics<int64_t>::abs(output_x - pad_l) -
      Numerics<int64_t>::abs(output_x - (input_w + pad_l - 1)) - output_x +
      2 * pad_l + input_w - 1 - o_start_x + i_start_x;

  return std::make_pair<int64_t, int64_t>(
      input_offset + input_x, output_offset + output_x);
}

template <typename scalar_t>
void reflection_pad1d_out_kernel(
    scalar_t* input,
    scalar_t* output,
    int64_t input_w,
    int64_t pad_l,
    int64_t pad_r,
    int64_t nbatch,
    int64_t nplane,
    int64_t output_w) {
  auto& queue = dpcppGetCurrentQueue();
  int work_group_size = output_w > 256 ? 256 : output_w;
  int work_group_num = CeilDiv(output_w, (int64_t)256);

  auto cgf = DPCPP_Q_CGF(cgh) {
    auto input_data = input;
    auto output_data = output;
    auto kfn = DPCPP_Q_KFN(DPCPP::nd_item<3> item) {
      auto input_ptr = input_data;
      auto output_ptr = output_data;
      auto output_x = item.get_global_id(0);

      if (output_x < output_w) {
        // input index and output index mapping
        auto index_pair =
            get_index_mapping1d(input_w, output_w, output_x, pad_l, item);
        output_ptr[index_pair.second] = input_ptr[index_pair.first];
      }
    };
    cgh.parallel_for<DPCPP_K(ReflectionPad1d, scalar_t)>(
        DPCPP::nd_range<3>(
            DPCPP::range<3>(work_group_size * work_group_num, nplane, nbatch),
            DPCPP::range<3>(work_group_size, 1, 1)),
        kfn);
  };
  DPCPP_Q_ASYNC_SUBMIT(queue, cgf);
}

void reflection_pad1d_out_template(
    Tensor& output,
    const Tensor& input_,
    IntArrayRef padding) {
  TORCH_CHECK(
      xpu::dpcpp::detail::canUse32BitIndexMath(input_),
      "input tensor must fit into 32-bit index math");

  int64_t dim_plane = 0;
  int64_t dim_w = 1;
  int64_t nbatch = 1;

  TORCH_CHECK(
      input_.numel() > 0 &&
          (input_.ndimension() == 2 || input_.ndimension() == 3),
      "non-empty 2D "
      "or 3D (batch mode) tensor expected for input, but got: ",
      input_);

  if (input_.ndimension() == 3) {
    nbatch = input_.size(0);
    dim_plane++;
    dim_w++;
  }

  int64_t pad_l = padding[0];
  int64_t pad_r = padding[1];

  int64_t nplane = input_.size(dim_plane);
  int64_t input_w = input_.size(dim_w);
  int64_t output_w = input_w + pad_l + pad_r;

  TORCH_CHECK(
      pad_l < input_w && pad_r < input_w,
      "Padding size should be less "
      "than the corresponding input dimension, but got: padding (",
      pad_l,
      ", ",
      pad_r,
      ") at dimension ",
      dim_w,
      " of input ",
      input_);

  TORCH_CHECK(
      output_w >= 1,
      "input (W: ",
      input_w,
      ")is too small. Calculated output W: ",
      output_w);

  if (input_.ndimension() == 2) {
    output.resize_({nplane, output_w});
  } else {
    output.resize_({nbatch, nplane, output_w});
  }

  Tensor input = input_.contiguous();

  IPEX_DISPATCH_FLOATING_TYPES_AND_HALF(
      input.scalar_type(), "reflection_pad1d_out_template", [&] {
        reflection_pad1d_out_kernel<scalar_t>(
            input.data_ptr<scalar_t>(),
            output.data_ptr<scalar_t>(),
            input_w,
            pad_l,
            pad_r,
            nbatch,
            nplane,
            output_w);
      });
}

template <typename scalar_t>
void reflection_pad1d_backward_out_kernel(
    scalar_t* grad_input,
    scalar_t* grad_output,
    int64_t input_w,
    int64_t pad_l,
    int64_t pad_r,
    int64_t nbatch,
    int64_t nplane,
    int64_t output_w) {
  auto& queue = dpcppGetCurrentQueue();
  int work_group_size = output_w > 256 ? 256 : output_w;
  int work_group_num = CeilDiv(output_w, (int64_t)256);

  auto cgf = DPCPP_Q_CGF(cgh) {
    auto grad_input_data = grad_input;
    auto grad_output_data = grad_output;
    auto kfn = DPCPP_Q_KFN(DPCPP::nd_item<3> item) {
      auto grad_input_ptr = grad_input_data;
      auto grad_output_ptr = grad_output_data;
      auto output_x = item.get_global_id(0);

      if (output_x < output_w) {
        // grad input index and grad output index mapping
        auto index_pair =
            get_index_mapping1d(input_w, output_w, output_x, pad_l, item);
        atomicAdd(
            (dpcpp_global_ptr_pt<scalar_t>)&grad_input_ptr[index_pair.first],
            grad_output_ptr[index_pair.second]);
      }
    };
    cgh.parallel_for<DPCPP_K(ReflectionPad1dBackward, scalar_t)>(
        DPCPP::nd_range<3>(
            DPCPP::range<3>(work_group_size * work_group_num, nplane, nbatch),
            DPCPP::range<3>(work_group_size, 1, 1)),
        kfn);
  };
  DPCPP_Q_ASYNC_SUBMIT(queue, cgf);
}

void reflection_pad1d_backward_out_template(
    Tensor& grad_input,
    const Tensor& grad_output_,
    const Tensor& input,
    IntArrayRef padding) {
  TORCH_CHECK(
      xpu::dpcpp::detail::canUse32BitIndexMath(input),
      "input tensor must fit into 32-bit index math");

  TORCH_CHECK(
      xpu::dpcpp::detail::canUse32BitIndexMath(grad_output_),
      "input tensor must fit into 32-bit index math");

  int64_t dim_plane = 0;
  int64_t dim_w = 1;
  int64_t nbatch = 1;

  if (input.ndimension() == 3) {
    nbatch = input.size(0);
    dim_plane++;
    dim_w++;
  }

  int64_t pad_l = padding[0];
  int64_t pad_r = padding[1];

  int64_t nplane = input.size(dim_plane);
  int64_t input_w = input.size(dim_w);
  int64_t output_w = input_w + pad_l + pad_r;

  Tensor grad_output = grad_output_.contiguous();

  TORCH_CHECK(
      output_w == grad_output.size(dim_w),
      "gradOutput width unexpected. Expected: ",
      output_w,
      ", Got: ",
      grad_output.size(dim_w));
      IPEX_DISPATCH_ATOMIC_FLOATING_TYPES(grad_input.scalar_type(),
        "reflection_pad1d_backward_out_template", [&] {
        reflection_pad1d_backward_out_kernel<scalar_t>(
            grad_input.data_ptr<scalar_t>(),
            grad_output.data_ptr<scalar_t>(),
            input_w,
            pad_l,
            pad_r,
            nbatch,
            nplane,
            output_w);
      });
}

} // namespace impl

Tensor& reflection_pad1d_out(
    Tensor& output,
    const Tensor& input,
    IntArrayRef padding) {
  impl::reflection_pad1d_out_template(output, input, padding);
  return output;
}

Tensor reflection_pad1d(const Tensor& input, IntArrayRef padding) {
  auto output = at::empty({0}, input.options());
  return at::AtenIpexTypeXPU::reflection_pad1d_out(output, input, padding);
}

Tensor& reflection_pad1d_backward_out(
    Tensor& grad_input,
    const Tensor& grad_output,
    const Tensor& input,
    IntArrayRef padding) {
  grad_input.resize_as_(input);
  grad_input.zero_();
  impl::reflection_pad1d_backward_out_template(
      grad_input, grad_output, input, padding);
  return grad_input;
}

Tensor reflection_pad1d_backward(
    const Tensor& grad_output,
    const Tensor& input,
    IntArrayRef padding) {
  auto grad_input = at::zeros_like(input, MemoryFormat::Contiguous);
  impl::reflection_pad1d_backward_out_template(
      grad_input, grad_output, input, padding);
  return grad_input;
}

} // namespace AtenIpexTypeXPU
} // namespace at