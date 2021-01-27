#include <torch/csrc/autograd/profiler.h>
#include <utils/Profiler.h>
#include <sstream>

float DPCPPEventStubImpl::elapsed() {
  float us;
  event_.wait();
  auto start = event_.template get_profiling_info<
      cl::sycl::info::event_profiling::command_start>();
  auto end = event_.template get_profiling_info<
      cl::sycl::info::event_profiling::command_end>();

  if (end < start) {
    std::stringstream ss;
    ss << __BASE_FILE__ << ":" << __LINE__
       << ": dpcpp profile end time < start time ";
    throw std::runtime_error(ss.str());
  }

  auto duration = end - start;
  // nanoseconds to milliseconds
  us = duration / 1000.0;
  return us;
}

struct RegisterDPCPPMethods {
  RegisterDPCPPMethods() {
    static DPCPPProvfilerStubsImpl methods;
    registerXPUMethods(&methods);
  }
};

static RegisterDPCPPMethods reg;