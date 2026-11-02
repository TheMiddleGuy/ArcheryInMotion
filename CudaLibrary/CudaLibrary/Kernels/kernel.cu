// kernel.cu : GPU work, written with Thrust.

#include "kernel.cuh"
#include "ThrustUtils.cuh"

#include <thrust/copy.h>
#include <thrust/device_vector.h>
#include <thrust/transform.h>

namespace cudalib::kernels
{
    cudaError_t VectorAdd(int* c, const int* a, const int* b, int size)
    {
        // Runs on the GPU for every element. __host__ __device__ lets Thrust work out
        // the return type on the CPU side as well (needs nvcc's --extended-lambda flag).
        const auto add = [] __host__ __device__(int x, int y) { return x + y; };

        return detail::RunGuarded([&] {
            // Copy the inputs to the GPU. device_vector frees its memory automatically.
            const thrust::device_vector<int> devA(a, a + size);
            const thrust::device_vector<int> devB(b, b + size);
            thrust::device_vector<int> devC(size);

            // c[i] = a[i] + b[i]; Thrust picks the grid and block sizes.
            thrust::transform(devA.begin(), devA.end(), devB.begin(), devC.begin(), add);

            // Copy the result back to the CPU. This waits for the GPU to finish.
            thrust::copy(devC.begin(), devC.end(), c);
        });
    }
}
