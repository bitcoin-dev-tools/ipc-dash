cmake_minimum_required(VERSION 3.20)
project(bitcoind_ipc_exporter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_WITH_BITCOIN_CORE_TREE "Build using a local Bitcoin Core source/build tree" ON)
set(BITCOIN_SDK_DIR "" CACHE PATH "Path to packaged Bitcoin IPC SDK")
set(BITCOIN_SOURCE_DIR "" CACHE PATH "Path to Bitcoin Core source tree")
set(BITCOIN_BUILD_DIR "" CACHE PATH "Path to Bitcoin Core build tree")
set(BITCOIN_INCLUDE_DIRS "" CACHE STRING "Extra include dirs (semicolon separated)")
set(BITCOIN_LIBRARY_DIRS "" CACHE STRING "Extra library dirs (semicolon separated)")
set(BITCOIN_LINK_LIBS "" CACHE STRING "Override link libraries (semicolon separated)")
set(DEFAULT_BITCOIN_LINK_LIBS
  bitcoin_common
  bitcoin_util
  bitcoin_crypto
  bitcoin_clientversion
  bitcoin_consensus
  multiprocess
  secp256k1
  leveldb
  crc32c
  minisketch
  univalue
  capnp-rpc
  capnp
  kj-async
  kj
)

add_executable(bitcoind-ipc-exporter
  src/main.cpp
  src/translation_stub.cpp
  src/wallet_stubs.cpp
)

target_compile_options(bitcoind-ipc-exporter PRIVATE -Wall -Wpedantic)
target_compile_options(bitcoind-ipc-exporter PRIVATE -UNDEBUG)

if(BITCOIN_SDK_DIR)
  target_include_directories(bitcoind-ipc-exporter PRIVATE
    "${BITCOIN_SDK_DIR}/include/src"
    "${BITCOIN_SDK_DIR}/include/build"
    "${BITCOIN_SDK_DIR}/include/src/univalue/include"
  )
  target_link_directories(bitcoind-ipc-exporter PRIVATE
    "${BITCOIN_SDK_DIR}/lib"
  )
  set(DEFAULT_BITCOIN_LINK_LIBS
    bitcoin_ipc
    ${DEFAULT_BITCOIN_LINK_LIBS}
  )
elseif(BUILD_WITH_BITCOIN_CORE_TREE)
  if(NOT BITCOIN_SOURCE_DIR)
    message(FATAL_ERROR "BITCOIN_SOURCE_DIR is required when BUILD_WITH_BITCOIN_CORE_TREE=ON")
  endif()
  if(NOT BITCOIN_BUILD_DIR)
    message(FATAL_ERROR "BITCOIN_BUILD_DIR is required when BUILD_WITH_BITCOIN_CORE_TREE=ON")
  endif()

  target_include_directories(bitcoind-ipc-exporter PRIVATE
    "${BITCOIN_SOURCE_DIR}/src"
    "${BITCOIN_SOURCE_DIR}/src/univalue/include"
    "${BITCOIN_BUILD_DIR}"
    "${BITCOIN_BUILD_DIR}/src"
  )
  target_link_directories(bitcoind-ipc-exporter PRIVATE
    "${BITCOIN_BUILD_DIR}/src"
    "${BITCOIN_BUILD_DIR}/src/univalue"
  )
  set(DEFAULT_BITCOIN_LINK_LIBS
    bitcoin_ipc
    bitcoin_node
    ${DEFAULT_BITCOIN_LINK_LIBS}
  )
endif()

if(BITCOIN_INCLUDE_DIRS)
  target_include_directories(bitcoind-ipc-exporter PRIVATE ${BITCOIN_INCLUDE_DIRS})
endif()
if(BITCOIN_LIBRARY_DIRS)
  target_link_directories(bitcoind-ipc-exporter PRIVATE ${BITCOIN_LIBRARY_DIRS})
endif()

if(BITCOIN_LINK_LIBS)
  target_link_libraries(bitcoind-ipc-exporter PRIVATE
    "-Wl,--start-group"
    ${BITCOIN_LINK_LIBS}
    "-Wl,--end-group"
    pthread
  )
else()
  # Default library set for Bitcoin Core IPC-enabled builds.
  target_link_libraries(bitcoind-ipc-exporter PRIVATE
    "-Wl,--start-group"
    ${DEFAULT_BITCOIN_LINK_LIBS}
    "-Wl,--end-group"
    pthread
  )
endif()

install(TARGETS bitcoind-ipc-exporter RUNTIME DESTINATION bin)
