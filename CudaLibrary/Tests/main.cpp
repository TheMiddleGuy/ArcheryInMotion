// Smoke test for CudaLibrary.dll: prints the GPU and checks a vector add against the CPU.

#include "CudaLibrary.h"

#include <cstdio>
#include <vector>

int main()
{
    const int version = CudaLib_GetVersion();
    std::printf("CudaLibrary %d.%d.%d\n", version / 10000, (version / 100) % 100, version % 100);

    CudaLibDeviceInfo info{};
    if (CudaLib_GetDeviceInfo(0, &info) != CUDALIB_OK)
    {
        std::printf("No usable GPU: %s\n", CudaLib_GetLastErrorMessage());
        return 1;
    }
    std::printf("GPU 0: %s, compute %d.%d, %d SMs, %.1f GB\n", info.name, info.computeCapabilityMajor,
                info.computeCapabilityMinor, info.multiProcessorCount,
                static_cast<double>(info.totalMemoryBytes) / (1024.0 * 1024.0 * 1024.0));

    // Large enough to need many blocks, which the old launch configuration could not do.
    const int size = 1'000'000;
    std::vector<int> a(size), b(size), c(size, 0);
    for (int i = 0; i < size; ++i)
    {
        a[i] = i;
        b[i] = 2 * i;
    }

    if (CudaLib_VectorAdd(c.data(), a.data(), b.data(), size) != CUDALIB_OK)
    {
        std::printf("VectorAdd failed: %s\n", CudaLib_GetLastErrorMessage());
        return 1;
    }

    for (int i = 0; i < size; ++i)
    {
        if (c[i] != a[i] + b[i])
        {
            std::printf("Mismatch at %d: got %d, expected %d\n", i, c[i], a[i] + b[i]);
            return 1;
        }
    }

    std::printf("VectorAdd of %d elements: OK\n", size);
    return 0;
}
