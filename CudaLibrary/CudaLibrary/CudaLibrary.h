// CudaLibrary.h : public API of the CUDA library.
//
// This is a plain C interface on purpose: Unreal Engine loads the DLL and calls these
// functions, and a C interface does not break when the two sides are built with
// different compiler settings or C++ runtimes. No CUDA types appear here, so Unreal
// does not need the CUDA headers.

#pragma once

#include <stdint.h>

#if defined(_WIN32)
    #if defined(CUDALIB_BUILD_DLL)
        #define CUDALIB_API __declspec(dllexport)
    #else
        #define CUDALIB_API __declspec(dllimport)
    #endif
#else
    #define CUDALIB_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum CudaLibResult
{
    CUDALIB_OK = 0,
    CUDALIB_ERROR_INVALID_ARGUMENT = 1,
    CUDALIB_ERROR_NO_DEVICE = 2,
    CUDALIB_ERROR_CUDA = 3
} CudaLibResult;

typedef struct CudaLibDeviceInfo
{
    char name[256];
    int computeCapabilityMajor;
    int computeCapabilityMinor;
    int multiProcessorCount;
    uint64_t totalMemoryBytes;
} CudaLibDeviceInfo;

// Library version encoded as major * 10000 + minor * 100 + patch.
CUDALIB_API int CudaLib_GetVersion(void);

// Description of the error from the most recent failed call on this thread.
// The pointer stays valid until the next library call on the same thread.
CUDALIB_API const char* CudaLib_GetLastErrorMessage(void);

// Number of CUDA-capable GPUs (0 if none or no driver).
CUDALIB_API int CudaLib_GetDeviceCount(void);

CUDALIB_API CudaLibResult CudaLib_GetDeviceInfo(int deviceIndex, CudaLibDeviceInfo* outInfo);

// c[i] = a[i] + b[i] for i in [0, size). All three pointers are host (CPU) memory.
CUDALIB_API CudaLibResult CudaLib_VectorAdd(int* c, const int* a, const int* b, int size);

#ifdef __cplusplus
}
#endif
