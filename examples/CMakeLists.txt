cmake_minimum_required(VERSION 3.10)
project(rpc-examples C)

set(CMAKE_C_STANDARD 11)

# === Paths ===
set(RPC_CORE_DIR ${CMAKE_SOURCE_DIR}/../core)
set(RPC_PLATFORM_DIR ${CMAKE_SOURCE_DIR}/../platform/linux)

# Collect a list of kernel and platform sources
set(RPC_SOURCES
    ${RPC_CORE_DIR}/src/rpc.c
    ${RPC_CORE_DIR}/src/rpc_crc8.c
    ${RPC_CORE_DIR}/src/rpc_link.c
    ${RPC_CORE_DIR}/src/rpc_transport.c
    ${RPC_PLATFORM_DIR}/rpc_osal_linux.c
    ${RPC_PLATFORM_DIR}/rpc_phy_linux.c
)

# Add include directories
include_directories(
    ${RPC_CORE_DIR}/include
    ${RPC_PLATFORM_DIR}
)

# === Compiling all examples ===
file(GLOB EXAMPLES *.c)

foreach(example ${EXAMPLES})
    get_filename_component(example_name ${example} NAME_WE)
    add_executable(${example_name} ${example} ${RPC_SOURCES})
endforeach()

