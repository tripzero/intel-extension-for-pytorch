// Autogenerated file by gen-gpu-ops.py. Do not edit directly!

#include <ATen/Tensor.h>

namespace at {

namespace AtenIpexTypeDPCPP {
  at::Tensor add(const at::Tensor & self, const at::Tensor & other, at::Scalar alpha);
  at::Tensor & add_(at::Tensor & self, const at::Tensor & other, at::Scalar alpha);
  at::Tensor & add_out(at::Tensor & out, const at::Tensor & self, const at::Tensor & other, at::Scalar alpha);
  at::Tensor add(const at::Tensor & self, at::Scalar other, at::Scalar alpha);
  at::Tensor & add_(at::Tensor & self, at::Scalar other, at::Scalar alpha);
  at::Tensor & arange_out(at::Tensor & out, at::Scalar start, at::Scalar end, at::Scalar step);
  at::Tensor as_strided(const at::Tensor & self, at::IntArrayRef size, at::IntArrayRef stride, c10::optional<int64_t> storage_offset);
  at::Tensor bitwise_not(const at::Tensor & self);
  at::Tensor & bitwise_not_(at::Tensor & self);
  at::Tensor & bitwise_not_out(at::Tensor & out, const at::Tensor & self);
  at::Tensor logical_not(const at::Tensor & self);
  at::Tensor & logical_not_(at::Tensor & self);
  at::Tensor & logical_not_out(at::Tensor & out, const at::Tensor & self);
  at::Tensor convolution_overrideable(const at::Tensor & input, const at::Tensor & weight, const at::Tensor & bias, at::IntArrayRef stride, at::IntArrayRef padding, at::IntArrayRef dilation, bool transposed, at::IntArrayRef output_padding, int64_t groups);
  at::Tensor & copy_(at::Tensor & self, const at::Tensor & src, bool non_blocking);
  at::Tensor div(const at::Tensor & self, const at::Tensor & other);
  at::Tensor & div_(at::Tensor & self, const at::Tensor & other);
  at::Tensor & div_out(at::Tensor & out, const at::Tensor & self, const at::Tensor & other);
  at::Tensor empty(at::IntArrayRef size, const at::TensorOptions & options, c10::optional<at::MemoryFormat> memory_format);
  at::Tensor & resize_(at::Tensor & self, at::IntArrayRef size, c10::optional<at::MemoryFormat> memory_format);
  at::Tensor empty_strided(at::IntArrayRef size, at::IntArrayRef stride, const at::TensorOptions & options);
  at::Tensor & eye_out(at::Tensor & out, int64_t n);
  at::Tensor & eye_out(at::Tensor & out, int64_t n, int64_t m);
  at::Tensor & fill_(at::Tensor & self, at::Scalar value);
  at::Tensor & fill_(at::Tensor & self, const at::Tensor & value);
  at::Tensor & linspace_out(at::Tensor & out, at::Scalar start, at::Scalar end, int64_t steps);
  at::Tensor & logspace_out(at::Tensor & out, at::Scalar start, at::Scalar end, int64_t steps, double base);
  at::Tensor _log_softmax(const at::Tensor & self, int64_t dim, bool half_to_float);
  at::Tensor mul(const at::Tensor & self, const at::Tensor & other);
  at::Tensor & mul_(at::Tensor & self, const at::Tensor & other);
  at::Tensor & mul_out(at::Tensor & out, const at::Tensor & self, const at::Tensor & other);
  at::Tensor mul(const at::Tensor & self, at::Scalar other);
  at::Tensor & mul_(at::Tensor & self, at::Scalar other);
  std::tuple<at::Tensor,at::Tensor,at::Tensor> native_batch_norm(const at::Tensor & input, const at::Tensor & weight, const at::Tensor & bias, const at::Tensor & running_mean, const at::Tensor & running_var, bool training, double momentum, double eps);
  at::Tensor & range_out(at::Tensor & out, at::Scalar start, at::Scalar end, at::Scalar step);
  at::Tensor neg(const at::Tensor & self);
  at::Tensor & neg_(at::Tensor & self);
  at::Tensor & neg_out(at::Tensor & out, const at::Tensor & self);
  at::Tensor & relu_(at::Tensor & self);
  at::Tensor threshold(const at::Tensor & self, at::Scalar threshold, at::Scalar value);
  at::Tensor & threshold_(at::Tensor & self, at::Scalar threshold, at::Scalar value);
  at::Tensor & threshold_out(at::Tensor & out, const at::Tensor & self, at::Scalar threshold, at::Scalar value);
  at::Tensor roll(const at::Tensor & self, at::IntArrayRef shifts, at::IntArrayRef dims);
  at::Tensor & resize_as_(at::Tensor & self, const at::Tensor & the_template, c10::optional<at::MemoryFormat> memory_format);
  at::Tensor & zero_(at::Tensor & self);
  at::Tensor & sub_out(at::Tensor & out, const at::Tensor & self, const at::Tensor & other, at::Scalar alpha);
  at::Tensor sub(const at::Tensor & self, const at::Tensor & other, at::Scalar alpha);
  at::Tensor & sub_(at::Tensor & self, const at::Tensor & other, at::Scalar alpha);
  at::Tensor sub(const at::Tensor & self, at::Scalar other, at::Scalar alpha);
  at::Tensor & sub_(at::Tensor & self, at::Scalar other, at::Scalar alpha);
  at::Tensor rsub(const at::Tensor & self, const at::Tensor & other, at::Scalar alpha);
  at::Tensor rsub(const at::Tensor & self, at::Scalar other, at::Scalar alpha);
  at::Tensor addmm(const at::Tensor & self, const at::Tensor & mat1, const at::Tensor & mat2, at::Scalar beta, at::Scalar alpha);
  at::Tensor view(const at::Tensor & self, at::IntArrayRef size);
  at::Tensor & tril_(at::Tensor & self, int64_t diagonal);
  at::Tensor & triu_(at::Tensor & self, int64_t diagonal);
  at::Tensor & addcdiv_(at::Tensor & self, const at::Tensor & tensor1, const at::Tensor & tensor2, at::Scalar value);
  at::Tensor & triu_out(at::Tensor & out, const at::Tensor & self, int64_t diagonal);
  at::Tensor & tril_out(at::Tensor & out, const at::Tensor & self, int64_t diagonal);
  at::Tensor tril_indices(int64_t row, int64_t col, int64_t offset, const at::TensorOptions & options);
  at::Tensor triu_indices(int64_t row, int64_t col, int64_t offset, const at::TensorOptions & options);
  at::Tensor & addcmul_out(at::Tensor & out, const at::Tensor & self, const at::Tensor & tensor1, const at::Tensor & tensor2, at::Scalar value);
  at::Tensor addcmul(const at::Tensor & self, const at::Tensor & tensor1, const at::Tensor & tensor2, at::Scalar value);
  at::Tensor & addcmul_(at::Tensor & self, const at::Tensor & tensor1, const at::Tensor & tensor2, at::Scalar value);
  at::Tensor & addcdiv_out(at::Tensor & out, const at::Tensor & self, const at::Tensor & tensor1, const at::Tensor & tensor2, at::Scalar value);
  at::Tensor addcdiv(const at::Tensor & self, const at::Tensor & tensor1, const at::Tensor & tensor2, at::Scalar value);
  std::tuple<at::Tensor,at::Tensor> nll_loss_forward(const at::Tensor & self, const at::Tensor & target, const at::Tensor & weight, int64_t reduction, int64_t ignore_index);
  at::Tensor & adaptive_avg_pool2d_out(at::Tensor & out, const at::Tensor & self, at::IntArrayRef output_size);
  at::Tensor adaptive_avg_pool2d(const at::Tensor & self, at::IntArrayRef output_size);
  at::Tensor _adaptive_avg_pool2d(const at::Tensor & self, at::IntArrayRef output_size);
  at::Tensor & avg_pool2d_out(at::Tensor & out, const at::Tensor & self, at::IntArrayRef kernel_size, at::IntArrayRef stride, at::IntArrayRef padding, bool ceil_mode, bool count_include_pad, c10::optional<int64_t> divisor_override);
  at::Tensor avg_pool2d(const at::Tensor & self, at::IntArrayRef kernel_size, at::IntArrayRef stride, at::IntArrayRef padding, bool ceil_mode, bool count_include_pad, c10::optional<int64_t> divisor_override);
  std::tuple<at::Tensor &,at::Tensor &> max_pool2d_with_indices_out(at::Tensor & out, at::Tensor & indices, const at::Tensor & self, at::IntArrayRef kernel_size, at::IntArrayRef stride, at::IntArrayRef padding, at::IntArrayRef dilation, bool ceil_mode);
  std::tuple<at::Tensor,at::Tensor> max_pool2d_with_indices(const at::Tensor & self, at::IntArrayRef kernel_size, at::IntArrayRef stride, at::IntArrayRef padding, at::IntArrayRef dilation, bool ceil_mode);
  at::Tensor & upsample_nearest2d_out(at::Tensor & out, const at::Tensor & self, at::IntArrayRef output_size);
  at::Tensor upsample_nearest2d(const at::Tensor & self, at::IntArrayRef output_size);


} // namespace AtenIpexTypeDPCPP
} // namespace at

