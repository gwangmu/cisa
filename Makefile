CUR_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
LLVM_INSTALL := ${CUR_DIR}/llvm
CISA_BUILD := ${CUR_DIR}/build

all: 
	@if [ ! -d ${LLVM_INSTALL} ]; then \
		echo "error: please create a symlink "llvm" to the llvm install directory."; \
		exit; \
	fi; \
	echo "info: launching cmake.."; \
	mkdir -p ${CISA_BUILD}; \
	PATH=${LLVM_INSTALL}/bin:${PATH} \
			 LLVM_ROOT_DIR=${LLVM_INSTALL}/bin \
			 LLVM_TOOLS_BINARY_DIR=${LLVM_INSTALL}/bin \
			 LLVM_LIBRARY_DIRS=${LLVM_INSTALL}/lib \
			 LLVM_INCLUDE_DIRS=${LLVM_INSTALL}/include \
			 CC=clang CXX=clang++ \
			 cmake -B${CISA_BUILD} -S. \
					-DCMAKE_BUILD_TYPE=Debug; \
	make -C${CISA_BUILD} -j && ln -s scripts/entry.py cisa

clean:
	rm -rf ${CISA_BUILD} && rm cisa
