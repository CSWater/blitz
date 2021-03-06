#include <iostream>

#include <cuda.h>
#include <cuda_runtime_api.h>

#include "backends/backends.h"
#include "utils/blitz_gpu_function.h"
#include "utils/blitz_cpu_function.h"
#include "utils/blitz_algorithm_function.h"

using namespace blitz;

// N C H W
Shape input_shape(4, BLITZ_BUFFER_NCHW);
// K C R S
Shape filter_shape(4, BLITZ_FILTER_KCRS);
// N K P Q
Shape output_shape(4, BLITZ_BUFFER_NCHW);
// cpu workspace
Shape workspace_shape_cpu(1);
// gpu workspace
Shape workspace_shape_gpu(1);
// init timer
float elapsed_time_gpu;
CUevent event_start, event_stop;

void set_input_shape_nchw(size_t N, size_t C, size_t H, size_t W) {
  input_shape[0] = N;
  input_shape[1] = C;
  input_shape[2] = H;
  input_shape[3] = W;
}

void set_filter_shape_kcrs(size_t K, size_t C, size_t R, size_t S) {
  filter_shape[0] = K;
  filter_shape[1] = C;
  filter_shape[2] = R;
  filter_shape[3] = S;
}

void set_output_shape_nkpq(size_t N, size_t K, size_t P, size_t Q) {
  output_shape[0] = N;
  output_shape[1] = K;
  output_shape[2] = P;
  output_shape[3] = Q;
}

void compare_cpu_gpu(
  size_t size,
  float* output_cpu,
  float* output_gpu,
  float precision = 1e-2) {
  for (size_t i = 0; i < size; ++i) {
    if (output_cpu[i] > output_gpu[i] + precision ||
      output_cpu[i] < output_gpu[i] - precision) {
      std::cout << "Index: " << i << ", CPU: " << output_cpu[i] <<
        ", GPU: " << output_gpu[i] << std::endl;
    }
  }
}

// conmpare gpu result to cpu result
void convolution_forward(
  BLITZ_ALGORITHM algorithm,
  size_t pad_h, size_t pad_w,
  size_t str_h, size_t str_w,
  size_t iter) {
  // set up cpu
  CPUTensor<float> input_cpu(input_shape);
  CPUTensor<float> filter_cpu(filter_shape);
  CPUTensor<float> output_cpu(output_shape);
  CPUTensor<float> workspace_cpu(workspace_shape_cpu);
  // set up gpu
  GPUTensor<float> input_gpu(input_shape);
  GPUTensor<float> filter_gpu(filter_shape);
  GPUTensor<float> output_gpu(output_shape);
  GPUTensor<float> workspace_gpu(workspace_shape_gpu);
  // set up copy
  CPUTensor<float> output_copy(output_shape);
  // init values
  Backend<CPUTensor, float>::UniformDistributionFunc(&input_cpu, -0.1, 0.1);
  Backend<CPUTensor, float>::UniformDistributionFunc(&filter_cpu, -0.1, 0.1);
  cudaMemcpy(input_gpu.data(), input_cpu.data(),
    input_cpu.size() * sizeof(float), cudaMemcpyHostToDevice);
  cudaMemcpy(filter_gpu.data(), filter_cpu.data(),
    filter_cpu.size() * sizeof(float), cudaMemcpyHostToDevice);
  // cpu convolution 
  Backend<CPUTensor, float>::Convolution2DForwardFunc(
    &input_cpu,
    &filter_cpu,
    &output_cpu,
    &workspace_cpu,
    pad_h, pad_w, 
    str_h, str_w);
  Backend<GPUTensor, float>::Convolution2DForwardFunc(
    &input_gpu,
    &filter_gpu,
    &output_gpu,
    &workspace_gpu,
    pad_h, pad_w, 
    str_h, str_w,
    algorithm);
  BLITZ_GPU_TIMER_START(elapsed_time_gpu, event_start, event_stop);
  // gpu convolution
  for (size_t i = 1; i < iter; ++i) {
    Backend<GPUTensor, float>::Convolution2DForwardFunc(
      &input_gpu,
      &filter_gpu,
      &output_gpu,
      &workspace_gpu,
      pad_h, pad_w, 
      str_h, str_w,
      algorithm);
  }
  BLITZ_GPU_TIMER_END(elapsed_time_gpu, event_start, event_stop);
  BLITZ_GPU_TIMER_INFO((iter - 1) * 2 * filter_shape.size() * output_shape[0] * output_shape[2] * output_shape[3], elapsed_time_gpu);
  // copy from gpu to cpu
  cudaMemcpy(output_copy.data(), output_gpu.data(), output_gpu.size() * sizeof(float), cudaMemcpyDeviceToHost);
  compare_cpu_gpu(output_cpu.size(), output_cpu.data(), output_copy.data());
}

void convolution_backward(
  BLITZ_ALGORITHM algorithm,
  size_t pad_h, size_t pad_w,
  size_t str_h, size_t str_w,
  size_t iter) {
  // set up cpu
  CPUTensor<float> input_cpu(input_shape);
  CPUTensor<float> filter_cpu(filter_shape);
  CPUTensor<float> output_cpu(output_shape);
  CPUTensor<float> workspace_cpu(workspace_shape_cpu);
  // set up gpu
  GPUTensor<float> input_gpu(input_shape);
  GPUTensor<float> filter_gpu(filter_shape);
  GPUTensor<float> output_gpu(output_shape);
  GPUTensor<float> workspace_gpu(workspace_shape_gpu);
  // set up copy
  CPUTensor<float> input_copy(input_shape);
  // init values
  Backend<CPUTensor, float>::UniformDistributionFunc(&output_cpu, -0.1, 0.1);
  Backend<CPUTensor, float>::UniformDistributionFunc(&filter_cpu, -0.1, 0.1);
  cudaMemcpy(output_gpu.data(), output_cpu.data(),
    output_cpu.size() * sizeof(float), cudaMemcpyHostToDevice);
  cudaMemcpy(filter_gpu.data(), filter_cpu.data(),
    filter_cpu.size() * sizeof(float), cudaMemcpyHostToDevice);
  // cpu convolution 
  Backend<CPUTensor, float>::Convolution2DBackwardFunc(
    &output_cpu,
    &filter_cpu,
    &input_cpu,
    &workspace_cpu,
    pad_h, pad_w, 
    str_h, str_w);
  // gpu convolution
  Backend<GPUTensor, float>::Convolution2DBackwardFunc(
    &output_gpu,
    &filter_gpu,
    &input_gpu,
    &workspace_gpu,
    pad_h, pad_w, 
    str_h, str_w,
    algorithm);
  // copy from gpu to cpu
  cudaMemcpy(input_copy.data(), input_gpu.data(),
    input_gpu.size() * sizeof(float), cudaMemcpyDeviceToHost);
  compare_cpu_gpu(input_cpu.size(), input_cpu.data(), input_copy.data());
}

void convolution_update(
  BLITZ_ALGORITHM algorithm,
  size_t pad_h, size_t pad_w,
  size_t str_h, size_t str_w,
  size_t iter) {
  // set up cpu
  CPUTensor<float> input_cpu(input_shape);
  CPUTensor<float> filter_cpu(filter_shape);
  CPUTensor<float> output_cpu(output_shape);
  CPUTensor<float> workspace_cpu(workspace_shape_cpu);
  // set up gpu
  GPUTensor<float> input_gpu(input_shape);
  GPUTensor<float> filter_gpu(filter_shape);
  GPUTensor<float> output_gpu(output_shape);
  GPUTensor<float> workspace_gpu(workspace_shape_gpu);
  //// set up copy
  CPUTensor<float> filter_copy(filter_shape);
  //// init values
  Backend<CPUTensor, float>::UniformDistributionFunc(&output_cpu, -0.1, 0.1);
  Backend<CPUTensor, float>::UniformDistributionFunc(&input_cpu, -0.1, 0.1);
  cudaMemcpy(output_gpu.data(), output_cpu.data(),
    output_cpu.size() * sizeof(float), cudaMemcpyHostToDevice);
  cudaMemcpy(input_gpu.data(), input_cpu.data(),
    input_cpu.size() * sizeof(float), cudaMemcpyHostToDevice);
  // cpu convolution 
  Backend<CPUTensor, float>::Convolution2DUpdateFunc(
    &input_cpu,
    &output_cpu,
    &filter_cpu,
    &workspace_cpu,
    pad_h, pad_w, 
    str_h, str_w);
  // gpu convolution
  Backend<GPUTensor, float>::Convolution2DUpdateFunc(
    &input_gpu,
    &output_gpu,
    &filter_gpu,
    &workspace_gpu,
    pad_h, pad_w, 
    str_h, str_w,
    algorithm);
  // copy from gpu to cpu
  cudaMemcpy(filter_copy.data(), filter_gpu.data(),
    filter_gpu.size() * sizeof(float), cudaMemcpyDeviceToHost);
  compare_cpu_gpu(filter_cpu.size(), filter_cpu.data(), filter_copy.data(), 1e-2);
}

int main(int argc, char** argv) {
  const size_t NUM_ARGS = 16;
  // phase kernel N C H W R S K P Q pad_h pad_w str_h str_w
  if (argc != NUM_ARGS + 1) {
    std::cerr << "Not enough args!" << std::endl;
    exit(1);
  }
  // get args
  const std::string phase = std::string(argv[1]); 
  const std::string kernel = std::string(argv[2]); 
  const size_t N = atoi(argv[3]);
  const size_t C = atoi(argv[4]);
  const size_t H = atoi(argv[5]);
  const size_t W = atoi(argv[6]);
  const size_t R = atoi(argv[7]);
  const size_t S = atoi(argv[8]);
  const size_t K = atoi(argv[9]);
  const size_t P = atoi(argv[10]);
  const size_t Q = atoi(argv[11]);
  const size_t pad_h = atoi(argv[12]);
  const size_t pad_w = atoi(argv[13]);
  const size_t str_h = atoi(argv[14]);
  const size_t str_w = atoi(argv[15]);
  const size_t iterations = atoi(argv[16]);
  // set shapes
  set_input_shape_nchw(N, C, H, W);
  set_filter_shape_kcrs(K, C, R, S);
  set_output_shape_nkpq(N, K, P, Q);
  // set workspace shape
  workspace_shape_gpu[0] =
    input_shape.size() + output_shape.size() + filter_shape.size();
  workspace_shape_cpu[0] = C * R * S * P * Q;
  // run convolution
  if (phase == "forward") {
    convolution_forward(BlitzParseAlgorithm(kernel), pad_h, pad_w, str_h, str_w, iterations);
  } else if (phase == "backward") {
    convolution_backward(BlitzParseAlgorithm(kernel), pad_h, pad_w, str_h, str_w, iterations);
  } else if (phase == "update") {
    convolution_update(BlitzParseAlgorithm(kernel), pad_h, pad_w, str_h, str_w, iterations);
  }
  return 0;
}
