cmake_minimum_required(VERSION 3.24)

# Build for the GPU in this machine by default. To ship the DLL to other PCs, list
# the architectures explicitly instead, e.g. -DCMAKE_CUDA_ARCHITECTURES="86;89".
if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES native)
endif()

# Debug info goes into .pdb files, which works for both cl.exe and nvcc.
# (Edit-and-Continue /ZI from the Visual Studio template is not supported by nvcc.)
if(POLICY CMP0141)
    cmake_policy(SET CMP0141 NEW)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>")
endif()

project(CudaLibrary VERSION 0.1.0 LANGUAGES CXX)

# Fail early with a clear message instead of nvcc's "Cannot find compiler 'cl.exe' in PATH".
if(WIN32 AND NOT MSVC)
    message(FATAL_ERROR
        "CUDA on Windows needs the MSVC toolchain, but CMake is using ${CMAKE_CXX_COMPILER_ID}.\n"
        "In CLion: Settings > Build, Execution, Deployment > Toolchains > '+' > Visual Studio, "
        "move it to the top, then Tools > CMake > Reset Cache and Reload Project.")
endif()

enable_language(CUDA)

# C++20 is the newest standard nvcc supports, and it is what Unreal Engine 5 uses.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 20)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

# Put the .dll and the test .exe in the same folder so the test can find the DLL.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

enable_testing()

add_subdirectory(CudaLibrary)
add_subdirectory(Tests)
