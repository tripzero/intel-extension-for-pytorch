#pragma once

#include <float.h>
#include <limits.h>
#include <math.h>

// The lower_bound and upper_bound constants are same as lowest and max for
// integral types, but are -inf and +inf for floating point types. They are
// useful in implementing min, max, etc.

namespace {
// warning, use std::numeric_limits if possible.
constexpr double inf = INFINITY;
} // namespace

namespace at {
template <typename T>
struct numeric_limits {};

template <>
struct numeric_limits<uint8_t> {
  static inline uint8_t lowest() {
    return 0;
  }
  static inline uint8_t max() {
    return UINT8_MAX;
  }
  static inline uint8_t lower_bound() {
    return 0;
  }
  static inline uint8_t upper_bound() {
    return UINT8_MAX;
  }
};

template <>
struct numeric_limits<int8_t> {
  static inline int8_t lowest() {
    return INT8_MIN;
  }
  static inline int8_t max() {
    return INT8_MAX;
  }
  static inline int8_t lower_bound() {
    return INT8_MIN;
  }
  static inline int8_t upper_bound() {
    return INT8_MAX;
  }
};

template <>
struct numeric_limits<int16_t> {
  static inline int16_t lowest() {
    return INT16_MIN;
  }
  static inline int16_t max() {
    return INT16_MAX;
  }
  static inline int16_t lower_bound() {
    return INT16_MIN;
  }
  static inline int16_t upper_bound() {
    return INT16_MAX;
  }
};

template <>
struct numeric_limits<int32_t> {
  static inline int32_t lowest() {
    return INT32_MIN;
  }
  static inline int32_t max() {
    return INT32_MAX;
  }
  static inline int32_t lower_bound() {
    return INT32_MIN;
  }
  static inline int32_t upper_bound() {
    return INT32_MAX;
  }
};

template <>
struct numeric_limits<int64_t> {
  static inline int64_t lowest() {
    return INT64_MIN;
  }
  static inline int64_t max() {
    return INT64_MAX;
  }
  static inline int64_t lower_bound() {
    return INT64_MIN;
  }
  static inline int64_t upper_bound() {
    return INT64_MAX;
  }
};

template <>
struct numeric_limits<at::Half> {
  static inline at::Half lowest() {
    return at::Half(0xFBFF, at::Half::from_bits());
  }
  static inline at::Half max() {
    return at::Half(0x7BFF, at::Half::from_bits());
  }
  static inline at::Half lower_bound() {
    return at::Half(0xFC00, at::Half::from_bits());
  }
  static inline at::Half upper_bound() {
    return at::Half(0x7C00, at::Half::from_bits());
  }
};

template <>
struct numeric_limits<at::BFloat16> {
  static inline at::BFloat16 lowest() {
    return c10::BFloat16(0xFF7F, at::BFloat16::from_bits());
  }
  static inline at::BFloat16 max() {
    return c10::BFloat16(0x7F7F, at::BFloat16::from_bits());
  }
  static inline at::BFloat16 lower_bound() {
    return at::BFloat16(0xFF80, at::BFloat16::from_bits());
  }
  static inline at::BFloat16 upper_bound() {
    return at::BFloat16(0x7F80, at::BFloat16::from_bits());
  }
};

template <>
struct numeric_limits<float> {
  static inline float lowest() {
    return -FLT_MAX;
  }
  static inline float max() {
    return FLT_MAX;
  }
  static inline float lower_bound() {
    return -static_cast<float>(inf);
  }
  static inline float upper_bound() {
    return static_cast<float>(inf);
  }
};

template <>
struct numeric_limits<double> {
  static inline double lowest() {
    return -DBL_MAX;
  }
  static inline double max() {
    return DBL_MAX;
  }
  static inline double lower_bound() {
    return -inf;
  }
  static inline double upper_bound() {
    return inf;
  }
};

template <>
struct numeric_limits<bool> {
  static inline uint8_t lowest() {
    return false;
  }
  static inline uint8_t max() {
    return true;
  }
  static inline uint8_t lower_bound() {
    return false;
  }
  static inline uint8_t upper_bound() {
    return true;
  }
};
} // namespace at