// kernel.cuh : internal declarations of the GPU work. Not part of the public API.

#pragma once

#include <cuda_runtime.h>

namespace cudalib::kernels
{
    // c = a + b, element-wise. Pointers are host memory; this function handles the
    // device allocations and copies. Returns the first CUDA error it hits.
    // Functions declared here must not throw: they are called from the C API.
    cudaError_t VectorAdd(int* c, const int* a, const int* b, int size);
}
