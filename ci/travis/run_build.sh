#!/bin/bash
set -e

TOP_DIR="$(pwd)"

sudo apt-get update

. ./ci/travis/lib.sh

build_m1k_make() {
	sudo apt-get install -y gcc-arm-none-eabi \
		libstdc++-arm-none-eabi-newlib
	make
}

build_cppcheck() {
	export CPPCHECK_OPTIONS=""
	. ./build/cppcheck.sh
}

build_astyle() {
	export ASTYLE_EXT_LIST=""
	. ./build/astyle.sh
}

build_${BUILD_TYPE}
