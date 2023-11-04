#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;

class IRDumper : public PassInfoMixin<IRDumper> {
public:
	PreservedAnalyses run(Module &M, ModuleAnalysisManager &MAM);
};

PreservedAnalyses IRDumper::run(Module &M, ModuleAnalysisManager &MAM) {
	int bc_fd;

	StringRef FN = M.getName();
	sys::fs::openFileForWrite(
			FN.take_front(FN.size() - 2) + ".bc", bc_fd);
	raw_fd_ostream bc_file(bc_fd, true, true);
	WriteBitcodeToFile(M, bc_file);

  return PreservedAnalyses::all();
}

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo() {
	return {LLVM_PLUGIN_API_VERSION, "IRDumper", "v0.1",
		[](PassBuilder &PB) {
			PB.registerOptimizerLastEPCallback(
				[](ModulePassManager &MPM, OptimizationLevel OL) {
					MPM.addPass(IRDumper());
				});
		}};
}
