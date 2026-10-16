#!/bin/sh
# Builds each board and merges its bootloader, partition table and firmware into one image
# to flash at 0x0: tools/merge.sh [board...]   (default: all, images in dist/)
#   boards: l2 tdeck m9 xiao   (build-<board>, configured with idf.py -B build-<board> -DBOARD=...)
cd "$(dirname "$0")/.."
# ESP-IDF 5.5 from PlatformIO when it is not exported
if [ -z "$IDF_PATH" ]; then
    export IDF_PATH="$HOME/.platformio/packages/framework-espidf@src-6c2e555ff1cf079c2c8f517e77b082e1"
    export IDF_PYTHON_ENV_PATH="$HOME/.platformio/penv/.espidf-5.5.5"
    export PATH="$HOME/.platformio/tools/toolchain-xtensa-esp-elf/bin:$HOME/.platformio/tools/tool-cmake/bin:$HOME/.platformio/tools/tool-ninja:$IDF_PYTHON_ENV_PATH/bin:$IDF_PATH/tools:$PATH"
fi
mkdir -p dist
for board in ${@:-l2 tdeck m9 xiao}; do
    case $board in
        l2)    name=wio-tracker-l2 ;;
        tdeck) name=t-deck ;;
        m9)    name=thinknode-m9 ;;
        xiao)  name=xiao-esp32s3 ;;
        *)     echo "unknown board: $board"; exit 1 ;;
    esac
    b=build-$board
    if ! ninja -C "$b" > "$b/ninja.log" 2>&1; then
        grep -E "error|undefined reference" "$b/ninja.log" | grep -v "^/.*-c /" | sort -u | head -30
        echo "BUILD FAILED: $board"
        exit 1
    fi
    (cd "$b" && python -m esptool --chip esp32s3 merge_bin -o "../dist/QuakeESP32S3-$name.bin" @flash_args 2>&1 | grep -iE "wrote|error")
done
