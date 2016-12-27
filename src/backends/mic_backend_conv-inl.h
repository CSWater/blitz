#ifndef SRC_BACKENDS_MIC_BACKEND_CONV_INL_H_
#define SRC_BACKENDS_MIC_BACKEND_CONV_INL_H_

template<typename DType>
void Backend<MICTensor, DType>::Convolution2DForwardFunc(
  const MICTensor<DType>* input,
  const MICTensor<DType>* filter,
  MICTensor<DType>* output,
  MICTensor<DType>* workspace,
  size_t padding_height,
  size_t padding_width,
  size_t stride_height,
  size_t stride_width,
  BLITZ_ALGORITHM algorithm) {
  //decode the shape
  //get N, H, W, C, K, R, S, buffer_layout, filter_layout
  size_t NIN, C, H, W;
  size_t KF, CF, R, S;
  size_t NOUT, K, P, Q;
  Blitz2DBuffer(input->data_layout(), input->shape_ptr(), &NIN, &C, &H, &W);
  Blitz2DFilter(filter->data_layout(), filter->shape_ptr(), &KF, &CF, &R, &S);
  Blitz2DBuffer(output->data_layout(), output->shape_ptr(), &NOUT, &K, &P, &Q);
  CHECK_EQ(NIN, NOUT);
  CHECK_EQ(KF, K);
  CHECK_EQ(CF, C);

  // time counter
  #ifdef BLITZ_PERFORMANCE
  time_point<system_clock> start, end;
  duration<double> gemm_time = duration<double>::zero();
  duration<double> unpack_time = duration<double>::zero();
  double total_gemm_time = 0.0;
  double total_unpack_time = 0.0;
  #endif  // BLITZ_PERFORMANCE
  switch (algorithm) {
    case BLITZ_CONVOLUTION_XSMM_DIRECT:
      // NOTE(keren): it only support output padding
      XsmmBuffer xsmmBuffer;
      xsmmBuffer = BlitzXsmmPrepare2D(
        const_cast<MICTensor<DType>*>(input)->data(),
        output->data(),
        const_cast<MICTensor<DType>*>(filter)->data(),
        input->data_layout(), filter->data_layout(),
        NIN, H, W, C, K, R, S,
        stride_height, stride_width,
        padding_height, padding_width);
 
      #pragma omp parallel 
      {
        const size_t tid = omp_get_thread_num();
        CHKERR_LIBXSMM_DNN(libxsmm_dnn_convolve_st(xsmmBuffer.libxsmm_handle, LIBXSMM_DNN_CONV_KIND_FWD, 0, tid)); 
      }
        // CHKERR_LIBXSMM_DNN(libxsmm_dnn_copyout_buffer(xsmmBuffer.libxsmm_output, output->data(), LIBXSMM_DNN_CONV_FORMAT_NCHW));
      break;
    default:
      LOG(FATAL) << "Unupported algorithm type: " << algorithm;
      break;
  }
  #ifdef BLITZ_PERFORMANCE
  LOG(INFO) << "Forward convolution gemm: " << total_gemm_time;
  LOG(INFO) << "Forward convolution unpack: " << total_unpack_time;
  #endif  // BLITZ_PERFORMANCE
}

template<typename DType>
void Backend<MICTensor, DType>::Convolution2DBackwardFunc(
  const MICTensor<DType>* output,
  const MICTensor<DType>* filter,
  MICTensor<DType>* input,
  MICTensor<DType>* workspace,
  size_t padding_height,
  size_t padding_width,
  size_t stride_height,
  size_t stride_width,
  BLITZ_ALGORITHM algorithm){
  //decode the shape
  //get N, H, W, C, K, R, S, buffer_layout, filter_layout
  size_t NIN, C, H, W;
  size_t KF, CF, R, S;
  size_t NOUT, K, P, Q;
  Blitz2DBuffer(input->data_layout(), input->shape_ptr(), &NIN, &C, &H, &W);
  Blitz2DFilter(filter->data_layout(), filter->shape_ptr(), &KF, &CF, &R, &S);
  Blitz2DBuffer(output->data_layout(), output->shape_ptr(), &NOUT, &K, &P, &Q);
  CHECK_EQ(NIN, NOUT);
  CHECK_EQ(KF, K);
  CHECK_EQ(CF, C);

  #ifdef BLITZ_PERFORMANCE
  time_point<system_clock> start, end;
  duration<double> gemm_time = duration<double>::zero();
  duration<double> unpack_time = duration<double>::zero();
  double total_gemm_time = 0.0;
  double total_unpack_time = 0.0;
  #endif  // BLITZ_PERFORMANCE
  switch (algorithm) {
    case BLITZ_CONVOLUTION_XSMM_DIRECT:
      // NOTE(keren): it only support output padding
      XsmmBuffer xsmmBuffer;
      xsmmBuffer = BlitzXsmmPrepare2D(
        input->data(),
        const_cast<MICTensor<DType>*>(output)->data(),
        const_cast<MICTensor<DType>*>(filter)->data(),
        input->data_layout(), filter->data_layout(),
        NIN, H, W, C, K, R, S,
        stride_height, stride_width,
        padding_height, padding_width);
  //    #pragma omp parallel 
      {
//        const size_t tid = omp_get_thread_num();
        const size_t tid = 0;
        CHKERR_LIBXSMM_DNN(libxsmm_dnn_convolve_st(xsmmBuffer.libxsmm_handle, LIBXSMM_DNN_CONV_KIND_BWD, 0, tid)); 
      }
      break;
    default:
      LOG(FATAL) << "Unupported algorithm type: " << algorithm;
      break;
  }
  #ifdef BLITZ_PERFORMANCE
  LOG(INFO) << "Forward convolution gemm: " << total_gemm_time;
  LOG(INFO) << "Forward convolution unpack: " << total_unpack_time;
  #endif  // BLITZ_PERFORMANCE
}

template<typename DType>
void Backend<MICTensor, DType>::Convolution2DUpdateFunc(
  const MICTensor<DType>* input,
  const MICTensor<DType>* output,
  MICTensor<DType>* filter,
  MICTensor<DType>* workspace,
  size_t padding_height,
  size_t padding_width,
  size_t stride_height,
  size_t stride_width,
  BLITZ_ALGORITHM algorithm){
  //get N, H, W, C, K, R, S, buffer_layout, filter_layout
  size_t NIN, C, H, W;
  size_t KF, CF, R, S;
  size_t NOUT, K, P, Q;
  Blitz2DBuffer(input->data_layout(), input->shape_ptr(), &NIN, &C, &H, &W);
  Blitz2DFilter(filter->data_layout(), filter->shape_ptr(), &KF, &CF, &R, &S);
  Blitz2DBuffer(output->data_layout(), output->shape_ptr(), &NOUT, &K, &P, &Q);
  CHECK_EQ(NIN, NOUT);
  CHECK_EQ(KF, K);
  CHECK_EQ(CF, C);

  #ifdef BLITZ_PERFORMANCE
  time_point<system_clock> start, end;
  duration<double> gemm_time = duration<double>::zero();
  duration<double> unpack_time = duration<double>::zero();
  double total_gemm_time = 0.0;
  double total_unpack_time = 0.0;
  #endif  // BLITZ_PERFORMANCE
  switch (algorithm) {
    case BLITZ_CONVOLUTION_XSMM_DIRECT:
      // NOTE(keren): it only support output padding
      XsmmBuffer xsmmBuffer;
      xsmmBuffer = BlitzXsmmPrepare2D(
        const_cast<MICTensor<DType>*>(input)->data(),
        const_cast<MICTensor<DType>*>(output)->data(),
        filter->data(),
        input->data_layout(), filter->data_layout(),
        NIN, H, W, C, K, R, S,
        stride_height, stride_width,
        padding_height, padding_width);
      #pragma omp parallel 
      {
        const size_t tid = omp_get_thread_num();
        CHKERR_LIBXSMM_DNN(libxsmm_dnn_convolve_st(xsmmBuffer.libxsmm_handle, LIBXSMM_DNN_CONV_KIND_UPD, 0, tid)); 
      }
      break;
    default:
      LOG(FATAL) << "Unupported algorithm type: " << algorithm;
      break;
  }
  #ifdef BLITZ_PERFORMANCE
  LOG(INFO) << "Forward convolution gemm: " << total_gemm_time;
  LOG(INFO) << "Forward convolution unpack: " << total_unpack_time;
  #endif  // BLITZ_PERFORMANCE
}


#endif  // SRC_BACKENDS_MIC_BACKEND_CONV_INL_H_
