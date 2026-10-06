#!/usr/bin/env bash
set -Eeuo pipefail

: "${ANDROID_SDK_ROOT:?ANDROID_SDK_ROOT is required}"
NDK_VERSION="26.3.11579264"
NDK="${ANDROID_SDK_ROOT}/ndk/${NDK_VERSION}"
TOOLCHAIN="${NDK}/toolchains/llvm/prebuilt/linux-x86_64"
CC="${TOOLCHAIN}/bin/armv7a-linux-androideabi21-clang"
AR="${TOOLCHAIN}/bin/llvm-ar"
STRIP="${TOOLCHAIN}/bin/llvm-strip"
RAYLIB_COMMIT="dbc56a87da87d973a9c5baa4e7438a9d20121d28"

for required in "$CC" "$AR" "$STRIP"; do
  [[ -x "$required" ]] || { echo "missing NDK tool: $required" >&2; exit 1; }
done

DEPS="${RUNNER_TEMP:-/tmp}/crystal-miro-deps"
rm -rf "$DEPS"
mkdir -p "$DEPS" build/native build/receipts

git clone --filter=blob:none https://github.com/raysan5/raylib.git "$DEPS/raylib"
git -C "$DEPS/raylib" checkout --detach "$RAYLIB_COMMIT"
test "$(git -C "$DEPS/raylib" rev-parse HEAD)" = "$RAYLIB_COMMIT"

curl -fsSL https://www.lua.org/ftp/lua-5.4.8.tar.gz -o "$DEPS/lua.tar.gz"
tar -xzf "$DEPS/lua.tar.gz" -C "$DEPS"

make -C "$DEPS/raylib/src" \
  PLATFORM=PLATFORM_ANDROID \
  ANDROID_ARCH=arm \
  ANDROID_API_VERSION=21 \
  ANDROID_NDK="$NDK" \
  RAYLIB_BUILD_MODE=RELEASE \
  RAYLIB_LIBTYPE=STATIC \
  RAYLIB_MODULE_AUDIO=FALSE \
  -j2

LUA_SRC="$DEPS/lua-5.4.8/src"
LUA_OBJ="$DEPS/lua-objects"
mkdir -p "$LUA_OBJ"
for source in "$LUA_SRC"/*.c; do
  base="$(basename "$source")"
  case "$base" in lua.c|luac.c) continue ;; esac
  "$CC" -std=c99 -O2 -fPIC -I"$LUA_SRC" -c "$source" -o "$LUA_OBJ/${base%.c}.o"
done
"$AR" rcs "$DEPS/liblua.a" "$LUA_OBJ"/*.o

build_variant() {
  local flavor=$1 kind=$2 title=$3
  local obj="$DEPS/crystal-$flavor.o"
  local out="build/native/$flavor/armeabi-v7a/libcrystal.so"
  mkdir -p "$(dirname "$out")"

  "$CC" -std=c99 -O2 -fPIC -ffunction-sections -fdata-sections \
    -DPLATFORM_ANDROID -D__ANDROID_API__=21 \
    -DCRYSTAL_KIND="$kind" "-DCRYSTAL_TITLE=\"$title\"" \
    -I"$DEPS/raylib/src" -I"$LUA_SRC" \
    -c app/src/main/cpp/crystal.c -o "$obj"

  "$CC" -shared -o "$out" "$obj" \
    "$DEPS/liblua.a" "$DEPS/raylib/src/libraylib.a" \
    -Wl,-soname,libcrystal.so \
    -Wl,-u,ANativeActivity_onCreate \
    -Wl,--no-undefined -Wl,--gc-sections -Wl,--wrap=fopen \
    -landroid -llog -lEGL -lGLESv2 -lOpenSLES -ldl -lm -lc -latomic

  local before after
  before=$(stat -c %s "$out")
  "$STRIP" --strip-unneeded "$out"
  after=$(stat -c %s "$out")
  file "$out" | grep -Fq 'ELF 32-bit'
  printf '%s\t%s\t%s\n' "$flavor" "$before" "$after" >> build/receipts/native-sizes.tsv
}

printf 'flavor\tpre_strip_bytes\tpost_strip_bytes\n' > build/receipts/native-sizes.tsv
build_variant halite 0 "Crystal Halite"
build_variant quartz 1 "Crystal Quartz"
build_variant bismuth 2 "Crystal Bismuth"

sha256sum build/native/*/armeabi-v7a/libcrystal.so > build/receipts/native-sha256.txt
cat build/receipts/native-sizes.tsv
cat build/receipts/native-sha256.txt
