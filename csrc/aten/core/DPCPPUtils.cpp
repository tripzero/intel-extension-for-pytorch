#include <c10/core/Device.h>
#include <c10/macros/Macros.h>

#include <core/Context.h>
#include <core/DPCPPUtils.h>
#include <core/Device.h>
#include <core/Exception.h>
#include <core/Stream.h>
#include <utils/Env.h>

#include <cmath>


namespace xpu {
namespace dpcpp {

// Global device pool state
static std::once_flag init_device_flag;
static DPCPPDevicePool gDevPool;
static thread_local DeviceIndex cur_dev_index = 0;

static void clearDPCPPContextAndDevices() {
  xpu::dpcpp::clearDeviceContext();
  gDevPool.dev_sels.clear();
  gDevPool.devices.clear();
}

// It should be call only once. (std::call_once)
static void initGlobalDevicePoolState() {
  auto plaform_list = DPCPP::platform::get_platforms();
  DeviceIndex devIndex = 0;
  std::vector<DPCPP::device> root_devices;
  // Enumerated root devices(GPU cards) from GPU Platform firstly.
  for (const auto& platform : plaform_list) {
#ifdef USE_LEVEL_ZERO_ONLY
    if (platform.get_backend() != DPCPP::backend::level_zero)
      continue;
#else
    auto plat_name = platform.get_info<DPCPP::info::platform::name>();
    if (plat_name.compare(getPreferredPlatform()) != 0)
      continue;
#endif
    auto device_list = platform.get_devices();
    for (const auto& device : device_list) {
      if (device.is_gpu()) {
        root_devices.push_back(device);
      }
    }
  }

  // Mapping framework device to physical tile by default.
  // If IPEX_DISABLE_TILE_PARTITION enabled, mapping framework device to physical device.
  if (disable_tile_partition()) {
    gDevPool.devices = std::move(root_devices);
  } else {
    constexpr DPCPP::info::partition_property partition_by_affinity =
      DPCPP::info::partition_property::partition_by_affinity_domain;
    constexpr DPCPP::info::partition_affinity_domain next_partitionable =
      DPCPP::info::partition_affinity_domain::next_partitionable;
    for (const auto &root_device : root_devices) {
      std::vector<DPCPP::device> sub_devices;
      try {
        sub_devices = root_device.create_sub_devices<partition_by_affinity>(next_partitionable);
        gDevPool.devices.insert(gDevPool.devices.end(), sub_devices.begin(), sub_devices.end());
      } catch (DPCPP::feature_not_supported &e) {
        TORCH_WARN("Tile partition is not supported on this device: ", root_device.get_info<dpcpp_dev_name>());
        gDevPool.devices.push_back(root_device);
      }
    }
  }

  // Set device selector
  for (const auto& device : gDevPool.devices) {
    gDevPool.dev_sels.push_back({device});
  }
  auto device_count = gDevPool.devices.size();
  TORCH_CHECK(device_count > 0, "DPCPP Device count is zero");

  // Note: DPCPPRuntime's destruction happens before the destroy of the
  // global vars except the global vars with dpcpp type. This will make
  // our global device pool destruction crash. So we use atexit to
  // manually free all dpcpp devices. atexit callback happens before
  // DPCPPRuntime destruction.
  atexit(clearDPCPPContextAndDevices);
}

static void initDevicePoolCallOnce() {
  std::call_once(init_device_flag, initGlobalDevicePoolState);
}

int dpcppGetDeviceCount(int* deviceCount) {
  initDevicePoolCallOnce();
  std::lock_guard<std::mutex> lock(gDevPool.devices_mutex);
  *deviceCount = (int)gDevPool.devices.size();
  return DPCPP_SUCCESS;
}

int dpcppGetDevice(DeviceIndex* pDI) {
  initDevicePoolCallOnce();
  std::lock_guard<std::mutex> lock(gDevPool.devices_mutex);
  TORCH_CHECK(pDI != NULL);
  *pDI = cur_dev_index;
  return DPCPP_SUCCESS;
}

int dpcppSetDevice(DeviceIndex device_index) {
  initDevicePoolCallOnce();
  std::lock_guard<std::mutex> lock(gDevPool.devices_mutex);
  if (device_index >= (DeviceIndex)gDevPool.devices.size()) {
    TORCH_WARN("dpcppSetDevice: device_index is out of range");
  } else {
    cur_dev_index = device_index;
  }
  return DPCPP_SUCCESS;
}

DPCPP::device dpcppGetRawDevice(DeviceIndex device_index) {
  initDevicePoolCallOnce();
  std::lock_guard<std::mutex> lock(gDevPool.devices_mutex);
  if (device_index >= (DeviceIndex)gDevPool.devices.size()) {
    TORCH_CHECK(0, "dpcppSetDevice: device_index is out of range");
  }
  return gDevPool.devices[device_index];
}

DPCPPDeviceSelector dpcppGetDeviceSelector(DeviceIndex device_index) {
  initDevicePoolCallOnce();
  std::lock_guard<std::mutex> lock(gDevPool.devices_mutex);
  if (device_index >= (DeviceIndex)gDevPool.devices.size()) {
    TORCH_CHECK(0, "dpcppSetDevice: device_index is out of range");
  }
  return gDevPool.dev_sels[device_index];
}

DeviceIndex dpcppGetDeviceIndex(DPCPP::device device) {
  initDevicePoolCallOnce();
  std::lock_guard<std::mutex> lock(gDevPool.devices_mutex);
  auto it = std::find(gDevPool.devices.begin(), gDevPool.devices.end(), device);
  if (it != gDevPool.devices.end()) {
    return std::distance(gDevPool.devices.begin(), it);
  }
  return -1;
}

int dpcppGetDeviceIdFromPtr(DeviceIndex* device_id, void* ptr) {
  auto raw_device = DPCPP::get_pointer_device(ptr, xpu::dpcpp::getDeviceContext());
  *device_id = dpcppGetDeviceIndex(raw_device);
  return DPCPP_SUCCESS;
}

DPCPP::queue& dpcppGetCurrentQueue() {
  return getCurrentDPCPPStream().dpcpp_queue();
}

int64_t dpcppMaxWorkGroupSize(DPCPP::queue& queue) {
  return queue.get_device().get_info<dpcpp_dev_max_wgroup_size>();
}

int64_t dpcppMaxWorkGroupSize() {
  auto& queue = dpcppGetCurrentQueue();
  return dpcppMaxWorkGroupSize(queue);
}

int64_t dpcppMaxComputeUnitSize(DPCPP::queue& queue) {
  return queue.get_device()
      .template get_info<dpcpp_dev_max_units>();
}

int64_t dpcppMaxComputeUnitSize() {
  auto& queue = dpcppGetCurrentQueue();
  return dpcppMaxComputeUnitSize(queue);
}

int64_t dpcppMaxDSSNum(DPCPP::queue& queue) {
  // TODO: We need to got this info from DPC++ Runtime
  // Hardcode to 32 for ATS
  int64_t dss_num = 32;
  return dss_num;
}

int64_t dpcppMaxDSSNum() {
  auto& queue = dpcppGetCurrentQueue();
  return dpcppMaxDSSNum(queue);
}

int64_t dpcppLocalMemSize(DPCPP::queue& queue) {
  return queue.get_device()
      .template get_info<dpcpp_dev_local_mem_size>();
}

int64_t dpcppLocalMemSize() {
  auto& queue = dpcppGetCurrentQueue();
  return dpcppLocalMemSize(queue);
}

std::string getPreferredPlatform() {
  // TODO: To use more stable api from dpc++ runtime to preferred select
  // platform Following code logic based upon the assumption: gpu_selector will
  // select gpu device with priority considering platform: 1) level_zero 2)
  // opencl JIRA CMPLRLLVM-19937 is tracking this.
  DPCPP::device dev{DPCPP::gpu_selector{}};
  return dev.get_platform().get_info<DPCPP::info::platform::name>();
}

void parallel_for_setup(
    int64_t n,
    int64_t& tileSize,
    int64_t& rng,
    int64_t& GRange) {
  tileSize = dpcppMaxWorkGroupSize();
  rng = n;
  if (rng == 0) {
    rng = static_cast<int64_t>(1);
  }

  GRange = rng;
  if (tileSize > GRange) {
    tileSize = GRange;
  } else if (GRange > tileSize) {
    int64_t xMode = static_cast<int64_t>(GRange % tileSize);
    if (xMode != 0) {
      GRange += static_cast<int64_t>(tileSize - xMode);
    }
  }
}

void parallel_for_setup(
    int64_t dim0,
    int64_t dim1,
    int64_t& tileSize0,
    int64_t& tileSize1,
    int64_t& rng0,
    int64_t& rng1,
    int64_t& GRange0,
    int64_t& GRange1) {
  int64_t max_workgroup_Size = dpcppMaxWorkGroupSize();
  int64_t pow_of_2 = static_cast<int64_t>(std::log2(max_workgroup_Size));
  tileSize1 =
      static_cast<int64_t>(std::pow(2, static_cast<int64_t>(pow_of_2 / 2)));
  rng1 = dim1;
  if (rng1 == 0) {
    rng1 = static_cast<int64_t>(1);
  }

  GRange1 = rng1;
  if (tileSize1 > GRange1) {
    tileSize1 = GRange1;
  } else if (GRange1 > tileSize1) {
    int64_t xMode = static_cast<int64_t>(GRange1 % tileSize1);
    if (xMode != 0) {
      GRange1 += static_cast<int64_t>(tileSize1 - xMode);
    }
  }

  tileSize0 = static_cast<int64_t>(max_workgroup_Size / tileSize1);
  rng0 = dim0;
  if (rng0 == 0) {
    rng0 = static_cast<int64_t>(1);
  }

  GRange0 = rng0;
  if (tileSize0 > GRange0) {
    tileSize0 = GRange0;
  } else if (GRange0 > tileSize0) {
    int64_t xMode = static_cast<int64_t>(GRange0 % tileSize0);
    if (xMode != 0) {
      GRange0 += static_cast<int64_t>(tileSize0 - xMode);
    }
  }
}

void parallel_for_setup(
    int64_t dim0,
    int64_t dim1,
    int64_t dim2,
    int64_t& tileSize0,
    int64_t& tileSize1,
    int64_t& tileSize2,
    int64_t& rng0,
    int64_t& rng1,
    int64_t& rng2,
    int64_t& GRange0,
    int64_t& GRange1,
    int64_t& GRange2) {
  int64_t max_workgroup_Size = dpcppMaxWorkGroupSize();
  int64_t pow_of_2 = static_cast<int64_t>(std::log2(max_workgroup_Size));
  tileSize2 =
      static_cast<int64_t>(std::pow(2, static_cast<int64_t>(pow_of_2 / 3)));
  rng2 = dim2;
  if (rng2 == 0) {
    rng1 = static_cast<int64_t>(1);
  }

  GRange2 = rng2;
  if (tileSize2 > GRange2) {
    tileSize2 = GRange2;
  } else if (GRange2 > tileSize2) {
    int64_t xMode = static_cast<int64_t>(GRange2 % tileSize2);
    if (xMode != 0)
      GRange2 += static_cast<int64_t>(tileSize2 - xMode);
  }

  pow_of_2 = static_cast<int64_t>(
      std::log2(static_cast<int64_t>(max_workgroup_Size / tileSize2)));
  tileSize1 =
      static_cast<int64_t>(std::pow(2, static_cast<int64_t>(pow_of_2 / 2)));

  rng1 = dim1;
  if (rng1 == 0) {
    rng1 = static_cast<int64_t>(1);
  }

  GRange1 = rng1;
  if (tileSize1 > GRange1) {
    tileSize1 = GRange1;
  } else if (GRange1 > tileSize1) {
    int64_t xMode = static_cast<int64_t>(GRange1 % tileSize1);
    if (xMode != 0) {
      GRange1 += static_cast<int64_t>(tileSize1 - xMode);
    }
  }

  tileSize0 =
      static_cast<int64_t>(max_workgroup_Size / (tileSize1 * tileSize2));
  rng0 = dim0;
  if (rng0 == 0) {
    rng0 = static_cast<int64_t>(1);
  }

  GRange0 = rng0;
  if (tileSize0 > GRange0) {
    tileSize0 = GRange0;
  } else if (GRange0 > tileSize0) {
    int64_t xMode = static_cast<int64_t>(GRange0 % tileSize0);
    if (xMode != 0) {
      GRange0 += static_cast<int64_t>(tileSize0 - xMode);
    }
  }
}

} // namespace dpcpp
} // namespace at