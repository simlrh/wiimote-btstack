cmake_minimum_required(VERSION 3.0)
project(WiimoteBTStack)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(libusb-1.0 REQUIRED)

set(BTSTACK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/btstack")
set(BTSTACK_SRC "${BTSTACK_ROOT}/src")

set(SOURCES
	src/main.cpp
	src/btstack_config.h
	src/wiimote_btstack.hpp
	src/BTStack.hpp
	src/BTStack.cpp
	src/Wiimote.hpp
	src/Wiimote.cpp
	src/CommandQueue.hpp
	src/CommandQueue.cpp
	src/Pipes.hpp
)

set(
    BTSTACK_LIBRARIES
    "${LIBUSB_1_LIBRARY}"
)

set(
	BTSTACK_INCLUDE_DIRS
	"${BTSTACK_ROOT}/src/ble"
    "${BTSTACK_ROOT}/src/classic"
    "${BTSTACK_ROOT}/src"
    "${BTSTACK_ROOT}/3rd-party/mbedtls/include"
    "${BTSTACK_ROOT}/3rd-party/bluedroid/decoder/include"
    "${BTSTACK_ROOT}/3rd-party/bluedroid/encoder/include"
	"${BTSTACK_ROOT}/platform/posix"
)

set(
	BTSTACK_CORE_SOURCES
	"${BTSTACK_ROOT}/platform/posix/stdin_support.c"
	"${BTSTACK_SRC}/btstack_memory.c"
	"${BTSTACK_SRC}/btstack_linked_list.c"
	"${BTSTACK_SRC}/btstack_memory_pool.c"
	"${BTSTACK_SRC}/btstack_run_loop.c"
	"${BTSTACK_SRC}/btstack_util.c"
)

if(WIN32)
	list(APPEND BTSTACK_INCLUDE_DIRS "${BTSTACK_ROOT}/platform/win")
	list(APPEND BTSTACK_CORE_SOURCES "${BTSTACK_ROOT}/platform/win/sys/time.c")
    list(APPEND BTSTACK_LIBRARIES ws2_32)

	list(APPEND SOURCES src/WinPipes.cpp)

	add_definitions(-Dsnprintf=_snprintf)
endif(WIN32)

include_directories(
	${BTSTACK_INCLUDE_DIRS}
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    ${LIBUSB_1_INCLUDE_DIRS}
)

    
add_library(
    BTSTACK_CORE
    OBJECT
	${BTSTACK_CORE_SOURCES}
)

add_library(
    BTSTACK_COMMON
    OBJECT
    "${BTSTACK_SRC}/hci.c"
	"${BTSTACK_SRC}/hci_cmd.c"
	"${BTSTACK_SRC}/hci_dump.c"
	"${BTSTACK_SRC}/l2cap.c"
	"${BTSTACK_SRC}/l2cap_signaling.c"
    "${BTSTACK_ROOT}/platform/libusb/hci_transport_h2_libusb.c"
    "${BTSTACK_ROOT}/platform/posix/btstack_run_loop_posix.c"
    "${BTSTACK_ROOT}/platform/posix/le_device_db_fs.c"
    "${BTSTACK_ROOT}/platform/posix/btstack_link_key_db_fs.c"
)

add_library(
    BTSTACK_CLASSIC
    OBJECT
    "${BTSTACK_SRC}/classic/btstack_link_key_db_memory.c"
	"${BTSTACK_SRC}/classic/sdp_util.c"
	"${BTSTACK_SRC}/classic/spp_server.c"
	"${BTSTACK_SRC}/classic/rfcomm.c"
	"${BTSTACK_SRC}/classic/bnep.c"
	"${BTSTACK_SRC}/classic/sdp_server.c"
)

target_compile_definitions(
	BTSTACK_CORE
	PRIVATE
	"inline=__inline"
)
target_compile_definitions(
	BTSTACK_COMMON
	PRIVATE
	"inline=__inline"
)
target_compile_definitions(
	BTSTACK_CLASSIC
	PRIVATE
	"inline=__inline"
)

add_executable(
    wiimote_btstack
	${SOURCES}
    $<TARGET_OBJECTS:BTSTACK_CORE>
    $<TARGET_OBJECTS:BTSTACK_COMMON>
	$<TARGET_OBJECTS:BTSTACK_CLASSIC>
)

target_link_libraries(wiimote_btstack ${BTSTACK_LIBRARIES})