
[![License](https://img.shields.io/badge/License-GNU%20AGPL%20V3-green.svg?style=flat)](https://www.gnu.org/licenses/agpl-3.0.en.html)     ![x2tconverter](https://img.shields.io/badge/x2tconverter-v2.0.2.376-blue.svg?style=flat) ![Platforms Windows | OS X | Linux](https://img.shields.io/badge/Platforms-Windows%20%7C%20OS%20X%20%7C%20Linux%20-lightgray.svg?style=flat)

## Core
Server core components which are a part of [Document Server][1] and [Desktop Editors][2]. Enable the conversion between the most popular office document formats: DOC, DOCX, ODT, RTF, TXT, PDF, HTML, EPUB, XPS, DjVu, XLS, XLSX, ODS, CSV, PPT, PPTX, ODP.

  [1]: https://github.com/Euro-Office/DocumentServer
  [2]: https://github.com/Euro-Office/DesktopEditors

## Build third party dependencies

To build third-party dependencies you can use our build script at Common/3dParty/build_dependencies.sh. Currently it only supports a Nextcloud remote but feel free to add support for others as well. You have to add a NEXTCLOUD_USER, NEXTCLOUD_PASS, NEXTCLOUD_REMOTE and BASE_REMOTE_PATH. Then run 
```sh
./build_dependencies.sh
```

to build all dependencies. This takes very long (~5 hours or more). Once finished, the binary files are in `PKG_NAME/VERSION/TARGET_TRIPLET/PKG_NAME_binary.tar.bz2` of your specified remote. Once finished you can build the project with your pre-compiled binaries by specifying 
```sh
-DVCPKG_BINARY_REMOTE="https://my-remote.corp/repository/vcpkg/"
```
in the cmake command of the core or desktop build.

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
