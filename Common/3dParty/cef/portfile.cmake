# 1. Setup hashes for each architecture (Vcpkg requires these)
# Tip: Leave these blank first; vcpkg will tell you the correct hash on failure.
set(CEF_HASH "NOTFOUND")

message(STATUS "CEF Version: ${VERSION}")
message(STATUS "Target Triplet: ${TARGET_TRIPLET}")

if(VERSION STREQUAL "5414")
    if(TARGET_TRIPLET STREQUAL "x64-linux")
        set(CEF_HASH "1db09ba082a6ee199318589c1b77c945fd3f045dc4806ff3f8450fd0bd3bbafd9e435c1aefb56d90232da8d038460fe4b5e388ac3bba2fe8dac346f5d48b3890")
    #elseif(TARGET_TRIPLET STREQUAL "x64-windows")
    #    set(CEF_HASH "PASTE_HASH_FOR_127_WIN_HERE")
    endif()
endif()

# 2. Construct the URL using built-in vcpkg variables
set(CLOUD_TOKEN "n9KYBcFYyLLCgEw")
set(CEF_URL "https://cloud.nextcloud.com/public.php/dav/files/${CLOUD_TOKEN}/CEF/${VERSION}/${TARGET_TRIPLET}/cef_binary.tar.bz2")

# 3. Download
vcpkg_download_distfile(ARCHIVE
    URLS "${CEF_URL}"
    FILENAME "cef_binary.tar.bz2"
    SHA512 "${CEF_HASH}"
)

# 4. Extract
vcpkg_extract_source_archive(
    SOURCE_PATH
    ARCHIVE "${ARCHIVE}"
)

# 5. Install exactly where you requested
file(INSTALL "${SOURCE_PATH}/" DESTINATION "${CURRENT_PACKAGES_DIR}/cef")

# 6. Required copyright file
file(INSTALL "${SOURCE_PATH}/LICENSE.txt" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)