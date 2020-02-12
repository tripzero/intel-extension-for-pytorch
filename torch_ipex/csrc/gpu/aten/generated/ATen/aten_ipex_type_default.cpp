// Autogenerated file by gen-gpu-ops.py. Do not edit directly!
#include <ATen/aten_ipex_type_default.h>
#include <ATen/aten_ipex_type_dpcpp.h>

#include <ATen/Context.h>
#include <ATen/core/op_registration/op_registration.h>

namespace at {

at::Tensor AtenIpexTypeDefault::as_strided(const at::Tensor & self, at::IntArrayRef size, at::IntArrayRef stride, c10::optional<int64_t> storage_offset) {
  return AtenIpexTypeDPCPP::as_strided(self, size, stride, storage_offset);
}

at::Tensor AtenIpexTypeDefault::convolution_overrideable(const at::Tensor & input, const at::Tensor & weight, const at::Tensor & bias, at::IntArrayRef stride, at::IntArrayRef padding, at::IntArrayRef dilation, bool transposed, at::IntArrayRef output_padding, int64_t groups) {
  return AtenIpexTypeDPCPP::convolution_overrideable(input, weight, bias, stride, padding, dilation, transposed, output_padding, groups);
}

at::Tensor & AtenIpexTypeDefault::copy_(at::Tensor & self, const at::Tensor & src, bool non_blocking) {
  return AtenIpexTypeDPCPP::copy_(self, src, non_blocking);
}

at::Tensor AtenIpexTypeDefault::empty(at::IntArrayRef size, const at::TensorOptions & options, c10::optional<at::MemoryFormat> memory_format) {
  return AtenIpexTypeDPCPP::empty(size, options, memory_format);
}

at::Tensor & AtenIpexTypeDefault::resize_(at::Tensor & self, at::IntArrayRef size, c10::optional<at::MemoryFormat> memory_format) {
  return AtenIpexTypeDPCPP::resize_(self, size, memory_format);
}

at::Tensor & AtenIpexTypeDefault::fill_(at::Tensor & self, at::Scalar value) {
  return AtenIpexTypeDPCPP::fill_(self, value);
}

at::Tensor & AtenIpexTypeDefault::fill_(at::Tensor & self, const at::Tensor & value) {
  return AtenIpexTypeDPCPP::fill_(self, value);
}

at::Tensor AtenIpexTypeDefault::threshold(const at::Tensor & self, at::Scalar threshold, at::Scalar value) {
  return AtenIpexTypeDPCPP::threshold(self, threshold, value);
}

at::Tensor & AtenIpexTypeDefault::threshold_(at::Tensor & self, at::Scalar threshold, at::Scalar value) {
  return AtenIpexTypeDPCPP::threshold_(self, threshold, value);
}

at::Tensor & AtenIpexTypeDefault::threshold_out(at::Tensor & out, const at::Tensor & self, at::Scalar threshold, at::Scalar value) {
  return AtenIpexTypeDPCPP::threshold_out(out, self, threshold, value);
}

<<<<<<< Updated upstream
at::Tensor AtenIpexTypeDefault::addmm(const at::Tensor & self, const at::Tensor & mat1, const at::Tensor & mat2, at::Scalar beta, at::Scalar alpha) {
  return AtenIpexTypeDPCPP::addmm(self, mat1, mat2, beta, alpha);
=======
at::Tensor & AtenIpexTypeDefault::resize_as_(at::Tensor & self, const at::Tensor & the_template, c10::optional<at::MemoryFormat> memory_format) {
  return AtenIpexTypeDPCPP::resize_as_(self, the_template, memory_format);
>>>>>>> Stashed changes
}



void RegisterAtenTypeFunctions() {
  static auto dispatch = torch::RegisterOperators()
  .op(torch::RegisterOperators::options().schema("aten::as_strided(Tensor(a) self, int[] size, int[] stride, int? storage_offset=None) -> Tensor(a)")
      .impl_unboxedOnlyKernel<at::Tensor(const at::Tensor &, at::IntArrayRef, at::IntArrayRef, c10::optional<int64_t>), &AtenIpexTypeDefault::as_strided>(at::TensorTypeId::DPCPPTensorId)
      .aliasAnalysis(c10::AliasAnalysisKind::FROM_SCHEMA))
  .op(torch::RegisterOperators::options().schema("aten::convolution_overrideable(Tensor input, Tensor weight, Tensor? bias, int[] stride, int[] padding, int[] dilation, bool transposed, int[] output_padding, int groups) -> Tensor")
      .impl_unboxedOnlyKernel<at::Tensor(const at::Tensor &, const at::Tensor &, const at::Tensor &, at::IntArrayRef, at::IntArrayRef, at::IntArrayRef, bool, at::IntArrayRef, int64_t), &AtenIpexTypeDefault::convolution_overrideable>(at::TensorTypeId::DPCPPTensorId)
      .aliasAnalysis(c10::AliasAnalysisKind::FROM_SCHEMA))
  .op(torch::RegisterOperators::options().schema("aten::copy_(Tensor(a!) self, Tensor src, bool non_blocking=False) -> Tensor(a!)")
      .impl_unboxedOnlyKernel<at::Tensor &(at::Tensor &, const at::Tensor &, bool), &AtenIpexTypeDefault::copy_>(at::TensorTypeId::DPCPPTensorId)
      .aliasAnalysis(c10::AliasAnalysisKind::FROM_SCHEMA))
  .op(torch::RegisterOperators::options().schema("aten::empty.memory_format(int[] size, *, ScalarType? dtype=None, Layout? layout=None, Device? device=None, bool? pin_memory=None, MemoryFormat? memory_format=None) -> Tensor")
      .impl_unboxedOnlyKernel<at::Tensor(at::IntArrayRef, const at::TensorOptions &, c10::optional<at::MemoryFormat>), &AtenIpexTypeDefault::empty>(at::TensorTypeId::DPCPPTensorId)
      .aliasAnalysis(c10::AliasAnalysisKind::FROM_SCHEMA))
  .op(torch::RegisterOperators::options().schema("aten::resize_(Tensor(a!) self, int[] size, *, MemoryFormat? memory_format=None) -> Tensor(a!)")
      .impl_unboxedOnlyKernel<at::Tensor &(at::Tensor &, at::IntArrayRef, c10::optional<at::MemoryFormat>), &AtenIpexTypeDefault::resize_>(at::TensorTypeId::DPCPPTensorId)
      .aliasAnalysis(c10::AliasAnalysisKind::FROM_SCHEMA))
  .op(torch::RegisterOperators::options().schema("aten::fill_.Scalar(Tensor(a!) self, Scalar value) -> Tensor(a!)")
      .impl_unboxedOnlyKernel<at::Tensor &(at::Tensor &, at::Scalar), &AtenIpexTypeDefault::fill_>(at::TensorTypeId::DPCPPTensorId)
      .aliasAnalysis(c10::AliasAnalysisKind::FROM_SCHEMA))
  .op(torch::RegisterOperators::options().schema("aten::fill_.Tensor(Tensor(a!) self, Tensor value) -> Tensor(a!)")
      .impl_unboxedOnlyKernel<at::Tensor &(at::Tensor &, const at::Tensor &), &AtenIpexTypeDefault::fill_>(at::TensorTypeId::DPCPPTensorId)
      .aliasAnalysis(c10::AliasAnalysisKind::FROM_SCHEMA))
  .op(torch::RegisterOperators::options().schema("aten::threshold(Tensor self, Scalar threshold, Scalar value) -> Tensor")
      .impl_unboxedOnlyKernel<at::Tensor(const at::Tensor &, at::Scalar, at::Scalar), &AtenIpexTypeDefault::threshold>(at::TensorTypeId::DPCPPTensorId)
      .aliasAnalysis(c10::AliasAnalysisKind::FROM_SCHEMA))
  .op(torch::RegisterOperators::options().schema("aten::threshold_(Tensor(a!) self, Scalar threshold, Scalar value) -> Tensor(a!)")
      .impl_unboxedOnlyKernel<at::Tensor &(at::Tensor &, at::Scalar, at::Scalar), &AtenIpexTypeDefault::threshold_>(at::TensorTypeId::DPCPPTensorId)
      .aliasAnalysis(c10::AliasAnalysisKind::FROM_SCHEMA))
  .op(torch::RegisterOperators::options().schema("aten::threshold.out(Tensor self, Scalar threshold, Scalar value, *, Tensor(a!) out) -> Tensor(a!)")
      .impl_unboxedOnlyKernel<at::Tensor &(at::Tensor &, const at::Tensor &, at::Scalar, at::Scalar), &AtenIpexTypeDefault::threshold_out>(at::TensorTypeId::DPCPPTensorId)
      .aliasAnalysis(c10::AliasAnalysisKind::FROM_SCHEMA))
<<<<<<< Updated upstream
  .op(torch::RegisterOperators::options().schema("aten::addmm(Tensor self, Tensor mat1, Tensor mat2, *, Scalar beta=1, Scalar alpha=1) -> Tensor")
      .impl_unboxedOnlyKernel<at::Tensor(const at::Tensor &, const at::Tensor &, const at::Tensor &, at::Scalar, at::Scalar), &AtenIpexTypeDefault::addmm>(at::TensorTypeId::DPCPPTensorId)
=======
  .op(torch::RegisterOperators::options().schema("aten::resize_as_(Tensor(a!) self, Tensor the_template, *, int? memory_format=None) -> (Tensor(a!))")
      .impl_unboxedOnlyKernel<at::Tensor &(at::Tensor &, const at::Tensor &, c10::optional<at::MemoryFormat>), &AtenIpexTypeDefault::resize_as_>(at::TensorTypeId::DPCPPTensorId)
>>>>>>> Stashed changes
      .aliasAnalysis(c10::AliasAnalysisKind::FROM_SCHEMA))
;
}

}  // namespace at

