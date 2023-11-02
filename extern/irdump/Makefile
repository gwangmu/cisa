CUR_DIR := $(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))
LLVM_INSTALL_DIR = ${CUR_DIR}/llvm
BUILD_DIR := ${CUR_DIR}/build
NPROC := 4

all: ${LLVM_INSTALL_DIR} ${BUILD_DIR}
	@echo "info: building.."; 
	@make -C${BUILD_DIR} -j${NPROC} 

${LLVM_INSTALL_DIR}:
	@echo "fatal: '${LLVM_INSTALL_DIR}' not found."
	@echo "fatal: create a symlink to the LLVM install directory at '${LLVM_INSTALL_DIR}'" 
	@exit 1;

${BUILD_DIR}:
	@echo "info: launching cmake.."
	@mkdir -p ${BUILD_DIR} 
	@PATH=${LLVM_INSTALL_DIR}/bin:${PATH} \
		LLVM_ROOT_DIR=${LLVM_INSTALL_DIR}/bin \
		LLVM_LIBRARY_DIRS=${LLVM_INSTALL_DIR}/lib \
		LLVM_INCLUDE_DIRS=${LLVM_INSTALL_DIR}/include \
		CC=clang CXX=clang++ \
		cmake -S. -B${BUILD_DIR} \
			-DCMAKE_BUILD_TYPE=Debug \
			-DCMAKE_CXX_FLAGS_DEBUG="-fno-rtti -fpic"

clean:
	rm -rf ${BUILD_DIR}
