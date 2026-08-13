cmake_minimum_required(VERSION 3.20)
project(LLVM_Project)

set(CMAKE_CXX_STANDARD 20)

# Tell CMake to search MSYS2 for packages before vcpkg
list(APPEND CMAKE_PREFIX_PATH "D:/msys64/mingw64")
set(LLVM_DIR "D:/msys64/mingw64/lib/cmake/llvm" CACHE PATH "LLVM dir" FORCE)

# Load dependencies from MSYS2
find_package(ZLIB REQUIRED)
find_package(LibXml2 REQUIRED)

# Load LLVM
find_package(LLVM REQUIRED CONFIG)

include_directories(${LLVM_INCLUDE_DIRS} ${LIBXML2_INCLUDE_DIR})

# Split LLVM definitions string into individual flags
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND "${LLVM_DEFINITIONS}")
add_compile_options(${LLVM_DEFINITIONS_LIST})

add_executable(main main.cpp)

llvm_map_components_to_libnames(llvm_libs Core Support IRReader)
target_link_libraries(main PRIVATE ${llvm_libs} ${LIBXML2_LIBRARIES})