// Distributed under the MIT License.
// See LICENSE.txt for details.

/// \file
/// Defines class PointerVector

#pragma once

#include <cmath>

// The Utilities/Blaze.hpp configures Blaze
#include "ErrorHandling/Assert.hpp"
#include "Utilities/Blaze.hpp"

#include <blaze/math/CustomVector.h>
#include <blaze/system/Version.h>
#include <blaze/util/typetraits/RemoveConst.h>

// clang-tidy: do not use pointer arithmetic
#define SPECTRE_BLAZE_ALLOCATOR(_TYPE_T, _SIZE_V) \
  new _TYPE_T[_SIZE_V]  // NOLINT
#define SPECTRE_BLAZE_DEALLOCATOR blaze::ArrayDelete()

namespace blaze {
template <typename T>
BLAZE_ALWAYS_INLINE SIMDdouble step_function(const SIMDf64<T>& v) noexcept
#if BLAZE_AVX512F_MODE || BLAZE_MIC_MODE
{
  return _mm512_set_pd((~v)[7] < 0.0 ? 0.0 : 1.0, (~v)[6] < 0.0 ? 0.0 : 1.0,
                       (~v)[5] < 0.0 ? 0.0 : 1.0, (~v)[4] < 0.0 ? 0.0 : 1.0,
                       (~v)[3] < 0.0 ? 0.0 : 1.0, (~v)[2] < 0.0 ? 0.0 : 1.0,
                       (~v)[1] < 0.0 ? 0.0 : 1.0, (~v)[0] < 0.0 ? 0.0 : 1.0);
}
#elif BLAZE_AVX_MODE
{
  return _mm256_set_pd((~v)[3] < 0.0 ? 0.0 : 1.0, (~v)[2] < 0.0 ? 0.0 : 1.0,
                       (~v)[1] < 0.0 ? 0.0 : 1.0, (~v)[0] < 0.0 ? 0.0 : 1.0);
}
#elif BLAZE_SSE2_MODE
{
  return _mm_set_pd((~v)[1] < 0.0 ? 0.0 : 1.0, (~v)[0] < 0.0 ? 0.0 : 1.0);
}
#else
{
  return SIMDdouble{(~v).value < 0.0 ? 0.0 : 1.0};
}
#endif

BLAZE_ALWAYS_INLINE double step_function(const double& v) noexcept {
  return v < 0.0 ? 0.0 : 1.0;
}

struct StepFunction {
  explicit inline StepFunction() = default;

  template <typename T>
  BLAZE_ALWAYS_INLINE decltype(auto) operator()(const T& a) const noexcept {
    return step_function(a);
  }

  template <typename T>
  BLAZE_ALWAYS_INLINE decltype(auto) load(const T& a) const noexcept {
    BLAZE_CONSTRAINT_MUST_BE_SIMD_PACK(T);
    return step_function(a);
  }
};
}  // namespace blaze

template <typename VT, bool TF>
BLAZE_ALWAYS_INLINE decltype(auto) step_function(
    const blaze::DenseVector<VT, TF>& vec) noexcept {
  return map(~vec, blaze::StepFunction{});
}

template <typename VT, bool TF>
BLAZE_ALWAYS_INLINE decltype(auto) StepFunction(
    const blaze::DenseVector<VT, TF>& vec) noexcept {
  return map(~vec, blaze::StepFunction{});
}

// Blaze 3.3 and newer already has atan2 implemented
#if ((BLAZE_MAJOR_VERSION == 3) && (BLAZE_MINOR_VERSION == 2))
namespace blaze {
template <typename T0, typename T1>
BLAZE_ALWAYS_INLINE const SIMDfloat atan2(const SIMDf32<T0>& a,
                                          const SIMDf32<T1>& b) noexcept
#if BLAZE_SVML_MODE && (BLAZE_AVX512F_MODE || BLAZE_MIC_MODE)
{
  return _mm512_atan2_ps((~a).eval().value, (~b).eval().value);
}
#elif BLAZE_SVML_MODE && BLAZE_AVX_MODE
{
  return _mm256_atan2_ps((~a).eval().value, (~b).eval().value);
}
#elif BLAZE_SVML_MODE && BLAZE_SSE_MODE
{
  return _mm_atan2_ps((~a).eval().value, (~b).eval().value);
}
#else
    = delete;
#endif

template <typename T0, typename T1>
BLAZE_ALWAYS_INLINE const SIMDdouble atan2(const SIMDf64<T0>& a,
                                           const SIMDf64<T1>& b) noexcept
#if BLAZE_SVML_MODE && (BLAZE_AVX512F_MODE || BLAZE_MIC_MODE)
{
  return _mm512_atan2_pd((~a).eval().value, (~b).eval().value);
}
#elif BLAZE_SVML_MODE && BLAZE_AVX_MODE
{
  return _mm256_atan2_pd((~a).eval().value, (~b).eval().value);
}
#elif BLAZE_SVML_MODE && BLAZE_SSE_MODE
{
  return _mm_atan2_pd((~a).eval().value, (~b).eval().value);
}
#else
    = delete;
#endif

template <typename T0, typename T1>
using HasSIMDAtan2 = std::integral_constant<
    bool, std::is_same<std::decay_t<T0>, std::decay_t<T1>>::value and
              std::is_arithmetic<std::decay_t<T0>>::value and bool(  // NOLINT
                  BLAZE_SVML_MODE) and                               // NOLINT
              (bool(BLAZE_SSE_MODE) || bool(BLAZE_AVX_MODE) ||       // NOLINT
               bool(BLAZE_MIC_MODE) || bool(BLAZE_AVX512F_MODE))>;   // NOLINT

struct Atan2 {
  template <typename T1, typename T2>
  BLAZE_ALWAYS_INLINE decltype(auto) operator()(const T1& a, const T2& b) const
      noexcept {
    using std::atan2;
    return atan2(a, b);
  }

  template <typename T1, typename T2>
  static constexpr bool simdEnabled() noexcept {
    return HasSIMDAtan2<T1, T2>::value;
  }

  template <typename T1, typename T2>
  BLAZE_ALWAYS_INLINE decltype(auto) load(const T1& a, const T2& b) const
      noexcept {
    using std::atan2;
    BLAZE_CONSTRAINT_MUST_BE_SIMD_PACK(T1);
    BLAZE_CONSTRAINT_MUST_BE_SIMD_PACK(T2);
    return atan2(a, b);
  }
};
}  // namespace blaze

template <typename VT0, typename VT1, bool TF>
BLAZE_ALWAYS_INLINE decltype(auto) atan2(
    const blaze::DenseVector<VT0, TF>& y,
    const blaze::DenseVector<VT1, TF>& x) noexcept {
  return map(~y, ~x, blaze::Atan2{});
}
#endif  // ((BLAZE_MAJOR_VERSION == 3) && (BLAZE_MINOR_VERSION == 2))

// hypot function support
// Blaze 3.3 and newer already has hypot implemented
#if ((BLAZE_MAJOR_VERSION == 3) && (BLAZE_MINOR_VERSION == 2))
namespace blaze {
template <typename T0, typename T1>
BLAZE_ALWAYS_INLINE const SIMDfloat hypot(const SIMDf32<T0>& a,
                                          const SIMDf32<T1>& b) noexcept
#if BLAZE_SVML_MODE && (BLAZE_AVX512F_MODE || BLAZE_MIC_MODE)
{
  return _mm512_hypot_ps((~a).eval().value, (~b).eval().value);
}
#elif BLAZE_SVML_MODE && BLAZE_AVX_MODE
{
  return _mm256_hypot_ps((~a).eval().value, (~b).eval().value);
}
#elif BLAZE_SVML_MODE && BLAZE_SSE_MODE
{
  return _mm_hypot_ps((~a).eval().value, (~b).eval().value);
}
#else
    = delete;
#endif

template <typename T0, typename T1>
BLAZE_ALWAYS_INLINE const SIMDdouble hypot(const SIMDf64<T0>& a,
                                           const SIMDf64<T1>& b) noexcept
#if BLAZE_SVML_MODE && (BLAZE_AVX512F_MODE || BLAZE_MIC_MODE)
{
  return _mm512_hypot_pd((~a).eval().value, (~b).eval().value);
}
#elif BLAZE_SVML_MODE && BLAZE_AVX_MODE
{
  return _mm256_hypot_pd((~a).eval().value, (~b).eval().value);
}
#elif BLAZE_SVML_MODE && BLAZE_SSE_MODE
{
  return _mm_hypot_pd((~a).eval().value, (~b).eval().value);
}
#else
    = delete;
#endif

template <typename T0, typename T1>
using HasSIMDHypot = std::integral_constant<
    bool, std::is_same<std::decay_t<T0>, std::decay_t<T1>>::value and
              std::is_arithmetic<std::decay_t<T0>>::value and bool(  // NOLINT
                  BLAZE_SVML_MODE) and                               // NOLINT
              (bool(BLAZE_SSE_MODE) || bool(BLAZE_AVX_MODE) ||       // NOLINT
               bool(BLAZE_MIC_MODE) || bool(BLAZE_AVX512F_MODE))>;   // NOLINT

struct Hypot {
  template <typename T1, typename T2>
  BLAZE_ALWAYS_INLINE decltype(auto) operator()(const T1& a, const T2& b) const
      noexcept {
    using std::hypot;
    return hypot(a, b);
  }

  template <typename T1, typename T2>
  static constexpr bool simdEnabled() noexcept {
    return HasSIMDHypot<T1, T2>::value;
  }

  template <typename T1, typename T2>
  BLAZE_ALWAYS_INLINE decltype(auto) load(const T1& a, const T2& b) const
      noexcept {
    using std::hypot;
    BLAZE_CONSTRAINT_MUST_BE_SIMD_PACK(T1);
    BLAZE_CONSTRAINT_MUST_BE_SIMD_PACK(T2);
    return hypot(a, b);
  }
};
}  // namespace blaze

template <typename VT0, typename VT1, bool TF>
BLAZE_ALWAYS_INLINE decltype(auto) hypot(
    const blaze::DenseVector<VT0, TF>& x,
    const blaze::DenseVector<VT1, TF>& y) noexcept {
  return map(~x, ~y, blaze::Hypot{});
}
#endif  // ((BLAZE_MAJOR_VERSION == 3) && (BLAZE_MINOR_VERSION == 2))

namespace blaze {
template <typename ST>
struct DivideScalarByVector {
 public:
  explicit inline DivideScalarByVector(ST scalar) : scalar_(scalar) {}

  template <typename T>
  BLAZE_ALWAYS_INLINE decltype(auto) operator()(const T& a) const {
    return scalar_ / a;
  }

  template <typename T>
  static constexpr bool simdEnabled() {
    return blaze::HasSIMDDiv<T, ST>::value;
  }

  template <typename T>
  BLAZE_ALWAYS_INLINE decltype(auto) load(const T& a) const {
    BLAZE_CONSTRAINT_MUST_BE_SIMD_PACK(T);
    return set(scalar_) / a;
  }

 private:
  ST scalar_;
};

template <typename Scalar, typename VT, bool TF,
          typename = blaze::EnableIf_<blaze::IsNumeric<Scalar>>>
BLAZE_ALWAYS_INLINE decltype(auto) operator/(
    Scalar scalar, const blaze::DenseVector<VT, TF>& vec) {
  return forEach(~vec, DivideScalarByVector<Scalar>(scalar));
}

template <typename ST>
struct AddScalar {
 public:
  explicit inline AddScalar(ST scalar) : scalar_(scalar) {}

  template <typename T>
  BLAZE_ALWAYS_INLINE decltype(auto) operator()(const T& a) const {
    return a + scalar_;
  }

  template <typename T>
  static constexpr bool simdEnabled() {
    return blaze::HasSIMDAdd<T, ST>::value;
  }

  template <typename T>
  BLAZE_ALWAYS_INLINE decltype(auto) load(const T& a) const {
    BLAZE_CONSTRAINT_MUST_BE_SIMD_PACK(T);
    return a + set(scalar_);
  }

 private:
  ST scalar_;
};

template <typename VT, bool TF, typename Scalar,
          typename = blaze::EnableIf_<blaze::IsNumeric<Scalar>>>
decltype(auto) operator+(const blaze::DenseVector<VT, TF>& vec, Scalar scalar) {
  return forEach(~vec, AddScalar<Scalar>(scalar));
}

template <typename Scalar, typename VT, bool TF,
          typename = blaze::EnableIf_<blaze::IsNumeric<Scalar>>>
decltype(auto) operator+(Scalar scalar, const blaze::DenseVector<VT, TF>& vec) {
  return forEach(~vec, AddScalar<Scalar>(scalar));
}

template <typename VT, bool TF, typename Scalar,
          typename = blaze::EnableIf_<blaze::IsNumeric<Scalar>>>
VT& operator+=(blaze::DenseVector<VT, TF>& vec, Scalar scalar) {
  (~vec) = (~vec) + scalar;
  return ~vec;
}

template <typename ST>
struct SubScalarRhs {
 public:
  explicit inline SubScalarRhs(ST scalar) : scalar_(scalar) {}

  template <typename T>
  BLAZE_ALWAYS_INLINE decltype(auto) operator()(const T& a) const {
    return a - scalar_;
  }

  template <typename T>
  static constexpr bool simdEnabled() {
    return blaze::HasSIMDSub<T, ST>::value;
  }

  template <typename T>
  BLAZE_ALWAYS_INLINE decltype(auto) load(const T& a) const {
    BLAZE_CONSTRAINT_MUST_BE_SIMD_PACK(T);
    return a - set(scalar_);
  }

 private:
  ST scalar_;
};

template <typename ST>
struct SubScalarLhs {
 public:
  explicit inline SubScalarLhs(ST scalar) : scalar_(scalar) {}

  template <typename T>
  BLAZE_ALWAYS_INLINE decltype(auto) operator()(const T& a) const {
    return scalar_ - a;
  }

  template <typename T>
  static constexpr bool simdEnabled() {
    return blaze::HasSIMDSub<T, ST>::value;
  }

  template <typename T>
  BLAZE_ALWAYS_INLINE decltype(auto) load(const T& a) const {
    BLAZE_CONSTRAINT_MUST_BE_SIMD_PACK(T);
    return set(scalar_) - a;
  }

 private:
  ST scalar_;
};

template <typename VT, bool TF, typename Scalar,
          typename = blaze::EnableIf_<blaze::IsNumeric<Scalar>>>
decltype(auto) operator-(const blaze::DenseVector<VT, TF>& vec, Scalar scalar) {
  return forEach(~vec, SubScalarRhs<Scalar>(scalar));
}

template <typename VT, bool TF, typename Scalar,
          typename = blaze::EnableIf_<blaze::IsNumeric<Scalar>>>
decltype(auto) operator-(Scalar scalar, const blaze::DenseVector<VT, TF>& vec) {
  return forEach(~vec, SubScalarLhs<Scalar>(scalar));
}

template <typename VT, bool TF, typename Scalar,
          typename = blaze::EnableIf_<blaze::IsNumeric<Scalar>>>
VT& operator-=(blaze::DenseVector<VT, TF>& vec, Scalar scalar) {
  (~vec) = (~vec) - scalar;
  return ~vec;
}
}  // namespace blaze
/// \endcond

/*!
 * \ingroup UtilitiesGroup
 * \brief A raw pointer endowed with expression template support via the Blaze
 * library
 *
 * PointerVector can be used instead of a raw pointer to pass around size
 * information and to be able to have the pointer array support expression
 * templates. The primary use case for PointerVector is inside the Data class
 * so that Data has support for expression templates but not incurring any
 * overhead for them.
 *
 * See the Blaze documentation for CustomVector for details on the template
 * parameters to PointerVector since CustomVector is what PointerVector is
 * modeled after.
 */
template <typename Type, bool AF = blaze::unaligned, bool PF = blaze::unpadded,
          bool TF = blaze::defaultTransposeFlag>
struct PointerVector
    : public blaze::DenseVector<PointerVector<Type, AF, PF, TF>, TF> {
  /// \cond
 public:
  using This = PointerVector<Type, AF, PF, TF>;
  using BaseType = blaze::DenseVector<This, TF>;
  using ResultType = blaze::DynamicVector<blaze::RemoveConst_<Type>, TF>;
  using TransposeType = PointerVector<Type, AF, PF, !TF>;
  using ElementType = Type;
  using SIMDType = blaze::SIMDTrait_<ElementType>;
  using ReturnType = const Type&;
  using CompositeType = const PointerVector&;

  using Reference = Type&;
  using ConstReference = const Type&;
  using Pointer = Type*;
  using ConstPointer = const Type*;

  using Iterator = blaze::DenseIterator<Type, AF>;
  using ConstIterator = blaze::DenseIterator<const Type, AF>;

  enum : bool { simdEnabled = blaze::IsVectorizable<Type>::value };
  enum : bool { smpAssignable = !blaze::IsSMPAssignable<Type>::value };

  PointerVector() = default;
  PointerVector(Type* ptr, size_t size) : v_(ptr), size_(size) {}
  PointerVector(const PointerVector& /*rhs*/) = default;
  PointerVector& operator=(const PointerVector& /*rhs*/) = default;
  PointerVector(PointerVector&& /*rhs*/) = default;
  PointerVector& operator=(PointerVector&& /*rhs*/) = default;
  ~PointerVector() = default;

  /*!\name Data access functions */
  //@{
  Type& operator[](const size_t i) noexcept {
    // clang-tidy: do not use pointer arithmetic
    return v_[i];  // NOLINT
  }
  const Type& operator[](const size_t i) const noexcept {
    // clang-tidy: do not use pointer arithmetic
    return v_[i];  // NOLINT
  }
  inline Reference at(size_t index);
  inline ConstReference at(size_t index) const;
  Pointer data() noexcept { return v_; }
  ConstPointer data() const noexcept { return v_; }
  Iterator begin() noexcept { return Iterator(v_); }
  ConstIterator begin() const noexcept { return ConstIterator(v_); }
  ConstIterator cbegin() const noexcept { return ConstIterator(v_); }
  Iterator end() noexcept {
    // clang-tidy: do not use pointer arithmetic
    return Iterator(v_ + size_);  // NOLINT
  }
  ConstIterator end() const noexcept {
    // clang-tidy: do not use pointer arithmetic
    return ConstIterator(v_ + size_);  // NOLINT
  }
  ConstIterator cend() const noexcept { return ConstIterator(v_ + size_); }
  //@}

  /*!\name Assignment operators */
  //@{
  inline PointerVector& operator=(const Type& rhs);
  inline PointerVector& operator=(std::initializer_list<Type> list);

  template <typename Other, size_t N>
  inline PointerVector& operator=(const Other (&array)[N]);

  template <typename VT>
  inline PointerVector& operator=(const blaze::Vector<VT, TF>& rhs);
  template <typename VT>
  inline PointerVector& operator+=(const blaze::Vector<VT, TF>& rhs);
  template <typename VT>
  inline PointerVector& operator-=(const blaze::Vector<VT, TF>& rhs);
  template <typename VT>
  inline PointerVector& operator*=(const blaze::Vector<VT, TF>& rhs);
  template <typename VT>
  inline PointerVector& operator/=(const blaze::Vector<VT, TF>& rhs);
  template <typename VT>
  inline PointerVector& operator%=(const blaze::Vector<VT, TF>& rhs);

  template <typename Other>
  inline std::enable_if_t<blaze::IsNumeric<Other>::value,
                          PointerVector<Type, AF, PF, TF>>&
  operator*=(Other rhs);

  template <typename Other>
  inline std::enable_if_t<blaze::IsNumeric<Other>::value,
                          PointerVector<Type, AF, PF, TF>>&
  operator/=(Other rhs);
  //@}

  /*!\name Utility functions */
  //@{
  void clear() noexcept {
    size_ = 0;
    v_ = nullptr;
  }

  size_t spacing() const noexcept { return size_; }

  size_t size() const noexcept { return size_; }
  //@}

  /*!\name Resource management functions */
  //@{
  void reset() { clear(); }

  inline void reset(Type* ptr, size_t n) {
    v_ = ptr;
    size_ = n;
  }
  //@}

 private:
  template <typename VT>
  using VectorizedAssign = std::integral_constant<
      bool, blaze::useOptimizedKernels && simdEnabled && VT::simdEnabled &&
                blaze::IsSIMDCombinable<Type, blaze::ElementType_<VT>>::value>;

  template <typename VT>
  using VectorizedAddAssign = std::integral_constant<
      bool, blaze::useOptimizedKernels && simdEnabled && VT::simdEnabled &&
                blaze::IsSIMDCombinable<Type, blaze::ElementType_<VT>>::value &&
                blaze::HasSIMDAdd<Type, blaze::ElementType_<VT>>::value>;

  template <typename VT>
  using VectorizedSubAssign = std::integral_constant<
      bool, blaze::useOptimizedKernels && simdEnabled && VT::simdEnabled &&
                blaze::IsSIMDCombinable<Type, blaze::ElementType_<VT>>::value &&
                blaze::HasSIMDSub<Type, blaze::ElementType_<VT>>::value>;

  template <typename VT>
  using VectorizedMultAssign = std::integral_constant<
      bool, blaze::useOptimizedKernels && simdEnabled && VT::simdEnabled &&
                blaze::IsSIMDCombinable<Type, blaze::ElementType_<VT>>::value &&
                blaze::HasSIMDMult<Type, blaze::ElementType_<VT>>::value>;

  template <typename VT>
  using VectorizedDivAssign = std::integral_constant<
      bool, blaze::useOptimizedKernels && simdEnabled && VT::simdEnabled &&
                blaze::IsSIMDCombinable<Type, blaze::ElementType_<VT>>::value &&
                blaze::HasSIMDDiv<Type, blaze::ElementType_<VT>>::value>;

  //! The number of elements packed within a single SIMD element.
  enum : size_t { SIMDSIZE = blaze::SIMDTrait<ElementType>::size };

 public:
  /*!\name Expression template evaluation functions */
  //@{
  template <typename Other>
  inline bool canAlias(const Other* alias) const noexcept;
  template <typename Other>
  inline bool isAliased(const Other* alias) const noexcept;

  inline bool isAligned() const noexcept;
  inline bool canSMPAssign() const noexcept;

  BLAZE_ALWAYS_INLINE SIMDType load(size_t index) const noexcept;
  BLAZE_ALWAYS_INLINE SIMDType loada(size_t index) const noexcept;
  BLAZE_ALWAYS_INLINE SIMDType loadu(size_t index) const noexcept;

  BLAZE_ALWAYS_INLINE void store(size_t index, const SIMDType& value) noexcept;
  BLAZE_ALWAYS_INLINE void storea(size_t index, const SIMDType& value) noexcept;
  BLAZE_ALWAYS_INLINE void storeu(size_t index, const SIMDType& value) noexcept;
  BLAZE_ALWAYS_INLINE void stream(size_t index, const SIMDType& value) noexcept;

  template <typename VT>
  inline std::enable_if_t<not(
      PointerVector<Type, AF, PF, TF>::template VectorizedAssign<VT>::value)>
  assign(const blaze::DenseVector<VT, TF>& rhs);

  template <typename VT>
  inline std::enable_if_t<(VectorizedAssign<VT>::value)> assign(
      const blaze::DenseVector<VT, TF>& rhs);

  template <typename VT>
  inline std::enable_if_t<not(
      PointerVector<Type, AF, PF, TF>::template VectorizedAddAssign<VT>::value)>
  addAssign(const blaze::DenseVector<VT, TF>& rhs);

  template <typename VT>
  inline std::enable_if_t<(VectorizedAddAssign<VT>::value)> addAssign(
      const blaze::DenseVector<VT, TF>& rhs);

  template <typename VT>
  inline void addAssign(const blaze::SparseVector<VT, TF>& rhs);

  template <typename VT>
  inline std::enable_if_t<not(
      PointerVector<Type, AF, PF, TF>::template VectorizedSubAssign<VT>::value)>
  subAssign(const blaze::DenseVector<VT, TF>& rhs);

  template <typename VT>
  inline std::enable_if_t<(VectorizedSubAssign<VT>::value)> subAssign(
      const blaze::DenseVector<VT, TF>& rhs);

  template <typename VT>
  inline void subAssign(const blaze::SparseVector<VT, TF>& rhs);

  template <typename VT>
  inline std::enable_if_t<not(PointerVector<Type, AF, PF, TF>::
                                  template VectorizedMultAssign<VT>::value)>
  multAssign(const blaze::DenseVector<VT, TF>& rhs);

  template <typename VT>
  inline std::enable_if_t<(VectorizedMultAssign<VT>::value)> multAssign(
      const blaze::DenseVector<VT, TF>& rhs);

  template <typename VT>
  inline void multAssign(const blaze::SparseVector<VT, TF>& rhs);

  template <typename VT>
  inline std::enable_if_t<not(
      PointerVector<Type, AF, PF, TF>::template VectorizedDivAssign<VT>::value)>
  divAssign(const blaze::DenseVector<VT, TF>& rhs);

  template <typename VT>
  inline std::enable_if_t<(VectorizedDivAssign<VT>::value)> divAssign(
      const blaze::DenseVector<VT, TF>& rhs);
  //@}

 private:
  Type* v_ = nullptr;
  size_t size_ = 0;
  /// \endcond
};

/// \cond
template <typename Type, bool AF, bool PF, bool TF>
inline typename PointerVector<Type, AF, PF, TF>::Reference
PointerVector<Type, AF, PF, TF>::at(size_t index) {
  if (index >= size_) {
    BLAZE_THROW_OUT_OF_RANGE("Invalid vector access index");
  }
  return (*this)[index];
}

template <typename Type, bool AF, bool PF, bool TF>
inline typename PointerVector<Type, AF, PF, TF>::ConstReference
PointerVector<Type, AF, PF, TF>::at(size_t index) const {
  if (index >= size_) {
    BLAZE_THROW_OUT_OF_RANGE("Invalid vector access index");
  }
  return (*this)[index];
}

template <typename Type, bool AF, bool PF, bool TF>
inline PointerVector<Type, AF, PF, TF>& PointerVector<Type, AF, PF, TF>::
operator=(const Type& rhs) {
  for (size_t i = 0; i < size_; ++i) {
    // clang-tidy: do not use pointer arithmetic
    v_[i] = rhs;  // NOLINT
  }
  return *this;
}

template <typename Type, bool AF, bool PF, bool TF>
inline PointerVector<Type, AF, PF, TF>& PointerVector<Type, AF, PF, TF>::
operator=(std::initializer_list<Type> list) {
  ASSERT(list.size() <= size_, "Invalid assignment to custom vector");
  std::fill(std::copy(list.begin(), list.end(), v_), v_ + size_, Type());
  return *this;
}

template <typename Type, bool AF, bool PF, bool TF>
template <typename Other, size_t N>
inline PointerVector<Type, AF, PF, TF>& PointerVector<Type, AF, PF, TF>::
operator=(const Other (&array)[N]) {
  ASSERT(size_ == N, "Invalid array size");
  for (size_t i = 0UL; i < N; ++i) {
    // clang-tidy: do not use pointer arithmetic
    v_[i] = array[i];  // NOLINT
  }
  return *this;
}

template <typename Type, bool AF, bool PF, bool TF>
template <typename VT>
inline PointerVector<Type, AF, PF, TF>& PointerVector<Type, AF, PF, TF>::
operator=(const blaze::Vector<VT, TF>& rhs) {
  ASSERT((~rhs).size() == size_, "Vector sizes do not match");
  blaze::smpAssign(*this, ~rhs);
  return *this;
}

template <typename Type, bool AF, bool PF, bool TF>
template <typename VT>
inline PointerVector<Type, AF, PF, TF>& PointerVector<Type, AF, PF, TF>::
operator+=(const blaze::Vector<VT, TF>& rhs) {
  ASSERT((~rhs).size() == size_, "Vector sizes do not match");
  blaze::smpAddAssign(*this, ~rhs);
  return *this;
}

template <typename Type, bool AF, bool PF, bool TF>
template <typename VT>
inline PointerVector<Type, AF, PF, TF>& PointerVector<Type, AF, PF, TF>::
operator-=(const blaze::Vector<VT, TF>& rhs) {
  ASSERT((~rhs).size() == size_, "Vector sizes do not match");
  blaze::smpSubAssign(*this, ~rhs);
  return *this;
}

template <typename Type, bool AF, bool PF, bool TF>
template <typename VT>
inline PointerVector<Type, AF, PF, TF>& PointerVector<Type, AF, PF, TF>::
operator*=(const blaze::Vector<VT, TF>& rhs) {
  BLAZE_CONSTRAINT_MUST_BE_VECTOR_WITH_TRANSPOSE_FLAG(VT, TF);
  BLAZE_CONSTRAINT_MUST_NOT_REQUIRE_EVALUATION(blaze::ResultType_<VT>);

  using MultType = blaze::MultTrait_<ResultType, blaze::ResultType_<VT>>;

  BLAZE_CONSTRAINT_MUST_BE_VECTOR_WITH_TRANSPOSE_FLAG(MultType, TF);
  BLAZE_CONSTRAINT_MUST_NOT_REQUIRE_EVALUATION(MultType);

  ASSERT((~rhs).size() == size_, "Vector sizes do not match");
  blaze::smpMultAssign(*this, ~rhs);
  return *this;
}

template <typename Type, bool AF, bool PF, bool TF>
template <typename VT>
inline PointerVector<Type, AF, PF, TF>& PointerVector<Type, AF, PF, TF>::
operator/=(const blaze::Vector<VT, TF>& rhs) {
  BLAZE_CONSTRAINT_MUST_BE_VECTOR_WITH_TRANSPOSE_FLAG(VT, TF);
  BLAZE_CONSTRAINT_MUST_NOT_REQUIRE_EVALUATION(blaze::ResultType_<VT>);

  using DivType = blaze::DivTrait_<ResultType, blaze::ResultType_<VT>>;

  BLAZE_CONSTRAINT_MUST_BE_VECTOR_WITH_TRANSPOSE_FLAG(DivType, TF);
  BLAZE_CONSTRAINT_MUST_NOT_REQUIRE_EVALUATION(DivType);

  ASSERT((~rhs).size() == size_, "Vector sizes do not match");
  blaze::smpDivAssign(*this, ~rhs);
  return *this;
}

template <typename Type, bool AF, bool PF, bool TF>
template <typename VT>
inline PointerVector<Type, AF, PF, TF>& PointerVector<Type, AF, PF, TF>::
operator%=(const blaze::Vector<VT, TF>& rhs) {
  using blaze::assign;

  BLAZE_CONSTRAINT_MUST_BE_VECTOR_WITH_TRANSPOSE_FLAG(VT, TF);
  BLAZE_CONSTRAINT_MUST_NOT_REQUIRE_EVALUATION(blaze::ResultType_<VT>);

  using CrossType = blaze::CrossTrait_<ResultType, blaze::ResultType_<VT>>;

  BLAZE_CONSTRAINT_MUST_BE_DENSE_VECTOR_TYPE(CrossType);
  BLAZE_CONSTRAINT_MUST_BE_VECTOR_WITH_TRANSPOSE_FLAG(CrossType, TF);
  BLAZE_CONSTRAINT_MUST_NOT_REQUIRE_EVALUATION(CrossType);

  if (size_ != 3UL || (~rhs).size() != 3UL) {
    BLAZE_THROW_INVALID_ARGUMENT("Invalid vector size for cross product");
  }

  const CrossType tmp(*this % (~rhs));
  assign(*this, tmp);

  return *this;
}

template <typename Type, bool AF, bool PF, bool TF>
template <typename Other>
inline std::enable_if_t<blaze::IsNumeric<Other>::value,
                        PointerVector<Type, AF, PF, TF>>&
PointerVector<Type, AF, PF, TF>::operator*=(Other rhs) {
  blaze::smpAssign(*this, (*this) * rhs);
  return *this;
}

template <typename Type, bool AF, bool PF, bool TF>
template <typename Other>
inline std::enable_if_t<blaze::IsNumeric<Other>::value,
                        PointerVector<Type, AF, PF, TF>>&
PointerVector<Type, AF, PF, TF>::operator/=(Other rhs) {
  BLAZE_USER_ASSERT(rhs != Other(0), "Division by zero detected");
  blaze::smpAssign(*this, (*this) / rhs);
  return *this;
}

template <typename Type, bool AF, bool PF, bool TF>
template <typename Other>
inline bool PointerVector<Type, AF, PF, TF>::canAlias(const Other* alias) const
    noexcept {
  return static_cast<const void*>(this) == static_cast<const void*>(alias);
}

template <typename Type, bool AF, bool PF, bool TF>
template <typename Other>
inline bool PointerVector<Type, AF, PF, TF>::isAliased(const Other* alias) const
    noexcept {
  return static_cast<const void*>(this) == static_cast<const void*>(alias);
}

template <typename Type, bool AF, bool PF, bool TF>
inline bool PointerVector<Type, AF, PF, TF>::isAligned() const noexcept {
  return (AF || checkAlignment(v_));
}

template <typename Type, bool AF, bool PF, bool TF>
inline bool PointerVector<Type, AF, PF, TF>::canSMPAssign() const noexcept {
  return (size() > blaze::SMP_DVECASSIGN_THRESHOLD);
}

template <typename Type, bool AF, bool PF, bool TF>
BLAZE_ALWAYS_INLINE typename PointerVector<Type, AF, PF, TF>::SIMDType
PointerVector<Type, AF, PF, TF>::load(size_t index) const noexcept {
  if (AF) {
    return loada(index);
  }
  return loadu(index);
}

template <typename Type, bool AF, bool PF, bool TF>
BLAZE_ALWAYS_INLINE typename PointerVector<Type, AF, PF, TF>::SIMDType
PointerVector<Type, AF, PF, TF>::loada(size_t index) const noexcept {
  using blaze::loada;

  BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE(Type);

  BLAZE_INTERNAL_ASSERT(index < size_, "Invalid vector access index");
  BLAZE_INTERNAL_ASSERT(index + SIMDSIZE <= size_,
                        "Invalid vector access index");
  BLAZE_INTERNAL_ASSERT(!AF || index % SIMDSIZE == 0UL,
                        "Invalid vector access index");
  BLAZE_INTERNAL_ASSERT(checkAlignment(v_ + index),
                        "Invalid vector access index");
  // clang-tidy: do not use pointer arithmetic
  return loada(v_ + index);  // NOLINT
}

template <typename Type, bool AF, bool PF, bool TF>
BLAZE_ALWAYS_INLINE typename PointerVector<Type, AF, PF, TF>::SIMDType
PointerVector<Type, AF, PF, TF>::loadu(size_t index) const noexcept {
  using blaze::loadu;

  BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE(Type);

  BLAZE_INTERNAL_ASSERT(index < size_, "Invalid vector access index");
  BLAZE_INTERNAL_ASSERT(index + SIMDSIZE <= size_,
                        "Invalid vector access index");
  // clang-tidy: do not use pointer arithmetic
  return loadu(v_ + index);  // NOLINT
}

template <typename Type, bool AF, bool PF, bool TF>
BLAZE_ALWAYS_INLINE void PointerVector<Type, AF, PF, TF>::store(
    size_t index, const SIMDType& value) noexcept {
  if (AF) {
    storea(index, value);
  } else {
    storeu(index, value);
  }
}

template <typename Type, bool AF, bool PF, bool TF>
BLAZE_ALWAYS_INLINE void PointerVector<Type, AF, PF, TF>::storea(
    size_t index, const SIMDType& value) noexcept {
  using blaze::storea;

  BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE(Type);

  BLAZE_INTERNAL_ASSERT(index < size_, "Invalid vector access index");
  BLAZE_INTERNAL_ASSERT(index + SIMDSIZE <= size_,
                        "Invalid vector access index");
  BLAZE_INTERNAL_ASSERT(!AF || index % SIMDSIZE == 0UL,
                        "Invalid vector access index");
  BLAZE_INTERNAL_ASSERT(checkAlignment(v_ + index),
                        "Invalid vector access index");
  // clang-tidy: do not use pointer arithmetic
  storea(v_ + index, value);  // NOLINT
}

template <typename Type, bool AF, bool PF, bool TF>
BLAZE_ALWAYS_INLINE void PointerVector<Type, AF, PF, TF>::storeu(
    size_t index, const SIMDType& value) noexcept {
  using blaze::storeu;

  BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE(Type);

  BLAZE_INTERNAL_ASSERT(index < size_, "Invalid vector access index");
  BLAZE_INTERNAL_ASSERT(index + SIMDSIZE <= size_,
                        "Invalid vector access index");
  // clang-tidy: do not use pointer arithmetic
  storeu(v_ + index, value);  // NOLINT
}

template <typename Type, bool AF, bool PF, bool TF>
BLAZE_ALWAYS_INLINE void PointerVector<Type, AF, PF, TF>::stream(
    size_t index, const SIMDType& value) noexcept {
  using blaze::stream;

  BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE(Type);

  BLAZE_INTERNAL_ASSERT(index < size_, "Invalid vector access index");
  BLAZE_INTERNAL_ASSERT(index + SIMDSIZE <= size_,
                        "Invalid vector access index");
  BLAZE_INTERNAL_ASSERT(!AF || index % SIMDSIZE == 0UL,
                        "Invalid vector access index");
  BLAZE_INTERNAL_ASSERT(checkAlignment(v_ + index),
                        "Invalid vector access index");
  // clang-tidy: do not use pointer arithmetic
  stream(v_ + index, value);  // NOLINT
}

template <typename Type, bool AF, bool PF, bool TF>
template <typename VT>
inline std::enable_if_t<
    not(PointerVector<Type, AF, PF, TF>::template PointerVector<
        Type, AF, PF, TF>::BLAZE_TEMPLATE VectorizedAssign<VT>::value)>
PointerVector<Type, AF, PF, TF>::assign(const blaze::DenseVector<VT, TF>& rhs) {
  BLAZE_INTERNAL_ASSERT(size_ == (~rhs).size(), "Invalid vector sizes");

  const size_t ipos(size_ & size_t(-2));
  BLAZE_INTERNAL_ASSERT((size_ - (size_ % 2UL)) == ipos,
                        "Invalid end calculation");

  for (size_t i = 0UL; i < ipos; i += 2UL) {
    // clang-tidy: do not use pointer arithmetic
    v_[i] = (~rhs)[i];              // NOLINT
    v_[i + 1UL] = (~rhs)[i + 1UL];  // NOLINT
  }
  if (ipos < (~rhs).size()) {
    // clang-tidy: do not use pointer arithmetic
    v_[ipos] = (~rhs)[ipos];  // NOLINT
  }
}

template <typename Type, bool AF, bool PF, bool TF>
template <typename VT>
inline std::enable_if_t<(PointerVector<Type, AF, PF, TF>::BLAZE_TEMPLATE
                             VectorizedAssign<VT>::value)>
PointerVector<Type, AF, PF, TF>::assign(const blaze::DenseVector<VT, TF>& rhs) {
  BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE(Type);

  BLAZE_INTERNAL_ASSERT(size_ == (~rhs).size(), "Invalid vector sizes");

  const size_t ipos(size_ & size_t(-SIMDSIZE));
  BLAZE_INTERNAL_ASSERT((size_ - (size_ % SIMDSIZE)) == ipos,
                        "Invalid end calculation");

  if (AF && blaze::useStreaming &&
      size_ > (blaze::cacheSize / (sizeof(Type) * 3UL)) &&
      !(~rhs).isAliased(this)) {
    size_t i(0UL);

    for (; i < ipos; i += SIMDSIZE) {
      stream(i, (~rhs).load(i));
    }
    for (; i < size_; ++i) {
      // clang-tidy: do not use pointer arithmetic
      v_[i] = (~rhs)[i];  // NOLINT
    }
  } else {
    const size_t i4way(size_ & size_t(-SIMDSIZE * 4));
    BLAZE_INTERNAL_ASSERT((size_ - (size_ % (SIMDSIZE * 4UL))) == i4way,
                          "Invalid end calculation");
    BLAZE_INTERNAL_ASSERT(i4way <= ipos, "Invalid end calculation");

    size_t i(0UL);
    blaze::ConstIterator_<VT> it((~rhs).begin());

    for (; i < i4way; i += SIMDSIZE * 4UL) {
      store(i, it.load());
      it += SIMDSIZE;
      store(i + SIMDSIZE, it.load());
      it += SIMDSIZE;
      store(i + SIMDSIZE * 2UL, it.load());
      it += SIMDSIZE;
      store(i + SIMDSIZE * 3UL, it.load());
      it += SIMDSIZE;
    }
    for (; i < ipos; i += SIMDSIZE, it += SIMDSIZE) {
      store(i, it.load());
    }
    for (; i < size_; ++i, ++it) {
      // clang-tidy: do not use pointer arithmetic
      v_[i] = *it;  // NOLINT
    }
  }
}

template <typename Type, bool AF, bool PF, bool TF>
template <typename VT>
inline std::enable_if_t<
    not(PointerVector<Type, AF, PF, TF>::template PointerVector<
        Type, AF, PF, TF>::BLAZE_TEMPLATE VectorizedAddAssign<VT>::value)>
PointerVector<Type, AF, PF, TF>::addAssign(
    const blaze::DenseVector<VT, TF>& rhs) {
  BLAZE_INTERNAL_ASSERT(size_ == (~rhs).size(), "Invalid vector sizes");

  const size_t ipos(size_ & size_t(-2));
  BLAZE_INTERNAL_ASSERT((size_ - (size_ % 2UL)) == ipos,
                        "Invalid end calculation");

  for (size_t i = 0UL; i < ipos; i += 2UL) {
    // clang-tidy: do not use pointer arithmetic
    v_[i] += (~rhs)[i];              // NOLINT
    v_[i + 1UL] += (~rhs)[i + 1UL];  // NOLINT
  }
  if (ipos < (~rhs).size()) {
    // clang-tidy: do not use pointer arithmetic
    v_[ipos] += (~rhs)[ipos];  // NOLINT
  }
}

template <typename Type, bool AF, bool PF, bool TF>
template <typename VT>
inline std::enable_if_t<(PointerVector<Type, AF, PF, TF>::BLAZE_TEMPLATE
                             VectorizedAddAssign<VT>::value)>
PointerVector<Type, AF, PF, TF>::addAssign(
    const blaze::DenseVector<VT, TF>& rhs) {
  BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE(Type);

  BLAZE_INTERNAL_ASSERT(size_ == (~rhs).size(), "Invalid vector sizes");

  const size_t ipos(size_ & size_t(-SIMDSIZE));
  BLAZE_INTERNAL_ASSERT((size_ - (size_ % SIMDSIZE)) == ipos,
                        "Invalid end calculation");

  const size_t i4way(size_ & size_t(-SIMDSIZE * 4));
  BLAZE_INTERNAL_ASSERT((size_ - (size_ % (SIMDSIZE * 4UL))) == i4way,
                        "Invalid end calculation");
  BLAZE_INTERNAL_ASSERT(i4way <= ipos, "Invalid end calculation");

  size_t i(0UL);
  blaze::ConstIterator_<VT> it((~rhs).begin());

  for (; i < i4way; i += SIMDSIZE * 4UL) {
    store(i, load(i) + it.load());
    it += SIMDSIZE;
    store(i + SIMDSIZE, load(i + SIMDSIZE) + it.load());
    it += SIMDSIZE;
    store(i + SIMDSIZE * 2UL, load(i + SIMDSIZE * 2UL) + it.load());
    it += SIMDSIZE;
    store(i + SIMDSIZE * 3UL, load(i + SIMDSIZE * 3UL) + it.load());
    it += SIMDSIZE;
  }
  for (; i < ipos; i += SIMDSIZE, it += SIMDSIZE) {
    store(i, load(i) + it.load());
  }
  for (; i < size_; ++i, ++it) {
    // clang-tidy: do not use pointer arithmetic
    v_[i] += *it;  // NOLINT
  }
}

template <typename Type, bool AF, bool PF, bool TF>
template <typename VT>
inline void PointerVector<Type, AF, PF, TF>::addAssign(
    const blaze::SparseVector<VT, TF>& rhs) {
  BLAZE_INTERNAL_ASSERT(size_ == (~rhs).size(), "Invalid vector sizes");

  for (blaze::ConstIterator_<VT> element = (~rhs).begin();
       element != (~rhs).end(); ++element) {
    v_[element->index()] += element->value();
  }
}

template <typename Type, bool AF, bool PF, bool TF>
template <typename VT>
inline std::enable_if_t<
    not(PointerVector<Type, AF, PF, TF>::template PointerVector<
        Type, AF, PF, TF>::BLAZE_TEMPLATE VectorizedSubAssign<VT>::value)>
PointerVector<Type, AF, PF, TF>::subAssign(
    const blaze::DenseVector<VT, TF>& rhs) {
  BLAZE_INTERNAL_ASSERT(size_ == (~rhs).size(), "Invalid vector sizes");

  const size_t ipos(size_ & size_t(-2));
  BLAZE_INTERNAL_ASSERT((size_ - (size_ % 2UL)) == ipos,
                        "Invalid end calculation");

  for (size_t i = 0UL; i < ipos; i += 2UL) {
    // clang-tidy: do not use pointer arithmetic
    v_[i] -= (~rhs)[i];              // NOLINT
    v_[i + 1UL] -= (~rhs)[i + 1UL];  // NOLINT
  }
  if (ipos < (~rhs).size()) {
    // clang-tidy: do not use pointer arithmetic
    v_[ipos] -= (~rhs)[ipos];  // NOLINT
  }
}

template <typename Type, bool AF, bool PF, bool TF>
template <typename VT>
inline std::enable_if_t<(PointerVector<Type, AF, PF, TF>::BLAZE_TEMPLATE
                             VectorizedSubAssign<VT>::value)>
PointerVector<Type, AF, PF, TF>::subAssign(
    const blaze::DenseVector<VT, TF>& rhs) {
  BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE(Type);

  BLAZE_INTERNAL_ASSERT(size_ == (~rhs).size(), "Invalid vector sizes");

  const size_t ipos(size_ & size_t(-SIMDSIZE));
  BLAZE_INTERNAL_ASSERT((size_ - (size_ % SIMDSIZE)) == ipos,
                        "Invalid end calculation");

  const size_t i4way(size_ & size_t(-SIMDSIZE * 4));
  BLAZE_INTERNAL_ASSERT((size_ - (size_ % (SIMDSIZE * 4UL))) == i4way,
                        "Invalid end calculation");
  BLAZE_INTERNAL_ASSERT(i4way <= ipos, "Invalid end calculation");

  size_t i(0UL);
  blaze::ConstIterator_<VT> it((~rhs).begin());

  for (; i < i4way; i += SIMDSIZE * 4UL) {
    store(i, load(i) - it.load());
    it += SIMDSIZE;
    store(i + SIMDSIZE, load(i + SIMDSIZE) - it.load());
    it += SIMDSIZE;
    store(i + SIMDSIZE * 2UL, load(i + SIMDSIZE * 2UL) - it.load());
    it += SIMDSIZE;
    store(i + SIMDSIZE * 3UL, load(i + SIMDSIZE * 3UL) - it.load());
    it += SIMDSIZE;
  }
  for (; i < ipos; i += SIMDSIZE, it += SIMDSIZE) {
    store(i, load(i) - it.load());
  }
  for (; i < size_; ++i, ++it) {
    // clang-tidy: do not use pointer arithmetic
    v_[i] -= *it;  // NOLINT
  }
}

template <typename Type, bool AF, bool PF, bool TF>
template <typename VT>
inline void PointerVector<Type, AF, PF, TF>::subAssign(
    const blaze::SparseVector<VT, TF>& rhs) {
  BLAZE_INTERNAL_ASSERT(size_ == (~rhs).size(), "Invalid vector sizes");

  for (blaze::ConstIterator_<VT> element = (~rhs).begin();
       element != (~rhs).end(); ++element) {
    v_[element->index()] -= element->value();
  }
}

template <typename Type, bool AF, bool PF, bool TF>
template <typename VT>
inline std::enable_if_t<
    not(PointerVector<Type, AF, PF, TF>::template PointerVector<
        Type, AF, PF, TF>::BLAZE_TEMPLATE VectorizedMultAssign<VT>::value)>
PointerVector<Type, AF, PF, TF>::multAssign(
    const blaze::DenseVector<VT, TF>& rhs) {
  BLAZE_INTERNAL_ASSERT(size_ == (~rhs).size(), "Invalid vector sizes");

  const size_t ipos(size_ & size_t(-2));
  BLAZE_INTERNAL_ASSERT((size_ - (size_ % 2UL)) == ipos,
                        "Invalid end calculation");

  for (size_t i = 0UL; i < ipos; i += 2UL) {
    // clang-tidy: do not use pointer arithmetic
    v_[i] *= (~rhs)[i];              // NOLINT
    v_[i + 1UL] *= (~rhs)[i + 1UL];  // NOLINT
  }
  if (ipos < (~rhs).size()) {
    // clang-tidy: do not use pointer arithmetic
    v_[ipos] *= (~rhs)[ipos];  // NOLINT
  }
}

template <typename Type, bool AF, bool PF, bool TF>
template <typename VT>
inline std::enable_if_t<(PointerVector<Type, AF, PF, TF>::BLAZE_TEMPLATE
                             VectorizedMultAssign<VT>::value)>
PointerVector<Type, AF, PF, TF>::multAssign(
    const blaze::DenseVector<VT, TF>& rhs) {
  BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE(Type);

  BLAZE_INTERNAL_ASSERT(size_ == (~rhs).size(), "Invalid vector sizes");

  const size_t ipos(size_ & size_t(-SIMDSIZE));
  BLAZE_INTERNAL_ASSERT((size_ - (size_ % SIMDSIZE)) == ipos,
                        "Invalid end calculation");

  const size_t i4way(size_ & size_t(-SIMDSIZE * 4));
  BLAZE_INTERNAL_ASSERT((size_ - (size_ % (SIMDSIZE * 4UL))) == i4way,
                        "Invalid end calculation");
  BLAZE_INTERNAL_ASSERT(i4way <= ipos, "Invalid end calculation");

  size_t i(0UL);
  blaze::ConstIterator_<VT> it((~rhs).begin());

  for (; i < i4way; i += SIMDSIZE * 4UL) {
    store(i, load(i) * it.load());
    it += SIMDSIZE;
    store(i + SIMDSIZE, load(i + SIMDSIZE) * it.load());
    it += SIMDSIZE;
    store(i + SIMDSIZE * 2UL, load(i + SIMDSIZE * 2UL) * it.load());
    it += SIMDSIZE;
    store(i + SIMDSIZE * 3UL, load(i + SIMDSIZE * 3UL) * it.load());
    it += SIMDSIZE;
  }
  for (; i < ipos; i += SIMDSIZE, it += SIMDSIZE) {
    store(i, load(i) * it.load());  // LCOV_EXCL_LINE
  }
  for (; i < size_; ++i, ++it) {
    // clang-tidy: do not use pointer arithmetic
    v_[i] *= *it;  // NOLINT
  }
}

template <typename Type, bool AF, bool PF, bool TF>
template <typename VT>
inline void PointerVector<Type, AF, PF, TF>::multAssign(
    const blaze::SparseVector<VT, TF>& rhs) {
  BLAZE_INTERNAL_ASSERT(size_ == (~rhs).size(), "Invalid vector sizes");

  const blaze::DynamicVector<Type, TF> tmp(serial(*this));
  reset();
  for (blaze::ConstIterator_<VT> element = (~rhs).begin();
       element != (~rhs).end(); ++element) {
    v_[element->index()] = tmp[element->index()] * element->value();
  }
}

template <typename Type, bool AF, bool PF, bool TF>
template <typename VT>
inline std::enable_if_t<
    not(PointerVector<Type, AF, PF, TF>::template PointerVector<
        Type, AF, PF, TF>::BLAZE_TEMPLATE VectorizedDivAssign<VT>::value)>
PointerVector<Type, AF, PF, TF>::divAssign(
    const blaze::DenseVector<VT, TF>& rhs) {
  BLAZE_INTERNAL_ASSERT(size_ == (~rhs).size(), "Invalid vector sizes");

  const size_t ipos(size_ & size_t(-2));
  BLAZE_INTERNAL_ASSERT((size_ - (size_ % 2UL)) == ipos,
                        "Invalid end calculation");

  for (size_t i = 0UL; i < ipos; i += 2UL) {
    // clang-tidy: do not use pointer arithmetic
    v_[i] /= (~rhs)[i];              // NOLINT
    v_[i + 1UL] /= (~rhs)[i + 1UL];  // NOLINT
  }
  if (ipos < (~rhs).size()) {
    // clang-tidy: do not use pointer arithmetic
    v_[ipos] /= (~rhs)[ipos];  // NOLINT
  }
}

template <typename Type, bool AF, bool PF, bool TF>
template <typename VT>
inline std::enable_if_t<(PointerVector<Type, AF, PF, TF>::BLAZE_TEMPLATE
                             VectorizedDivAssign<VT>::value)>
PointerVector<Type, AF, PF, TF>::divAssign(
    const blaze::DenseVector<VT, TF>& rhs) {
  BLAZE_CONSTRAINT_MUST_BE_VECTORIZABLE_TYPE(Type);

  BLAZE_INTERNAL_ASSERT(size_ == (~rhs).size(), "Invalid vector sizes");

  const size_t ipos(size_ & size_t(-SIMDSIZE));
  BLAZE_INTERNAL_ASSERT((size_ - (size_ % SIMDSIZE)) == ipos,
                        "Invalid end calculation");

  const size_t i4way(size_ & size_t(-SIMDSIZE * 4));
  BLAZE_INTERNAL_ASSERT((size_ - (size_ % (SIMDSIZE * 4UL))) == i4way,
                        "Invalid end calculation");
  BLAZE_INTERNAL_ASSERT(i4way <= ipos, "Invalid end calculation");

  size_t i(0UL);
  blaze::ConstIterator_<VT> it((~rhs).begin());

  for (; i < i4way; i += SIMDSIZE * 4UL) {
    store(i, load(i) / it.load());
    it += SIMDSIZE;
    store(i + SIMDSIZE, load(i + SIMDSIZE) / it.load());
    it += SIMDSIZE;
    store(i + SIMDSIZE * 2UL, load(i + SIMDSIZE * 2UL) / it.load());
    it += SIMDSIZE;
    store(i + SIMDSIZE * 3UL, load(i + SIMDSIZE * 3UL) / it.load());
    it += SIMDSIZE;
  }
  for (; i < ipos; i += SIMDSIZE, it += SIMDSIZE) {
    store(i, load(i) / it.load());  // LCOV_EXCL_LINE
  }
  for (; i < size_; ++i, ++it) {
    // clang-tidy: do not use pointer arithmetic
    v_[i] /= *it;  // NOLINT
  }
}
/// \endcond
