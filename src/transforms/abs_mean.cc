#include "transforms/abs_mean.h"

#include "backends/backends.h"

namespace blitz {

template<template <typename> class TensorType, typename DType>
DType AbsMean<TensorType, DType>::Apply(
  const shared_ptr<TensorType<DType> > output,
  const shared_ptr<TensorType<DType> > target) {
  return Backend<TensorType, DType>::AbsMeanApplyFunc(
    output.get(), target.get());
}

template<template <typename> class TensorType, typename DType>
void AbsMean<TensorType, DType>::Derivative(
  const shared_ptr<TensorType<DType> > output,
  const shared_ptr<TensorType<DType> > target,
  shared_ptr<TensorType<DType> > result) {
  Backend<TensorType, DType>::AbsMeanDerivativeFunc(
    output.get(), target.get(), result.get());
}

INSTANTIATE_CLASS_CPU(AbsMean);
#ifdef BLITZ_USE_MIC
  INSTANTIATE_CLASS_MIC(AbsMean);
#endif
#ifdef BLITZ_USE_GPU
  INSTANTIATE_CLASS_GPU(AbsMean);
#endif

}  // namespace blitz
