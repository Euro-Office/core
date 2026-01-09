cmake_minimum_required(VERSION 3.10)

project(kernel_network)

set(CORE_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")

include(${CORE_ROOT_DIR}/common.cmake)

if(NOT TARGET kernel)
    add_subdirectory(${CORE_ROOT_DIR}/Common kernel)
endif()



message( "Fetch and build OpenSSL" )
set(OPENSSL_3RDPARTY_DIR "${CORE_ROOT_DIR}/Common/3dParty/openssl")
set(OPENSSL_INSTALL_DIR "${EO_CORE_3RD_PARTY_INSTALL_DIR}/openssl_build_linux_64")
get_filename_component(OPENSSL_3RDPARTY_DIR_ABS "${OPENSSL_3RDPARTY_DIR}" ABSOLUTE)
get_filename_component(OPENSSL_INSTALL_DIR_ABS "${OPENSSL_INSTALL_DIR}" ABSOLUTE)
execute_process(
    COMMAND_ECHO STDOUT
    COMMAND ./nc-build.sh "${EO_CORE_3RD_PARTY_WORK_DIR}/openssl" "${OPENSSL_INSTALL_DIR}"
    WORKING_DIRECTORY ${OPENSSL_3RDPARTY_DIR_ABS}
)



message( "Fetch and patch Socket-IO" )
set(SOCKETIO_3DPARTY_DIR "${CORE_ROOT_DIR}/Common/3dParty/socketio")
set(SOCKETIO_INSTALL_DIR "${EO_CORE_3RD_PARTY_INSTALL_DIR}/socket.io-client-cpp")
get_filename_component(SOCKETIO_3DPARTY_DIR_ABS "${SOCKETIO_3DPARTY_DIR}" ABSOLUTE)
get_filename_component(SOCKETIO_INSTALL_DIR_ABS "${SOCKETIO_INSTALL_DIR}" ABSOLUTE)
execute_process(
    COMMAND_ECHO STDOUT
    COMMAND ./nc-fetch.sh "${SOCKETIO_INSTALL_DIR}"
    WORKING_DIRECTORY ${SOCKETIO_3DPARTY_DIR_ABS}
)



add_library(kernel_network SHARED
    ${SOCKETIO_INSTALL_DIR}/src_no_tls/internal/sio_client_impl.cpp
    ${SOCKETIO_INSTALL_DIR}/src_no_tls/internal/sio_client_impl.h
    ${SOCKETIO_INSTALL_DIR}/src_no_tls/internal/sio_packet.cpp
    ${SOCKETIO_INSTALL_DIR}/src_no_tls/internal/sio_packet.h
    ${SOCKETIO_INSTALL_DIR}/src_no_tls/sio_client.cpp
    ${SOCKETIO_INSTALL_DIR}/src_no_tls/sio_client.h
    ${SOCKETIO_INSTALL_DIR}/src_no_tls/sio_message.h
    ${SOCKETIO_INSTALL_DIR}/src_no_tls/sio_socket.cpp
    ${SOCKETIO_INSTALL_DIR}/src_no_tls/sio_socket.h
    ${SOCKETIO_INSTALL_DIR}/src/internal/sio_client_impl.cpp
    ${SOCKETIO_INSTALL_DIR}/src/internal/sio_client_impl.h
    ${SOCKETIO_INSTALL_DIR}/src/internal/sio_packet.cpp
    ${SOCKETIO_INSTALL_DIR}/src/internal/sio_packet.h
    ${SOCKETIO_INSTALL_DIR}/src/sio_client.cpp
    ${SOCKETIO_INSTALL_DIR}/src/sio_client.h
    ${SOCKETIO_INSTALL_DIR}/src/sio_message.h
    ${SOCKETIO_INSTALL_DIR}/src/sio_socket.cpp
    ${SOCKETIO_INSTALL_DIR}/src/sio_socket.h

    WebSocket/src/managerWebSocket.cpp
    WebSocket/src/socketio/socketio_internal.cpp

    WebSocket/include/websocket.h
	WebSocket/src/websocketbase.h
    WebSocket/src/socketio/socketio_internal.h
    WebSocket/src/socketio/socketio_internal_private.h
    WebSocket/src/socketio/socketio_internal_private_no_tls.h

    # filetransporter.pri
    
    ${CMAKE_CURRENT_SOURCE_DIR}/FileTransporter/src/FileTransporter.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/FileTransporter/src/Session.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/FileTransporter/src/manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/FileTransporter/src/FileTransporter_curl.cpp

    ${CMAKE_CURRENT_SOURCE_DIR}/FileTransporter/include/FileTransporter.h
	${CMAKE_CURRENT_SOURCE_DIR}/FileTransporter/include/manager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/FileTransporter/src/FileTransporter_private.h
	${CMAKE_CURRENT_SOURCE_DIR}/FileTransporter/src/transport_external.h
	${CMAKE_CURRENT_SOURCE_DIR}/FileTransporter/src/Session.h
)

set_default_options(kernel_network)

target_compile_definitions(kernel_network PRIVATE
    KERNEL_USE_DYNAMIC_LIBRARY_BUILDING
    BOOST_DATE_TIME_NO_LIB
    BOOST_REGEX_NO_LIB
    ASIO_STANDALONE
    _WEBSOCKETPP_CPP11_STL_
    _WEBSOCKETPP_CPP11_FUNCTIONAL_
    _WEBSOCKETPP_CPP11_TYPE_TRAITS_
    _WEBSOCKETPP_CPP11_CHRONO_
    SIO_TLS=1
    SIO_TLS_NO=0
    USE_IOWEBSOCKET
)

if(UNIX)
    target_compile_definitions(kernel_network PRIVATE
        USE_EXTERNAL_TRANSPORT
    )
endif()

target_include_directories(kernel_network PRIVATE
    ${OPENSSL_INSTALL_DIR}/include
    ${SOCKETIO_INSTALL_DIR}/lib/websocketpp
    ${SOCKETIO_INSTALL_DIR}/lib/rapidjson/include
    ${SOCKETIO_INSTALL_DIR}/lib/asio/asio/include
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/FileTransporter
)

target_link_libraries(kernel_network PRIVATE
    ${OPENSSL_INSTALL_DIR_ABS}/lib/libssl.a
    ${OPENSSL_INSTALL_DIR_ABS}/lib/libcrypto.a
)

target_link_libraries(kernel_network PUBLIC
    kernel
    pthread
)

if(WIN_32)
    target_link_libraries(kernel_network PUBLIC
        Advapi32
        urlmon
        Rpcrt4
        Shell32
    )
endif()
