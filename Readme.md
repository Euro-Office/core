
[![License](https://img.shields.io/badge/License-GNU%20AGPL%20V3-green.svg?style=flat)](https://www.gnu.org/licenses/agpl-3.0.en.html)     ![x2tconverter](https://img.shields.io/badge/x2tconverter-v2.0.2.376-blue.svg?style=flat) ![Platforms Windows | OS X | Linux](https://img.shields.io/badge/Platforms-Windows%20%7C%20OS%20X%20%7C%20Linux%20-lightgray.svg?style=flat)

## Core
Server core components which are a part of [Document Server][1] and [Desktop Editors][2]. Enable the conversion between the most popular office document formats: DOC, DOCX, ODT, RTF, TXT, PDF, HTML, EPUB, XPS, DjVu, XLS, XLSX, ODS, CSV, PPT, PPTX, ODP.

  [1]: https://github.com/Euro-Office/DocumentServer
  [2]: https://github.com/Euro-Office/DesktopEditors

## Remote cache for nc-build.py dependencies

The `nc-build.py` scripts in `Common/3dParty` share built dependencies through a remote cache (see `ensure_dep` in `Common/3dParty/build_3rdparty_common.py`). By default they download from the public `eo-build-assets` bucket, `https://s3.eu-central-4.ionoscloud.com/eo-build-assets`, and build what is missing locally.

Archives live under `<BASE_REMOTE_PATH>/<platform>/<dep>/<version>/`, where `<version>` is the one passed to `init_for_dep()`. An archive is never replaced: bump the version to publish a new build.

| Variable | Description |
| --- | --- |
| `REMOTE_CACHE_TYPE` | `s3` (default), `nextcloud` or `none`. |
| `BASE_REMOTE_PATH` | Folder / key prefix on the remote. Default `3rdparty`. |
| `REMOTE_CACHE_READ_ONLY` | Set to `1` to only download from the cache, never upload. |

S3 (any S3 compatible store, path-style):

| Variable | Description |
| --- | --- |
| `REMOTE_CACHE_S3_URL` | `<endpoint>/<bucket>`. Default `https://s3.eu-central-4.ionoscloud.com/eo-build-assets`. Anonymous Linux builds need list access to pick the right glibc variant. |
| `REMOTE_CACHE_S3_REGION` | Signing region. Default: taken from an `s3.<region>.` host, else `us-east-1`. |
| `REMOTE_CACHE_S3_ACCESS_KEY`, `REMOTE_CACHE_S3_SECRET_KEY`, `REMOTE_CACHE_S3_SESSION_TOKEN` | Credentials for uploads. Without them the cache is read-only. |

Nextcloud (WebDAV, needs `curl`):

| Variable | Description |
| --- | --- |
| `NEXTCLOUD_REMOTE` | WebDAV root. Default `https://cloud.nextcloud.com/remote.php/dav/files`. |
| `NEXTCLOUD_USER` | Appended to the remote as `<NEXTCLOUD_REMOTE>/<NEXTCLOUD_USER>`. Leave empty if `NEXTCLOUD_REMOTE` already points at the cache root, e.g. a public share `https://host/public.php/dav/files/<share-token>`. |
| `NEXTCLOUD_PASS` | Password or app token. Authentication is only used if user and password are set. |

The Nextcloud cache is enabled when `NEXTCLOUD_USER` and `NEXTCLOUD_PASS` are set, or when `NEXTCLOUD_REMOTE` is set explicitly (anonymous access).

In CI, pull request builds only read. To upload new or bumped dependencies, run the workflows manually on `main` and approve the `build-assets` deployment (Review deployments > Approve):

| Workflow | Uploads |
| --- | --- |
| Build | Linux x64/arm64, macOS arm64 |
| Build (Windows x64) | Windows x64 and arm64 |
| Build (Android) | Android arm64-v8a, x86_64 |

```sh
gh workflow run build.yml -R Euro-Office/core --ref main
```

Runs waiting for approval: [Actions > is:waiting](https://github.com/Euro-Office/core/actions?query=is%3Awaiting). Cache contents: [bucket listing](https://s3.eu-central-4.ionoscloud.com/eo-build-assets/?list-type=2&prefix=3rdparty/).

## Build heavy dependencies for vcpkg

`Common/3dParty/build_dependencies.sh` builds CEF, libvlc, Boost, Qt and V8 (this takes 5 hours or more), uploads them to `vcpkg/<PKG_NAME>/<VERSION>/<TARGET_TRIPLET>/<PKG_NAME>_binary.tar.bz2` in the bucket and records their SHA512 in the dependency's `hashes.txt`. Set `REMOTE_CACHE_S3_ACCESS_KEY` and `REMOTE_CACHE_S3_SECRET_KEY`, then run

```sh
./build_dependencies.sh
```

To use another remote, pass

```sh
-DVCPKG_BINARY_REMOTE="https://my-remote.corp/repository/vcpkg"
```

to the cmake command of the core or desktop build.

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
