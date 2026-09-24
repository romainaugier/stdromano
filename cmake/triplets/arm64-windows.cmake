# arm64-windows, as shipped by vcpkg, except that mimalloc is built as a static library

set(VCPKG_TARGET_ARCHITECTURE arm64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

if(PORT STREQUAL "mimalloc")
    set(VCPKG_LIBRARY_LINKAGE static)
endif()
