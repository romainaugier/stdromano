# x64-windows, as shipped by vcpkg, except that mimalloc is built as a static library
# The other triplets we use (x64-linux, arm64-linux, arm64-osx) are static already

set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)
set(VCPKG_PROVIDED_FORTRAN ON)

if(PORT STREQUAL "mimalloc")
    set(VCPKG_LIBRARY_LINKAGE static)
endif()
