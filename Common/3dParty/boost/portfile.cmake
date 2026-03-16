# 1. Setup hashes for each architecture (Vcpkg requires these)
# Tip: Leave these blank first; vcpkg will tell you the correct hash on failure.
set(PKG_SHA512 "NOTFOUND")
set(PKG_NAME "boost")

# 1. Read the hashes file from the LOCAL repository
file(READ "${CMAKE_CURRENT_LIST_DIR}/hashes.txt" HASH_DATA)

# 2. Extract the specific line for this version+triplet
string(REGEX MATCH "(${VERSION}\\|${TARGET_TRIPLET}\\|[A-Za-z0-9]+)" MATCHED_LINE "${HASH_DATA}")

# --- NEW ROBUST LOGIC ---
if(MATCHED_LINE)
    # Split the matched line by the pipe '|' character
    string(REPLACE "|" ";" LINE_ITEMS "${MATCHED_LINE}")
    
    # LINE_ITEMS is now a CMake list: VERSION;TRIPLET;HASH
    list(GET LINE_ITEMS 2 PKG_SHA512)
else()
    message(FATAL_ERROR "No hash line found for ${VERSION}|${TARGET_TRIPLET} in hashes.txt")
endif()

# 2. Construct the URL using built-in vcpkg variables
set(CLOUD_TOKEN "n9KYBcFYyLLCgEw")
set(PKG_URL "https://cloud.nextcloud.com/public.php/dav/files/${CLOUD_TOKEN}/${PKG_NAME}/${VERSION}/${TARGET_TRIPLET}/${PKG_NAME}_binary.tar.bz2")

# 3. Download
vcpkg_download_distfile(ARCHIVE
    URLS "${PKG_URL}"
    FILENAME "${PKG_NAME}_binary.tar.bz2"
    SHA512 "${PKG_SHA512}"
)

# 4. Extract
vcpkg_extract_source_archive(
    SOURCE_PATH
    ARCHIVE "${ARCHIVE}"
)

# 5. Install exactly where you requested
file(INSTALL "${SOURCE_PATH}/" DESTINATION "${CURRENT_PACKAGES_DIR}/${PKG_NAME}" USE_SOURCE_PERMISSIONS)
