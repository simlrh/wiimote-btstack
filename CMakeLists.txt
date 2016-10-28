cmake_minimum_required(VERSION 2.8.12)
project(BTStack)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
find_package(libusb-1.0 REQUIRED)

set(BTSTACK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/btstack")
set(BTSTACK_SRC "${BTSTACK_ROOT}/src")

set(
    BTSTACK_LIBRARIES
    "${LIBUSB_1_LIBRARY}"
)

if(WIN32)
    list(APPEND BTSTACK_LIBRARIES ws2_32)
	add_definitions(-Dinline=__inline -Dsnprintf=_snprintf)
endif(WIN32)

set(
	BTSTACK_INCLUDE_DIRS
	"${BTSTACK_ROOT}/src/ble"
    "${BTSTACK_ROOT}/src/classic"
    "${BTSTACK_ROOT}/src"
    "${BTSTACK_ROOT}/3rd-party/mbedtls/include"
    "${BTSTACK_ROOT}/3rd-party/bluedroid/decoder/include"
    "${BTSTACK_ROOT}/3rd-party/bluedroid/encoder/include"
	"${BTSTACK_ROOT}/platform/win"
	"${BTSTACK_ROOT}/platform/posix"
	"${BTSTACK_ROOT}/platform/embedded"
)

include_directories(
	${BTSTACK_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${LIBUSB_1_INCLUDE_DIRS}
)
    
add_library(
    BTSTACK_CORE
    OBJECT
    "${BTSTACK_ROOT}/port/libusb/main.c"
    "${BTSTACK_ROOT}/platform/posix/stdin_support.c"
	"${BTSTACK_SRC}/btstack_memory.c"
	"${BTSTACK_SRC}/btstack_linked_list.c"
	"${BTSTACK_SRC}/btstack_memory_pool.c"
	"${BTSTACK_SRC}/btstack_run_loop.c"
	"${BTSTACK_SRC}/btstack_util.c"
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

add_executable(
    btstack_inquiry
    wiimote_inquiry.c
    "${BTSTACK_SRC}/ad_parser.c"
    $<TARGET_OBJECTS:BTSTACK_CORE>
    $<TARGET_OBJECTS:BTSTACK_COMMON>
    $<TARGET_OBJECTS:BTSTACK_CLASSIC>
)

target_link_libraries(btstack_inquiry ${BTSTACK_LIBRARIES})