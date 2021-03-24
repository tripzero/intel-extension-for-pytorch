#pragma once

#include <ATen/ATen.h>

#include <core/DPCPPUtils.h>
#include <core/Runtime.h>
#include <tensor/Context.h>
#include "Utils.h"

#include <oneapi/dnnl/dnnl.hpp>

#ifdef USE_PRIMITIVE_CACHE
#include <oneDNN/LRUCache.h>
#endif

using namespace dnnl;
using namespace at::AtenIpexTypeXPU;

namespace at {
namespace dpcpp {
namespace oneDNN {

typedef struct ReorderAttr {
public:
  ReorderAttr(bool is_group = false)
      : pattr_(primitive_attr()),
        sc_(std::vector<float>()) {}

public:
  void set_src_sc_and_zp(int scmask,
                         std::vector<float> sc,
                         int zpmask,
                         std::vector<int> zp) {
    pattr_.set_output_scales(scmask, sc);
    pattr_.set_zero_points(DNNL_ARG_SRC, zpmask, zp);
    sc_ = sc;
  }

  void set_dst_sc_and_zp(int scmask,
                         std::vector<float> sc,
                         int zpmask,
                         std::vector<int> zp) {
    pattr_.set_output_scales(scmask, sc);
    pattr_.set_zero_points(DNNL_ARG_DST, zpmask, zp);
    sc_ = sc;
  }

  bool is_quant() const { return !sc_.empty(); }

  std::vector<float> sc() const { return sc_; }

  primitive_attr pattr() const { return pattr_; }

  memory::dims dims(const Tensor& t) const {
    return memory::dims(t.sizes().vec());
  }

  memory::format_tag fmt(const Tensor& t) const {
    return get_dnnl_default_format(t.ndimension());
  }

  memory::data_type dt(const Tensor& t) const {
    return dt_to_dnnl(t.scalar_type());
  }

private:
  primitive_attr pattr_;
  std::vector<float> sc_;
} ReorderAttr;

static inline void reorder(const Tensor& src, Tensor& dst,
                           const ReorderAttr& rattr = ReorderAttr()) {
  TORCH_CHECK(dst.data_ptr() != src.data_ptr(),
             "oneDNN reorder supports out-place implementation only ...");

  auto engine = GpuEngineManager::Instance().get_engine({kXPU, current_device()});
  auto strm = GpuStreamManager::Instance().get_stream();

  auto src_ctx = DPCPPTensorContext::get_tensor_ctx(src);
  memory::desc src_desc = src_ctx.is_plain()
      ? memory::desc(rattr.dims(src), rattr.dt(src), rattr.fmt(src))
      : src_ctx.meta();
  auto src_mem = dpcpp_onednn_memory(src_desc, engine, src.data_ptr());

  auto dst_ctx = DPCPPTensorContext::get_tensor_ctx(dst);
  memory::desc dst_desc = dst_ctx.is_plain()
      ? memory::desc(rattr.dims(dst), rattr.dt(dst), rattr.fmt(dst))
      : dst_ctx.meta();
  auto dst_mem = dpcpp_onednn_memory(dst_desc, engine, dst.data_ptr());

  primitive prim;
  if (rattr.is_quant()) {
  auto pattr = rattr.pattr();
#ifdef USE_PRIMITIVE_CACHE
    lru_key_t key;
    auto oscale = rattr.sc();
    create_key(key, src_desc, dst_desc, oscale);
    prim = fetch_or_create_m<dnnl::reorder>(key, src_mem, dst_mem, pattr);
#else
    prim = dnnl::reorder(src_mem, dst_mem, pattr);
#endif
  } else {
#ifdef USE_PRIMITIVE_CACHE
    lru_key_t key;
    create_key(key, src_desc, dst_desc);
    prim = fetch_or_create_m<dnnl::reorder>(key, src_mem, dst_mem);
#else
    prim = dnnl::reorder(src_mem, dst_mem);
#endif
  }

  DPCPP_ONEDNN_EXEC(prim, strm,
      {{DNNL_ARG_SRC, src_mem}, {DNNL_ARG_DST, dst_mem}});
}

static inline Tensor reorder_copy(Tensor& dst, const Tensor& src) {
  auto engine = GpuEngineManager::Instance().get_engine({kXPU, current_device()});
  auto strm = GpuStreamManager::Instance().get_stream();

  // align to dst
  auto dst_ctx = DPCPPTensorContext::get_tensor_ctx(dst);
  memory::desc dst_desc = dst_ctx.is_plain() ?
                             memory::desc(get_onednn_dims(dst),
                                          get_onednn_dtype(dst),
                                          get_onednn_strides(dst)) :
                             dst_ctx.meta();
  memory dst_mem = dpcpp_onednn_memory(dst_desc, engine, dst.data_ptr());

  auto src_ctx = DPCPPTensorContext::get_tensor_ctx(src);
  memory::desc src_desc = src_ctx.is_plain() ?
                          memory::desc(get_onednn_dims(src),
                                       get_onednn_dtype(src),
                                       get_onednn_strides(src)) :
                          src_ctx.meta();
  memory src_mem = dpcpp_onednn_memory(src_desc, engine, src.data_ptr());
  // simple copy checking address only
  if (dst.data_ptr() != src.data_ptr()) {
#ifdef USE_PRIMITIVE_CACHE
    lru_key_t key;
    create_key(key, src_desc, dst_desc);
    auto prim = fetch_or_create_m<dnnl::reorder>(key, src_mem, dst_mem);
#else
    auto prim = dnnl::reorder(src_mem, dst_mem);
#endif
    DPCPP_ONEDNN_EXEC(prim, strm, {{DNNL_ARG_SRC, src_mem}, {DNNL_ARG_DST, dst_mem}});
  }

  return dst;
}

}}}