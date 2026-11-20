cmake_minimum_required(VERSION 3.10)

project(kernel_network)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")

include(${CORE_ROOT_DIR}/common.cmake)

if(NOT TARGET kernel)
    add_subdirectory(${CORE_ROOT_DIR}/Common kernel)
endif()

find_package(sioxx REQUIRED)

add_library(kernel_network SHARED
    WebSocket/src/managerWebSocket.cpp
    WebSocket/src/socketio/socketio_internal.cpp
    WebSocket/src/socketio/socketio_internal_private.cpp

    WebSocket/include/websocket.h
	WebSocket/src/websocketbase.h
    WebSocket/src/socketio/socketio_internal.h
    WebSocket/src/socketio/socketio_internal_private.h
    WebSocket/src/socketio/socketio_internal_private_no_tls.h

    # filetransporter.pri
    ${CMAKE_CURRENT_SOURCE_DIR}/FileTransporter/src/FileTransporter.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/FileTransporter/src/manager.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/FileTransporter/include/FileTransporter.h
	${CMAKE_CURRENT_SOURCE_DIR}/FileTransporter/include/manager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FileTransporter/src/FileTransporter_private.h
	${CMAKE_CURRENT_SOURCE_DIR}/FileTransporter/src/transport_external.h
)

# filetransporter.pri
if( MSVC )
    target_sources( kernel_network PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/FileTransporter/src/FileTransporter_win.cpp
    )
elseif( APPLE )
    target_sources( kernel_network PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/FileTransporter/src/FileTransporter_mac.mm
    )
else() # LINUX
    target_sources( kernel_network PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/FileTransporter/src/FileTransporter_curl.cpp
    )
endif()

set_default_options(kernel_network)

if( MSVC )
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/FileTransporter/src/FileTransporter_win.cpp
        PROPERTIES
            COMPILE_DEFINITIONS "UNICODE;_UNICODE"
    )
endif()

target_compile_definitions(kernel_network PRIVATE
    KERNEL_USE_DYNAMIC_LIBRARY_BUILDING
    USE_IOWEBSOCKET
)

if(UNIX)
    target_compile_definitions(kernel_network PRIVATE
        USE_EXTERNAL_TRANSPORT
    )
endif()

target_include_directories(kernel_network PRIVATE
    ${OPENSSL_INSTALL_DIR_ABS}/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/FileTransporter
    ${CORE_ROOT_DIR}
)

if( NOT MSVC )
    target_compile_options(kernel_network PRIVATE
        -include csignal
    )
endif()

target_link_libraries(kernel_network PRIVATE
    sioxx::sioxx
    ${OPENSSL_LIBSSL}
    ${OPENSSL_LIBCRYPTO}
)

target_link_libraries(kernel_network PUBLIC
    kernel
)

# Android's libc includes pthreads
if( UNIX AND NOT ANDROID )
    target_link_libraries(kernel_network PUBLIC
        pthread
    )
endif()

if( MSVC )
    target_link_libraries(kernel_network PUBLIC
        Crypt32
        Advapi32
        urlmon
        Shell32
    )
endif()

if( APPLE )
    find_library( APPKIT_LIBRARY AppKit REQUIRED )
    target_link_libraries( kernel_network PRIVATE ${APPKIT_LIBRARY} )
endif()

copy_artifacts_to_folder("kernel_network" "${EO_CORE_OUTPUT_DIR}")

option(EO_BUILD_TESTS "Build unit tests" OFF)
if(EO_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

declare_victory( kernel_network )
