#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

using namespace llvm;

class IRDumper : public ModulePass {
public:
	static char ID;
	IRDumper() : ModulePass(ID) {}
	virtual bool runOnModule(Module &M);
};

bool IRDumper::runOnModule(Module &M) {
	int bc_fd;

	StringRef FN = M.getName();
	sys::fs::openFileForWrite(
			FN.take_front(FN.size() - 2) + ".bc", bc_fd);
	raw_fd_ostream bc_file(bc_fd, true, true);
	WriteBitcodeToFile(M, bc_file);

	return false;
}

char IRDumper::ID = 0;
static RegisterPass<IRDumper> X("IRDumper", "dump IR to file", false, false);

/* Pass registration (legacy) */

static void registerIRDumper(const PassManagerBuilder &PMB,
		legacy::PassManagerBase &PM) 
{	PM.add(new IRDumper()); }

static RegisterStandardPasses RegisterIRDumperPass(
		PassManagerBuilder::EP_OptimizerLast, registerIRDumper);
static RegisterStandardPasses RegisterIRDumperPassL0(
		PassManagerBuilder::EP_EnabledOnOptLevel0, registerIRDumper);
