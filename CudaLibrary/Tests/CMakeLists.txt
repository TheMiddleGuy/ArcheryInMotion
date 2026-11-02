# Small console program that uses the DLL exactly like Unreal will: through CudaLibrary.h only.
# Run it from CLion (CudaLibraryTest configuration) or with ctest.
add_executable(CudaLibraryTest main.cpp)
target_link_libraries(CudaLibraryTest PRIVATE CudaLibrary)

add_test(NAME CudaLibraryTest COMMAND CudaLibraryTest)
