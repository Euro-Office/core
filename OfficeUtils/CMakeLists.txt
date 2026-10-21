cmake_minimum_required(VERSION 3.10)

function(add_officeutils TARGET OFFICE_UTILS_DIR)

    set(OFFICE_UTILS_SOURCES_DIR "${OFFICE_UTILS_DIR}/src")

    # OfficeUtils.pro
    set(OFFICE_UTILS_SOURCES
        ${OFFICE_UTILS_SOURCES_DIR}/OfficeUtils.cpp
        ${OFFICE_UTILS_SOURCES_DIR}/ZipBuffer.cpp
        ${OFFICE_UTILS_SOURCES_DIR}/ZipUtilsCP.cpp
        ${OFFICE_UTILS_SOURCES_DIR}/zlib_addon.c
        ${OFFICE_UTILS_SOURCES_DIR}/ioapibuf.c
        
        ${OFFICE_UTILS_SOURCES_DIR}/OfficeUtils.h
        ${OFFICE_UTILS_SOURCES_DIR}/OfficeUtilsCommon.h
        ${OFFICE_UTILS_SOURCES_DIR}/ZipBuffer.h
        ${OFFICE_UTILS_SOURCES_DIR}/ZipFolder.h
        ${OFFICE_UTILS_SOURCES_DIR}/ZipUtilsCP.h
        ${OFFICE_UTILS_SOURCES_DIR}/zlib_addon.h
        ${OFFICE_UTILS_SOURCES_DIR}/ioapibuf.h
    )

    target_sources( ${TARGET} PRIVATE ${OFFICE_UTILS_SOURCES} )

    target_include_directories( ${TARGET} PRIVATE
        ${ZLIBNG_INCLUDEDIR_ABS}
        ${ZLIBNG_INCLUDEDIR_ABS}/minizip # exposes unzip.h, zip.h, ioapi.h directly
        ${OFFICE_UTILS_SOURCES_DIR}
    )

    target_link_directories( ${TARGET} PUBLIC
        ${ZLIBNG_LIBDIR_ABS}
    )

    if( MSVC )
        target_link_libraries( ${TARGET} PUBLIC
            minizip                 # minizip.lib (minizip-ng with MZ_COMPAT=ON)
            ${ZLIBNG_LIB_NAME}      # zlib.lib (zlib-ng, zlib1.dll)
        )
    else()
        target_link_libraries( ${TARGET} PUBLIC
            minizip # libminizip.so (minizip-ng with MZ_COMPAT=ON)
            z       # libz.so (zlib-ng)
        )
    endif()


    if( NOT MSVC )
        target_compile_options( ${TARGET} PRIVATE "SHELL:-include unistd.h")
    endif()

    if(APPLE)
        target_compile_definitions( ${TARGET} PRIVATE
            _MAC
        )
    endif()
endfunction()
