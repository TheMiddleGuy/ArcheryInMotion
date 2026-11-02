find_package(CUDAToolkit REQUIRED)

add_library(CudaLibrary SHARED
    CudaLibrary.h            # public C API, the only header Unreal includes
    CudaLibrary.cpp          # C API implementation (host code, compiled by cl.exe)
    Kernels/kernel.cuh       # internal declarations
    Kernels/ThrustUtils.cuh  # shared Thrust helpers (exception -> cudaError_t)
    Kernels/kernel.cu        # GPU work with Thrust (compiled by nvcc)
)

target_include_directories(CudaLibrary PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(CudaLibrary
    PRIVATE
        CUDALIB_BUILD_DLL  # switches CUDALIB_API to dllexport while building the DLL
        CUDALIB_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        CUDALIB_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        CUDALIB_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

# Thrust (part of CCCL) ships with the CUDA Toolkit and nvcc finds its headers on its own.
# --extended-lambda allows __device__ / __host__ __device__ lambdas, which keeps Thrust
# code short: no separate functor structs needed.
target_compile_options(CudaLibrary PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:--extended-lambda>)

# CCCL/Thrust headers require MSVC's standard-conforming preprocessor; cl.exe still
# defaults to the old one. nvcc forwards this to cl.exe for the host-side code.
if(MSVC)
    target_compile_options(CudaLibrary PRIVATE
        $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=/Zc:preprocessor>
        $<$<COMPILE_LANGUAGE:CXX>:/Zc:preprocessor>
    )
endif()

# Link the CUDA runtime statically, so Unreal only needs CudaLibrary.dll
# (plus the NVIDIA driver) and no cudart64_*.dll next to it.
set_target_properties(CudaLibrary PROPERTIES CUDA_RUNTIME_LIBRARY Static)
target_link_libraries(CudaLibrary PRIVATE CUDA::cudart_static)
