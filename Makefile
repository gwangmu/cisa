CUR_DIR := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
LLVM_BUILD := ${CUR_DIR}/llvm
CISA_BUILD := ${CUR_DIR}/build

#@if [ ! -d ${CISA_BUILD} ]; then \

all: 
	@echo "info: launching cmake.."; \
	mkdir -p ${CISA_BUILD}; \
	PATH=${LLVM_BUILD}/bin:${PATH} \
			 LLVM_ROOT_DIR=${LLVM_BUILD}/bin \
			 LLVM_TOOLS_BINARY_DIR=${LLVM_BUILD}/bin \
			 LLVM_LIBRARY_DIRS=${LLVM_BUILD}/lib \
			 LLVM_INCLUDE_DIRS=${LLVM_BUILD}/include \
			 CC=clang CXX=clang++ \
			 cmake -B${CISA_BUILD} -S. \
					-DCMAKE_BUILD_TYPE=Debug; \
	make -C${CISA_BUILD} -j

clean:
	rm -rf ${CISA_BUILD}
