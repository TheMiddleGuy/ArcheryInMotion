// CudaLibrary.cpp : implementation of the public C API.
// Validates arguments, calls into the kernels and turns CUDA errors into readable messages.

#include "CudaLibrary.h"
#include "Kernels/kernel.cuh"

#include <cuda_runtime.h>

#include <cstdio>
#include <string>

namespace
{
    thread_local std::string g_lastError;

    CudaLibResult Fail(CudaLibResult code, std::string message)
    {
        g_lastError = std::move(message);
        return code;
    }

    CudaLibResult FromCuda(cudaError_t error, const char* where)
    {
        if (error == cudaSuccess)
            return CUDALIB_OK;

        if (error == cudaErrorNoDevice || error == cudaErrorInsufficientDriver)
            return Fail(CUDALIB_ERROR_NO_DEVICE, std::string(where) + ": " + cudaGetErrorString(error));

        return Fail(CUDALIB_ERROR_CUDA,
                    std::string(where) + ": " + cudaGetErrorName(error) + " - " + cudaGetErrorString(error));
    }
}

extern "C" {

int CudaLib_GetVersion(void)
{
    return CUDALIB_VERSION_MAJOR * 10000 + CUDALIB_VERSION_MINOR * 100 + CUDALIB_VERSION_PATCH;
}

const char* CudaLib_GetLastErrorMessage(void)
{
    return g_lastError.c_str();
}

int CudaLib_GetDeviceCount(void)
{
    int count = 0;
    if (cudaGetDeviceCount(&count) != cudaSuccess)
    {
        cudaGetLastError(); // clear the sticky error so later calls are not affected
        return 0;
    }
    return count;
}

CudaLibResult CudaLib_GetDeviceInfo(int deviceIndex, CudaLibDeviceInfo* outInfo)
{
    g_lastError.clear();

    if (outInfo == nullptr)
        return Fail(CUDALIB_ERROR_INVALID_ARGUMENT, "CudaLib_GetDeviceInfo: outInfo is null");

    const int count = CudaLib_GetDeviceCount();
    if (count == 0)
        return Fail(CUDALIB_ERROR_NO_DEVICE, "CudaLib_GetDeviceInfo: no CUDA-capable GPU found");
    if (deviceIndex < 0 || deviceIndex >= count)
        return Fail(CUDALIB_ERROR_INVALID_ARGUMENT, "CudaLib_GetDeviceInfo: deviceIndex out of range");

    cudaDeviceProp props{};
    if (const CudaLibResult r = FromCuda(cudaGetDeviceProperties(&props, deviceIndex), "cudaGetDeviceProperties");
        r != CUDALIB_OK)
        return r;

    std::snprintf(outInfo->name, sizeof(outInfo->name), "%s", props.name);
    outInfo->computeCapabilityMajor = props.major;
    outInfo->computeCapabilityMinor = props.minor;
    outInfo->multiProcessorCount = props.multiProcessorCount;
    outInfo->totalMemoryBytes = static_cast<uint64_t>(props.totalGlobalMem);
    return CUDALIB_OK;
}

CudaLibResult CudaLib_VectorAdd(int* c, const int* a, const int* b, int size)
{
    g_lastError.clear();

    if (c == nullptr || a == nullptr || b == nullptr)
        return Fail(CUDALIB_ERROR_INVALID_ARGUMENT, "CudaLib_VectorAdd: null pointer");
    if (size < 0)
        return Fail(CUDALIB_ERROR_INVALID_ARGUMENT, "CudaLib_VectorAdd: negative size");
    if (size == 0)
        return CUDALIB_OK;

    return FromCuda(cudalib::kernels::VectorAdd(c, a, b, size), "CudaLib_VectorAdd");
}

} // extern "C"
