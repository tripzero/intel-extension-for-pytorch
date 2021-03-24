#include <ATen/ATen.h>
#include <core/CachingAllocator.h>

namespace at {
namespace AtenIpexTypeXPU {
Tensor record_stream(const Tensor& self, c10::Stream stream) {
  at::dpcpp::dpcpp_recordQueue(self.storage().data_ptr(), at::dpcpp::DPCPPStream::unpack(stream.pack()));
  return self;
}
}  // namespace AtenIpexTypeXPU

namespace AtenIpexTypeQuantizedXPU {
Tensor record_stream(const Tensor& self, c10::Stream stream) {
  at::dpcpp::dpcpp_recordQueue(self.storage().data_ptr(), at::dpcpp::DPCPPStream::unpack(stream.pack()));
  return self;
}
}  // namespace AtenIpexTypeQuantizedXPU
}  // namespace at