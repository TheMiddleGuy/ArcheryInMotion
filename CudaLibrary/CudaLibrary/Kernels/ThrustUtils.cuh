// ThrustUtils.cuh : helpers shared by all Thrust-based GPU code in this library.

#pragma once

#include <cuda_runtime.h>

#include <thrust/system/cuda/error.h>
#include <thrust/system_error.h>

#include <new>
#include <utility>

namespace cudalib::detail
{
    // Runs a block of Thrust code and turns any exception it throws into a cudaError_t.
    //
    // Thrust reports failures with C++ exceptions (thrust::system_error, std::bad_alloc).
    // Those must never escape the DLL: the public API is plain C, and Unreal builds its
    // own code with exceptions disabled. Wrap every Thrust call sequence in this.
    //
    //     return RunGuarded([&] {
    //         thrust::device_vector<float> d(hostPtr, hostPtr + n);
    //         ...
    //     });
    template <typename Fn>
    cudaError_t RunGuarded(Fn&& fn) noexcept
    {
        try
        {
            std::forward<Fn>(fn)();
            return cudaGetLastError();
        }
        catch (const thrust::system_error& e)
        {
            if (e.code().category() == thrust::system::cuda_category())
                return static_cast<cudaError_t>(e.code().value());
            return cudaErrorUnknown;
        }
        catch (const std::bad_alloc&)
        {
            return cudaErrorMemoryAllocation;
        }
        catch (...)
        {
            return cudaErrorUnknown;
        }
    }
}
