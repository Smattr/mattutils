## @file
## @brief Build system for libute
##
## All content in this file is in the public domain. Use it any way you wish.

add_library(libute
  src/asp.c
  src/dict_contains_.c
  src/dict_free_.c
  src/dict_get_.c
  src/dict_remove_.c
  src/dict_set_.c
  src/dict_size_.c
  src/dword_atomic_cas.c
  src/dword_atomic_cas_lo.c
  src/dword_atomic_cas_hi.c
  src/dword_atomic_cas_n.c
  src/dword_atomic_cas_n_lo.c
  src/dword_atomic_cas_n_hi.c
  src/dword_atomic_load.c
  src/dword_atomic_load_lo.c
  src/dword_atomic_load_hi.c
  src/dword_atomic_store.c
  src/dword_atomic_store_lo.c
  src/dword_atomic_store_hi.c
  src/dword_atomic_xchg.c
  src/dword_atomic_xchg_lo.c
  src/dword_atomic_xchg_hi.c
  src/dword_zero.c
  src/hash.c
  src/int128_atomic_cas.c
  src/int128_atomic_cas_n.c
  src/int128_atomic_load.c
  src/int128_atomic_store.c
  src/int128_atomic_xchg.c
  src/int128_put.c
  src/set_bitset_contains_.c
  src/set_bitset_free_.c
  src/set_bitset_insert_.c
  src/set_bitset_remove_.c
  src/set_bitset_size_.c
  src/set_boxed_contains_.c
  src/set_boxed_free_.c
  src/set_boxed_insert_.c
  src/set_boxed_remove_.c
  src/set_boxed_size_.c
  src/set_inline_contains_.c
  src/set_inline_free_.c
  src/set_inline_insert_.c
  src/set_inline_remove_.c
  src/set_inline_size_.c
  src/set_unboxed_contains_.c
  src/set_unboxed_free_.c
  src/set_unboxed_insert_.c
  src/set_unboxed_remove_.c
  src/set_unboxed_size_.c
  src/uint128_atomic_cas.c
  src/uint128_atomic_cas_n.c
  src/uint128_atomic_load.c
  src/uint128_atomic_store.c
  src/uint128_atomic_xchg.c
  src/uint128_put.c
  src/putb.c
)

if(APPLE)
  target_compile_options(libute PRIVATE -fno-common)
endif()

CHECK_C_COMPILER_FLAG(-mcx16 HAS_MCX16)
if(HAS_MCX16)
  target_compile_options(libute PUBLIC -mcx16)
endif()

target_include_directories(libute
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    src
)

set_target_properties(libute PROPERTIES PREFIX "")

install(TARGETS libute EXPORT libuteConfig
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(
  EXPORT libuteConfig
  FILE libuteConfig.cmake
  NAMESPACE ute::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ute
)
install(
  FILES uteConfig.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ute
)
