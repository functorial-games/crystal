#!/usr/bin/env bash
set -euo pipefail

OUT="${1:-emulator-evidence}"
mkdir -p "${OUT}"

adb wait-for-device
timeout 300 bash -c '
  until [[ "$(adb shell getprop sys.boot_completed 2>/dev/null | tr -d "\r")" == "1" ]]; do
    sleep 2
  done
'

adb shell wm size 576x1152
adb shell wm density 240
adb shell settings put system accelerometer_rotation 0
adb shell settings put system user_rotation 0
adb shell input keyevent 82 || true

adb shell getprop > "${OUT}/getprop.txt"
adb shell wm size > "${OUT}/wm-size.txt"
adb shell wm density > "${OUT}/wm-density.txt"
adb shell dumpsys SurfaceFlinger > "${OUT}/surfaceflinger.txt" || true

wait_for_pid() {
  local package="$1"
  for _ in $(seq 1 30); do
    if adb shell pidof "${package}" >/dev/null 2>&1; then
      return 0
    fi
    sleep 0.25
  done
  echo "process did not start: ${package}" >&2
  return 1
}

capture() {
  local material="$1"
  local state="$2"
  adb exec-out screencap -p > "${OUT}/${material}-${state}.png"
  test -s "${OUT}/${material}-${state}.png"
}

run_material() {
  local material="$1"
  local package="$2"
  local apk="$3"

  echo "=== ${material} ==="
  unzip -l "${apk}" | tee "${OUT}/${material}-apk-list.txt" | grep -q 'lib/x86_64/libcrystal.so'

  adb install -r "${apk}" | tee "${OUT}/${material}-install.txt"
  adb logcat -c
  adb shell am force-stop "${package}" || true
  adb shell am start -W -n "${package}/android.app.NativeActivity" | tee "${OUT}/${material}-start.txt"
  wait_for_pid "${package}"
  sleep 1

  # Default mode is BOTH. Emulator builds disable auto-spin, so these frames
  # are deterministic except for the explicitly injected input below.
  capture "${material}" "both"

  # Bottom-right mode button: BOTH -> SOLID -> NET.
  adb shell input tap 481 1089
  sleep 0.35
  capture "${material}" "solid"

  adb shell input tap 481 1089
  sleep 0.35
  capture "${material}" "net"

  # Rotate the object with a single deterministic drag while NET is visible.
  adb shell input swipe 180 560 410 680 600
  sleep 0.35
  capture "${material}" "net-rotated"

  adb logcat -d -v threadtime > "${OUT}/${material}-logcat.txt"
  if grep -E 'FATAL EXCEPTION|Fatal signal|ANR in org\.isomorphisms\.crystal'       "${OUT}/${material}-logcat.txt"; then
    echo "fatal Android/native failure in ${material}" >&2
    return 1
  fi

  adb shell am force-stop "${package}"
}

run_material   halite   org.isomorphisms.crystal.inspect.halite   app/build/outputs/apk/halite/debug/app-halite-debug.apk

run_material   quartz   org.isomorphisms.crystal.inspect.quartz   app/build/outputs/apk/quartz/debug/app-quartz-debug.apk

run_material   bismuth   org.isomorphisms.crystal.inspect.bismuth   app/build/outputs/apk/bismuth/debug/app-bismuth-debug.apk

python3 .github/scripts/check-emulator-images.py "${OUT}"
