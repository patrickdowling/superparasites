#!/usr/bin/env sh
export TOOLCHAIN_PATH="$(realpath "$(find /toolchain -maxdepth 1 -name "gcc-arm-none-eabi*")")/"
export PYTHONPATH=/build

$@
