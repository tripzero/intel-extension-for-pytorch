#include <ATen/NativeFunctions.h>
#include <ATen/SparseTensorUtils.h>
#include <runtime/Utils.h>
#include <core/Memory.h>

#include "comm/AccumulateType.h"
#include "comm/Numerics.h"
#include "comm/ATDispatch.h"

#ifdef USE_ONEDPL
#include <oneapi/dpl/algorithm>
#include <oneapi/dpl/execution>
#include <oneapi/dpl/numeric>
#include <oneapi/dpl/iterator>
#endif

using namespace xpu::dpcpp;
using namespace at::sparse;

namespace at {
namespace AtenIpexTypeSparseXPU {
namespace impl {

DPCPP_DEF_K2(coalesce_values_kernel, typename scalar_t);

template <typename scalar_t>
void coalesce_values_kernel(
  Tensor segment_offsets, Tensor value_indices,
  Tensor values, Tensor newValues,
  int64_t nnz, int64_t newNnz, int64_t stride) {
  using accscalar_t = AtenIpexTypeXPU::acc_type<scalar_t>;

  auto& queue = dpcppGetCurrentQueue();
  const int num_group_0 = CeilDiv(newNnz, (int64_t)4);
  const int num_group_1 = CeilDiv(stride, (int64_t)64);

  auto cgf = DPCPP_Q_CGF(cgh) {
    auto segment_offsets_data = segment_offsets.data_ptr<int64_t>();
    auto value_indices_data = value_indices.data_ptr<int64_t>();
    auto values_data = values.data_ptr<scalar_t>();
    auto newValues_data = newValues.data_ptr<scalar_t>();
    auto kfn = DPCPP_Q_KFN(DPCPP::nd_item<2> item) {
      auto segment_offsets_ptr = segment_offsets_data;
      auto value_indices_ptr = value_indices_data;
      auto values_ptr = values_data;
      auto newValues_ptr = newValues_data;

      int seg = item.get_global_id()[0];
      
      if (seg < newNnz) {
        const int newValueRow = seg * stride;
        const int begin = segment_offsets_ptr[seg];
        const int end = (seg < newNnz - 1) ? segment_offsets_ptr[seg + 1] : nnz;
        const int featureDim = item.get_global_id()[1];
        
        accscalar_t tmp = 0;
        for (int row = begin; row < end; row++) {
          const int valueRow = ((int) value_indices_ptr[row]) * stride;
          if (featureDim < stride) {
            tmp += static_cast<accscalar_t>(values_ptr[valueRow + featureDim]);
          }
        }
        if (featureDim < stride) {
          newValues_ptr[newValueRow + featureDim] = static_cast<scalar_t>(tmp);
        }
      }
    };

    // kick off kernel
    cgh.parallel_for<DPCPP_K(coalesce_values_kernel, scalar_t)>(
        DPCPP::nd_range<2>(
            DPCPP::range<2>(num_group_0 * 4, num_group_1 * 64), DPCPP::range<2>(4, 64)),
        kfn);
  };
  DPCPP_Q_ASYNC_SUBMIT(queue, cgf);

}
} // impl

Tensor _sparse_coo_tensor_with_dims_and_tensors(
  int64_t sparse_dim,
  int64_t dense_dim,
  IntArrayRef size,
  const Tensor& indices,
  const Tensor& values,
  const TensorOptions& options) {
  return at::native::new_with_dims_and_tensor_sparse(sparse_dim, dense_dim, size, indices, values, options);
}

Tensor empty(IntArrayRef size, const TensorOptions& options, c10::optional<MemoryFormat> memory_format) {
  return at::native::empty_sparse(size, options, memory_format);
}

Tensor _indices(const Tensor& self) {
  return at::native::_indices_sparse(self);
}

Tensor _values(const Tensor& self) {
  return at::native::_values_sparse(self);
}

Tensor& copy_sparse_to_sparse_(Tensor& self, const Tensor& src, bool non_blocking) {
  return at::native::copy_sparse_(self, src, non_blocking);
}

Tensor& _coalesced_(Tensor& self, bool coalesced) {
  return at::native::_coalesced_sparse_(self, coalesced);
}

bool is_coalesced(const Tensor& self) {
  return at::native::is_coalesced_sparse(self);
}

int64_t dense_dim(const Tensor& self) {
  return at::native::dense_dim_sparse(self);
}

int64_t sparse_dim(const Tensor& self) {
  return at::native::sparse_dim_sparse(self);
}

int64_t _nnz(const Tensor& self) {
  return at::native::_nnz_sparse(self);
}

Tensor coalesce(const Tensor& self) {
#ifndef USE_ONEDPL
  throw std::runtime_error("no oneDPL found when compile");
#else
  int64_t nnz = self._nnz();
  if (self.is_coalesced()) {
    return self;
  }
  // NOTE: Since `coalesce` is not an in-place operation when `is_coalesced` is false,
  // we should keep the original tensor intact and do coalesce on a copy of the tensor
  if (nnz < 2) {
    SparseTensor dst = self.clone();
    dst._coalesced_(true);
    return dst;
  }

  Tensor values = self._values();

  int64_t sparse_dim = self.sparse_dim();
  int64_t newNnz;

  // indices will be modified by Thrust, so we have to clone or use new storage
  // here.
  LongTensor indices1D = flatten_indices(self._indices(), self.sizes(), true);

  LongTensor origIndices = at::empty({nnz}, self._indices().options());
  LongTensor uniqueOffsets = at::empty({nnz}, self._indices().options());

  auto& dpcpp_queue = dpcppGetCurrentQueue();
  auto policy = oneapi::dpl::execution::make_device_policy(dpcpp_queue);

  {
    auto countIterI = oneapi::dpl::counting_iterator<int64_t>(0);
    auto countIterO = oneapi::dpl::counting_iterator<int64_t>(0);

    auto origIndices_ptr = origIndices.data_ptr<int64_t>();
    auto uniqueOffsets_ptr = uniqueOffsets.data_ptr<int64_t>();

    std::copy(policy, countIterI, countIterI + nnz, origIndices_ptr);
    std::copy(policy, countIterO, countIterO + nnz, uniqueOffsets_ptr);

    auto indices1D_ptr = indices1D.data_ptr<int64_t>();
    auto zipped_indices = oneapi::dpl::make_zip_iterator(indices1D_ptr, origIndices_ptr);
    std::sort(policy, zipped_indices, zipped_indices + nnz,
        [](auto lhs, auto rhs) {
          using std::get;
          return get<0>(lhs) < get<0>(rhs);          
        });
    auto zipped_uniqueOffsets = oneapi::dpl::make_zip_iterator(indices1D_ptr, uniqueOffsets_ptr);
    auto newEnd = std::unique(policy, zipped_uniqueOffsets, zipped_uniqueOffsets + nnz, 
    [](auto lhs, auto rhs) {
      using std::get;
      return get<0>(lhs) == get<0>(rhs);
    });
    newNnz = std::distance(zipped_uniqueOffsets, newEnd);
  }

  indices1D.resize_({1, newNnz});
  auto newValues_size = values.sizes().vec();
  newValues_size[0] = newNnz;
  Tensor newValues = at::empty(newValues_size, values.options());

  if (newValues.numel() > 0) {
    values = values.contiguous();
    int64_t stride = at::prod_intlist(values.sizes().slice(1));
    IPEX_DISPATCH_ALL_TYPES_AND2(
        at::ScalarType::BFloat16,
        at::ScalarType::Half,
        values.scalar_type(),
        "coalesce",
        [&]() {
          impl::coalesce_values_kernel<scalar_t>(uniqueOffsets, origIndices, values, newValues, nnz, newNnz, stride);
        });
  }

  LongTensor newIndices;
  if (sparse_dim == 1) {
    newIndices = indices1D;
  } else {
    newIndices = at::empty({sparse_dim, newNnz}, origIndices.options());
    for (int64_t d = sparse_dim - 1; d >= 0; d--) {
      // NB: Not a select, so I can preserve the outer dimension
      LongTensor indicesSlice = newIndices.narrow(0, d, 1);
      // Note for the porting guide: THCTensor_(copy) does NOT do normal
      // broadcasting logic; instead, it will blast the elements from one
      // to the other so long as the numel is the same
      indicesSlice.copy_(indices1D);
      indices1D.floor_divide_(self.size(d));
      indicesSlice.add_(indices1D, -self.size(d));
    }
  }
  ////////////////////////////////////////////////////////////
  // We can use unsafe sparse tensor constructor because the indices do not
  // need to be revalidated as we do not add or change indices, just remove
  // duplicates.
  SparseTensor dst = at::_sparse_coo_tensor_unsafe(newIndices, newValues, self.sizes())._coalesced_(true);

  return dst;

#endif
}

} // AtenIpexTypeSparseXPU
} // at