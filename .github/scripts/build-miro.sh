#!/usr/bin/env bash
set -euo pipefail

: "${ANDROID_SDK_ROOT:?ANDROID_SDK_ROOT is required}"

ABI="${1:-armeabi-v7a}"
NDK_VERSION="26.3.11579264"
NDK="${ANDROID_SDK_ROOT}/ndk/${NDK_VERSION}"
TOOLCHAIN="${NDK}/toolchains/llvm/prebuilt/linux-x86_64"
AR="${TOOLCHAIN}/bin/llvm-ar"
RAYLIB_ANDROID_ARCH=""
CC=""
DIAGNOSTIC_FLAGS=()

case "${ABI}" in
  armeabi-v7a)
    CC="${TOOLCHAIN}/bin/armv7a-linux-androideabi21-clang"
    RAYLIB_ANDROID_ARCH="arm"
    ;;
  x86_64)
    CC="${TOOLCHAIN}/bin/x86_64-linux-android21-clang"
    RAYLIB_ANDROID_ARCH="x86_64"
    DIAGNOSTIC_FLAGS+=("-DCRYSTAL_EMULATOR_TEST=1")
    ;;
  *)
    echo "unsupported ABI: ${ABI}" >&2
    exit 2
    ;;
esac

DEPS="${RUNNER_TEMP:-/tmp}/crystal-${ABI}-deps"
rm -rf "${DEPS}"
mkdir -p "${DEPS}"

git clone --depth 1 --branch 6.0 https://github.com/raysan5/raylib.git "${DEPS}/raylib"
curl -fsSL https://www.lua.org/ftp/lua-5.4.8.tar.gz -o "${DEPS}/lua.tar.gz"
tar -xzf "${DEPS}/lua.tar.gz" -C "${DEPS}"

make -C "${DEPS}/raylib/src" \
  PLATFORM=PLATFORM_ANDROID \
  ANDROID_ARCH="${RAYLIB_ANDROID_ARCH}" \
  ANDROID_API_VERSION=21 \
  ANDROID_NDK="${NDK}" \
  RAYLIB_BUILD_MODE=RELEASE \
  RAYLIB_LIBTYPE=STATIC \
  RAYLIB_MODULE_AUDIO=FALSE \
  -j2

LUA_SRC="${DEPS}/lua-5.4.8/src"
LUA_OBJ="${DEPS}/lua-objects"
mkdir -p "${LUA_OBJ}"
for source in "${LUA_SRC}"/*.c; do
  base="$(basename "${source}")"
  case "${base}" in
    lua.c|luac.c) continue ;;
  esac
  "${CC}" -std=c99 -O2 -fPIC \
    -I"${LUA_SRC}" -c "${source}" -o "${LUA_OBJ}/${base%.c}.o"
done
"${AR}" rcs "${DEPS}/liblua.a" "${LUA_OBJ}"/*.o

build_variant() {
  local flavor="$1"
  local kind="$2"
  local title="$3"
  local obj="${DEPS}/crystal-${flavor}.o"
  local out="app/src/${flavor}/jniLibs/${ABI}"

  mkdir -p "${out}"

  "${CC}" -std=c99 -O2 -fPIC -ffunction-sections -fdata-sections \
    -DPLATFORM_ANDROID -D__ANDROID_API__=21 \
    -DCRYSTAL_KIND="${kind}" "-DCRYSTAL_TITLE=\"${title}\"" \
    "${DIAGNOSTIC_FLAGS[@]}" \
    -I"${DEPS}/raylib/src" -I"${LUA_SRC}" \
    -c app/src/main/cpp/crystal.c -o "${obj}"

  "${CC}" -shared -o "${out}/libcrystal.so" "${obj}" \
    "${DEPS}/liblua.a" "${DEPS}/raylib/src/libraylib.a" \
    -Wl,-soname,libcrystal.so \
    -Wl,-u,ANativeActivity_onCreate \
    -Wl,--no-undefined \
    -Wl,--gc-sections \
    -Wl,--wrap=fopen \
    -landroid -llog -lEGL -lGLESv2 -lOpenSLES -ldl -lm -lc -latomic
}

build_variant halite 0 "Crystal Halite"
build_variant quartz 1 "Crystal Quartz"
build_variant bismuth 2 "Crystal Bismuth"

file app/src/*/jniLibs/"${ABI}"/libcrystal.so
