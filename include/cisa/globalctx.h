#pragma once

#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Analysis/AliasAnalysis.h>
#include "llvm/Support/CommandLine.h"
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <set>

using namespace llvm;
using namespace std;

typedef vector< pair<Module*, StringRef> > ModuleList;
// Mapping module to its file name.
typedef unordered_map<Module*, string> ModuleNameMap;
// The set of all functions.
typedef set<Function *> FuncSet;
typedef SmallPtrSet<CallInst*, 8> CallInstSet;
typedef DenseMap<Function *, set<Function *>> PlainFuncMap;
typedef unordered_map<string, set<string>> AffectMap;
typedef pair<Function *, string> FunctionPathPair;

struct GlobalContext {

	GlobalContext() {}

	// Statistics 
	unsigned NumFunctions = 0;
	unsigned NumFirstLayerTypeCalls = 0;
	unsigned NumSecondLayerTypeCalls = 0;
	unsigned NumSecondLayerTargets = 0;
	unsigned NumValidIndirectCalls = 0;
	unsigned NumIndirectCallTargets = 0;
	unsigned NumFirstLayerTargets = 0;

	// Global variables
	DenseMap<size_t, GlobalVariable *> Globals;
	
	// Map global function GUID (uint64_t) to its actual function with body.
	map<uint64_t, Function*> GlobalFuncMap;

	// Functions whose addresses are taken.
	FuncSet AddressTakenFuncs;

	// Map a callsite to all potential callee functions.
	//CalleeMap Callees;
  PlainFuncMap Callees;

	// Map a function to all potential caller instructions.
	//CallerMap Callers;
  PlainFuncMap Callers;

	// Map function signature to functions
	DenseMap<size_t, FuncSet>sigFuncsMap;

	// Modules.
	ModuleList Modules;
	ModuleNameMap ModuleMaps;
  map<string, Module *> ModuleRevMaps;

  map<string, set<string>> CallerPaths;
  map<size_t, set<string>> CandCallerPaths;
  map<string, map<string, Function *>> BCFuncs;

  // NOTE: If cached, a (AncF, DesF) pair should be either in "CallCache" or
  // "NegCallCache". If it doesn't exist in both, it should be traced in the
  // callgraph and cached accordingly.
  map<Function *, set<Function *>> AncCache;
  map<Function *, set<Function *>> NegAncCache;

  std::set<llvm::Function *> EmptyFuncSet;

public:
  void GetAllFunctions(set<FunctionPathPair> &funcs);
  void GetAllModuleFunctions(string path, set<Function *> &funcs);
  Function* GetFunction(string func, string path = "");

  const set<Function *>& GetCallers(Function *F);
  const set<Function *>& GetCallees(Function *F);

  enum CGWalkerReturn { NEXT, SKIP, STOP };
  enum CGWalkerDir { DOWN, UP };
  typedef function<CGWalkerReturn(Function *)> CGWalkerActor;
  void VisitSubtreeBFS(Function *F, CGWalkerDir dir, CGWalkerActor action);

  bool IsAncestor(Function *AncF, Function *DesF);
};

namespace CISA {
  GlobalContext& GetGlobalContext(); 
}
