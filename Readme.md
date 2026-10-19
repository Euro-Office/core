
[![License](https://img.shields.io/badge/License-GNU%20AGPL%20V3-green.svg?style=flat)](https://www.gnu.org/licenses/agpl-3.0.en.html)     ![x2tconverter](https://img.shields.io/badge/x2tconverter-v2.0.2.376-blue.svg?style=flat) ![Platforms Windows | OS X | Linux](https://img.shields.io/badge/Platforms-Windows%20%7C%20OS%20X%20%7C%20Linux%20-lightgray.svg?style=flat)

## Core
Server core components which are a part of [Document Server][1] and [Desktop Editors][2]. Enable the conversion between the most popular office document formats: DOC, DOCX, ODT, RTF, TXT, PDF, HTML, EPUB, XPS, DjVu, XLS, XLSX, ODS, CSV, PPT, PPTX, ODP.

  [1]: https://github.com/Euro-Office/DocumentServer
  [2]: https://github.com/Euro-Office/DesktopEditors

## Remote cache for nc-build.py dependencies

The `nc-build.py` scripts in `Common/3dParty` can share built dependencies through a remote cache (see `ensure_dep` in `Common/3dParty/build_3rdparty_common.py`). If no cache is configured, everything is built locally. The cache is configured through environment variables:

| Variable | Description |
| --- | --- |
| `REMOTE_CACHE_TYPE` | `nextcloud`, `s3` or `none`. Defaults to `s3` if `S3_BUCKET` is set, otherwise `nextcloud`. |
| `BASE_REMOTE_PATH` | Folder / key prefix on the remote. Default `Engineering/Office/Euro-Office/3DPARTY_DEPS_1`. |
| `REMOTE_CACHE_READ_ONLY` | Set to `1` to only download from the cache, never upload. |

Nextcloud (WebDAV, needs `curl`):

| Variable | Description |
| --- | --- |
| `NEXTCLOUD_REMOTE` | WebDAV root. Default `https://cloud.nextcloud.com/remote.php/dav/files`. |
| `NEXTCLOUD_USER` | Appended to the remote as `<NEXTCLOUD_REMOTE>/<NEXTCLOUD_USER>`. Leave empty if `NEXTCLOUD_REMOTE` already points at the cache root, e.g. a public share `https://host/public.php/dav/files/<share-token>`. |
| `NEXTCLOUD_PASS` | Password or app token. Authentication is only used if user and password are set. |

The Nextcloud cache is enabled when `NEXTCLOUD_USER` and `NEXTCLOUD_PASS` are set, or when `NEXTCLOUD_REMOTE` is set explicitly (anonymous access).

S3 (AWS or any S3 compatible store):

| Variable | Description |
| --- | --- |
| `S3_BUCKET` | Bucket name. Enables the S3 cache. |
| `S3_ENDPOINT` | e.g. `https://minio.example.com`. Default `https://s3.<S3_REGION>.amazonaws.com`. |
| `S3_REGION` | Signing region. Default `us-east-1`. |
| `S3_ADDRESSING_STYLE` | `path` (default) or `virtual`. |
| `S3_ACCESS_KEY`, `S3_SECRET_KEY`, `S3_SESSION_TOKEN` | Credentials. Fall back to `AWS_ACCESS_KEY_ID`, `AWS_SECRET_ACCESS_KEY` and `AWS_SESSION_TOKEN`. Without credentials, requests are sent unsigned (public bucket). |

Anonymous remotes are usually read-only. A failed upload only prints a warning, but you can set `REMOTE_CACHE_READ_ONLY=1` to skip the upload step entirely.

## Build for Android

Core cross-compiles for Android (`arm64-v8a`, `x86_64`) with the NDK's CMake toolchain file:

```sh
cmake -S . -B build-android -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK_ROOT/build/cmake/android.toolchain.cmake" \
    -DANDROID_ABI=arm64-v8a -DANDROID_PLATFORM=android-26 -DANDROID_STL=c++_shared \
    -DCMAKE_BUILD_TYPE=Release
cmake --build build-android --target x2tlib
```

Configuring runs `Common/3dParty/build_3rdparty.py --target=android-arm64` (or `android-x64`), which cross-compiles boost, ICU, openssl and V8 with the NDK from `ANDROID_NDK_ROOT`. The result is a set of shared libraries (`libx2t.so`, `libkernel.so`, …, plus unversioned `libicu*.so`) for the JNI wrapper in `X2tConverter/build/Android/libx2t`. `.docker/core-android.bake.Dockerfile` builds them in Docker (stage `core-android`, argument `ANDROID_ABI`); it needs an image with the NDK and the host tools as its `android-ndk` context, which [MobileEditors](https://github.com/Euro-Office/MobileEditors) provides.

## License

Core is released under the GNU AGPL v3.0 license. See the LICENSE file for more information.
