#include "torch_ipex/csrc/cpu/AliasOPs.h"

#include <ATen/Context.h>
#include <ATen/CPUGenerator.h>
#include <c10/util/Exception.h>
#include <c10/util/Logging.h>

#include <limits>

#include "torch_ipex/csrc/aten_ipex_bridge.h"
#include "torch_ipex/csrc/utils.h"

namespace torch_ipex {
namespace cpu {

//#define DBG
#if defined(DBG)
#define DEBUG(fmt) printf(fmt);
#else
#define DEBUG(fmt)
#endif

at::Tensor AtenIpexCPUAlias::as_strided(const at::Tensor & self, at::IntArrayRef size, at::IntArrayRef stride, c10::optional<int64_t> storage_offset) {
  TORCH_INTERNAL_ASSERT(false);
  DEBUG("AtenIpexCPUAlias::as_strided\n");
  TORCH_INTERNAL_ASSERT(self.layout() == c10::kStrided);
  auto&& _ipex_self = bridge::shallowFallbackToCPUTensor(self);
  auto&& _ipex_result = at::as_strided(_ipex_self, size, stride, storage_offset);
  static_cast<void>(_ipex_result); // Avoid warnings in case not used
  TORCH_INTERNAL_ASSERT(_ipex_result.is_contiguous());
  return bridge::upgradeToDPCPPTensor(_ipex_result);
}

std::vector<at::Tensor> AtenIpexCPUAlias::chunk(const at::Tensor & self, int64_t chunks, int64_t dim) {
  TORCH_INTERNAL_ASSERT(false);
  DEBUG("AtenIpexCPUAlias::chunk\n");
  TORCH_INTERNAL_ASSERT(self.layout() == c10::kStrided);
  auto&& _ipex_self = bridge::shallowFallbackToCPUTensor(self);
  auto&& _ipex_result = at::chunk(_ipex_self, chunks, dim);
  static_cast<void>(_ipex_result); // Avoid warnings in case not used
  return bridge::upgradeToDPCPPTensorVec(_ipex_result);
}

at::Tensor AtenIpexCPUAlias::diagonal(const at::Tensor & self, int64_t offset, int64_t dim1, int64_t dim2) {
  TORCH_INTERNAL_ASSERT(false);
  DEBUG("AtenIpexCPUAlias::diagonal\n");
  TORCH_INTERNAL_ASSERT(self.layout() == c10::kStrided);
  auto&& _ipex_self = bridge::shallowFallbackToCPUTensor(self);
  auto&& _ipex_result = at::diagonal(_ipex_self, offset, dim1, dim2);
  static_cast<void>(_ipex_result); // Avoid warnings in case not used
  TORCH_INTERNAL_ASSERT(_ipex_result.is_contiguous());
  return bridge::upgradeToDPCPPTensor(_ipex_result);
}

at::Tensor AtenIpexCPUAlias::expand(const at::Tensor & self, at::IntArrayRef size, bool implicit) {
  DEBUG("AtenIpexCPUAlias::expand\n");
  TORCH_INTERNAL_ASSERT(self.layout() == c10::kStrided);
  auto&& _ipex_self = bridge::shallowFallbackToCPUTensor(self);
  auto&& _ipex_result = _ipex_self.expand(size, implicit);
  static_cast<void>(_ipex_result); // Avoid warnings in case not used
  return bridge::shallowUpgradeToDPCPPTensorA(self, _ipex_result);
}

at::Tensor AtenIpexCPUAlias::narrow(const at::Tensor & self, int64_t dim, int64_t start, int64_t length) {
  TORCH_INTERNAL_ASSERT(false);
  DEBUG("AtenIpexCPUAlias::narrow\n");
  TORCH_INTERNAL_ASSERT(self.layout() == c10::kStrided);
  auto&& _ipex_self = bridge::shallowFallbackToCPUTensor(self);
  auto&& _ipex_result = at::narrow(_ipex_self, dim, start, length);
  static_cast<void>(_ipex_result); // Avoid warnings in case not used
  TORCH_INTERNAL_ASSERT(_ipex_result.is_contiguous());
  return bridge::upgradeToDPCPPTensor(_ipex_result);
}

at::Tensor AtenIpexCPUAlias::permute(const at::Tensor & self, at::IntArrayRef dims) {
  TORCH_INTERNAL_ASSERT(false);
  DEBUG("AtenIpexCPUAlias::permute\n");
  TORCH_INTERNAL_ASSERT(self.layout() == c10::kStrided);
  auto&& _ipex_self = bridge::shallowFallbackToCPUTensor(self);
  auto&& _ipex_result = _ipex_self.permute(dims);
  static_cast<void>(_ipex_result); // Avoid warnings in case not used
  TORCH_INTERNAL_ASSERT(_ipex_result.is_contiguous());
  return bridge::upgradeToDPCPPTensor(_ipex_result);
}

at::Tensor AtenIpexCPUAlias::numpy_T(const at::Tensor & self) {
  TORCH_INTERNAL_ASSERT(false);
  DEBUG("AtenIpexCPUAlias::numpy_T\n");
  TORCH_INTERNAL_ASSERT(self.layout() == c10::kStrided);
  auto&& _ipex_self = bridge::shallowFallbackToCPUTensor(self);
  auto&& _ipex_result = _ipex_self.numpy_T();
  static_cast<void>(_ipex_result); // Avoid warnings in case not used
  TORCH_INTERNAL_ASSERT(_ipex_result.is_contiguous());
  return bridge::upgradeToDPCPPTensor(_ipex_result);
}

at::Tensor AtenIpexCPUAlias::select(const at::Tensor & self, int64_t dim, int64_t index) {
  DEBUG("AtenIpexCPUAlias::select\n");
  TORCH_INTERNAL_ASSERT(self.layout() == c10::kStrided);
  auto&& _ipex_self = bridge::shallowFallbackToCPUTensor(self);
  auto&& _ipex_result = at::select(_ipex_self, dim, index);
  static_cast<void>(_ipex_result); // Avoid warnings in case not used
  return bridge::shallowUpgradeToDPCPPTensorA(self, _ipex_result);
}

at::Tensor AtenIpexCPUAlias::slice(const at::Tensor & self, int64_t dim, int64_t start, int64_t end, int64_t step) {
  DEBUG("AtenIpexCPUAlias::slice\n");
  TORCH_INTERNAL_ASSERT(self.layout() == c10::kStrided);
  auto&& _ipex_self = bridge::shallowFallbackToCPUTensor(self);
  auto&& _ipex_result = at::slice(_ipex_self, dim, start, end, step);
  static_cast<void>(_ipex_result); // Avoid warnings in case not used
  return bridge::shallowUpgradeToDPCPPTensorA(self, _ipex_result);
}

std::vector<at::Tensor> AtenIpexCPUAlias::split(const at::Tensor & self, int64_t split_size, int64_t dim) {
  DEBUG("AtenIpexCPUAlias::split\n");
  TORCH_INTERNAL_ASSERT(self.layout() == c10::kStrided);
  auto&& _ipex_self = bridge::shallowFallbackToCPUTensor(self);
  auto&& _ipex_result = at::split(_ipex_self, split_size, dim);
  static_cast<void>(_ipex_result); // Avoid warnings in case not used
  return bridge::shallowUpgradeToDPCPPTensorVec(_ipex_result);
}

at::Tensor AtenIpexCPUAlias::squeeze(const at::Tensor & self) {
  TORCH_INTERNAL_ASSERT(false);
  DEBUG("AtenIpexCPUAlias::squeeze\n");
  TORCH_INTERNAL_ASSERT(self.layout() == c10::kStrided);
  auto&& _ipex_self = bridge::shallowFallbackToCPUTensor(self);
  auto&& _ipex_result = at::squeeze(_ipex_self);
  static_cast<void>(_ipex_result); // Avoid warnings in case not used
  TORCH_INTERNAL_ASSERT(_ipex_result.is_contiguous());
  return bridge::upgradeToDPCPPTensor(_ipex_result);
}

at::Tensor AtenIpexCPUAlias::squeeze(const at::Tensor & self, int64_t dim) {
  DEBUG("AtenIpexCPUAlias::squeeze\n");
  TORCH_INTERNAL_ASSERT(self.layout() == c10::kStrided);
  auto&& _ipex_self = bridge::shallowFallbackToCPUTensor(self);
  auto&& _ipex_result = at::squeeze(_ipex_self, dim);
  static_cast<void>(_ipex_result); // Avoid warnings in case not used
  return bridge::shallowUpgradeToDPCPPTensorA(self, _ipex_result);
}

at::Tensor AtenIpexCPUAlias::t(const at::Tensor & self) {
  DEBUG("AtenIpexCPUAlias::t\n");
  TORCH_INTERNAL_ASSERT(self.layout() == c10::kStrided);
  auto&& _ipex_self = bridge::shallowFallbackToCPUTensor(self);
  auto&& _ipex_result = at::t(_ipex_self);
  static_cast<void>(_ipex_result); // Avoid warnings in case not used
  return bridge::shallowUpgradeToDPCPPTensorA(self, _ipex_result);
}

at::Tensor AtenIpexCPUAlias::transpose(const at::Tensor & self, int64_t dim0, int64_t dim1) {
  DEBUG("AtenIpexCPUAlias::transpose\n");
  TORCH_INTERNAL_ASSERT(self.layout() == c10::kStrided);
  auto&& _ipex_self = bridge::shallowFallbackToCPUTensor(self);
  auto&& _ipex_result = at::transpose(_ipex_self, dim0, dim1);
  static_cast<void>(_ipex_result); // Avoid warnings in case not used
  return bridge::shallowUpgradeToDPCPPTensorA(self, _ipex_result);
}

at::Tensor AtenIpexCPUAlias::unsqueeze(const at::Tensor & self, int64_t dim) {
  DEBUG("AtenIpexCPUAlias::unsqueeze\n");
  TORCH_INTERNAL_ASSERT(self.layout() == c10::kStrided);
  auto&& _ipex_self = bridge::shallowFallbackToCPUTensor(self);
  auto&& _ipex_result = at::unsqueeze(_ipex_self, dim);
  static_cast<void>(_ipex_result); // Avoid warnings in case not used
  return bridge::shallowUpgradeToDPCPPTensorA(self, _ipex_result);
}

at::Tensor AtenIpexCPUAlias::indices(const at::Tensor & self) {
  TORCH_INTERNAL_ASSERT(false);
  DEBUG("AtenIpexCPUAlias::indices\n");
  TORCH_INTERNAL_ASSERT(self.layout() == c10::kStrided);
  auto&& _ipex_self = bridge::shallowFallbackToCPUTensor(self);
  auto&& _ipex_result = _ipex_self.indices();
  static_cast<void>(_ipex_result); // Avoid warnings in case not used
  TORCH_INTERNAL_ASSERT(_ipex_result.is_contiguous());
  return bridge::upgradeToDPCPPTensor(_ipex_result);
}

at::Tensor AtenIpexCPUAlias::values(const at::Tensor & self) {
  TORCH_INTERNAL_ASSERT(false);
  DEBUG("AtenIpexCPUAlias::values\n");
  TORCH_INTERNAL_ASSERT(self.layout() == c10::kStrided);
  auto&& _ipex_self = bridge::shallowFallbackToCPUTensor(self);
  auto&& _ipex_result = _ipex_self.values();
  static_cast<void>(_ipex_result); // Avoid warnings in case not used
  TORCH_INTERNAL_ASSERT(_ipex_result.is_contiguous());
  return bridge::upgradeToDPCPPTensor(_ipex_result);
}

std::vector<at::Tensor> AtenIpexCPUAlias::unbind(const at::Tensor & self, int64_t dim) {
  TORCH_INTERNAL_ASSERT(false);
  DEBUG("AtenIpexCPUAlias::unbind\n");
  TORCH_INTERNAL_ASSERT(self.layout() == c10::kStrided);
  auto&& _ipex_self = bridge::shallowFallbackToCPUTensor(self);
  auto&& _ipex_result = at::unbind(_ipex_self, dim);
  static_cast<void>(_ipex_result); // Avoid warnings in case not used
  return bridge::upgradeToDPCPPTensorVec(_ipex_result);
}

at::Tensor AtenIpexCPUAlias::view(const at::Tensor & self, at::IntArrayRef size) {
  DEBUG("AtenIpexCPUAlias::view\n");
  TORCH_INTERNAL_ASSERT(self.layout() == c10::kStrided);
  auto&& _ipex_self = bridge::shallowFallbackToCPUTensor(self);
  auto&& _ipex_result = _ipex_self.view(size);
  static_cast<void>(_ipex_result); // Avoid warnings in case not used
  return bridge::shallowUpgradeToDPCPPTensorA(self, _ipex_result);
}

at::Tensor AtenIpexCPUAlias::unfold(const at::Tensor & self, int64_t dimension, int64_t size, int64_t step) {
  TORCH_INTERNAL_ASSERT(false);
  DEBUG("AtenIpexCPUAlias::unfold\n");
  TORCH_INTERNAL_ASSERT(self.layout() == c10::kStrided);
  auto&& _ipex_self = bridge::shallowFallbackToCPUTensor(self);
  auto&& _ipex_result = _ipex_self.unfold(dimension, size, step);
  static_cast<void>(_ipex_result); // Avoid warnings in case not used
  TORCH_INTERNAL_ASSERT(_ipex_result.is_contiguous());
  return bridge::upgradeToDPCPPTensor(_ipex_result);
}

at::Tensor AtenIpexCPUAlias::alias(const at::Tensor & self) {
  TORCH_INTERNAL_ASSERT(false);
  DEBUG("AtenIpexCPUAlias::alias\n");
  TORCH_INTERNAL_ASSERT(self.layout() == c10::kStrided);
  auto&& _ipex_self = bridge::shallowFallbackToCPUTensor(self);
  auto&& _ipex_result = at::alias(_ipex_self);
  static_cast<void>(_ipex_result); // Avoid warnings in case not used
  TORCH_INTERNAL_ASSERT(_ipex_result.is_contiguous());
  return bridge::upgradeToDPCPPTensor(_ipex_result);
}

}  // namespace cpu
}  // namespace torch_ipex
