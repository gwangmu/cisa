#include "llvm/IR/InstIterator.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/SystemUtils.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/Path.h"

#include <memory>
#include <vector>
#include <sstream>
#include <sys/resource.h>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <cassert>
#include <queue>

#include <cisa/globalctx.h>

#include "callgraph/CallGraph.h"
#include "callgraph/Config.h"
#include "callgraph/Common.h"

void GlobalContext::GetAllFunctions(std::set<FunctionPathPair> &funcs) {
  for (auto &_p : BCFuncs) {
    for (auto &_p2 : _p.second) 
      funcs.insert(FunctionPathPair(_p2.second, _p.first));
  }
}

void GlobalContext::GetAllModuleFunctions(std::string path, 
    std::set<llvm::Function *> &funcs) {
  for (auto &_p2 : BCFuncs[path]) 
    funcs.insert(_p2.second);
}

llvm::Function* GlobalContext::GetFunction(std::string func, std::string path) {
  if (!path.empty()) {
    if (BCFuncs.find(path) == BCFuncs.end())
      return nullptr;
    else if (BCFuncs[path].find(func) == BCFuncs[path].end()) 
      return nullptr;
    else
      return BCFuncs[path][func];
  } else {
    for (auto &_p : BCFuncs) {
      if (_p.second.find(func) != _p.second.end())
        return _p.second[func];
    }
    return nullptr;
  }
}

const std::set<llvm::Function *>& GlobalContext::GetCallers(llvm::Function *F) {
  if (!F) return EmptyFuncSet;
  return Callers[F];
}

const std::set<llvm::Function *>& GlobalContext::GetCallees(llvm::Function *F) {
  if (!F) return EmptyFuncSet;
  return Callees[F];
}

void GlobalContext::VisitSubtreeBFS(llvm::Function *AncF, GlobalContext::CGWalkerDir dir, 
    GlobalContext::CGWalkerActor action) {
  PlainFuncMap *nextmap; {
    switch (dir) {
      case DOWN:  nextmap = &Callees; break;
      case UP:    nextmap = &Callers; break;
      default:    assert(false);
    }
  }

  std::set<llvm::Function *> VisitedFs;
  std::queue<llvm::Function *> FQ({AncF});

  auto PopFromFQ = [&VisitedFs, &FQ]() {
    auto F = FQ.front(); FQ.pop();
    VisitedFs.insert(F);
    return F;
  };

  auto TryPushToFQ = [&VisitedFs, &FQ](llvm::Function *F) {
    if (VisitedFs.find(F) != VisitedFs.end()) return;
    FQ.push(F);
  };

  while (!FQ.empty()) {
    auto CurF = PopFromFQ();
//std::cout << "VISIT: " << CurF->getName().str() << "\n";
    auto ret = action(CurF);
    
    switch (ret) {
      case GlobalContext::SKIP: continue;
      case GlobalContext::STOP: break;
      case GlobalContext::NEXT: /* fallthrough */;
    }

    for (auto &NextF : (*nextmap)[CurF])
      TryPushToFQ(NextF);
  }
}

// Check if "AncF" is the ancestor of "DesF".
bool GlobalContext::IsAncestor(llvm::Function *AncF, llvm::Function *DesF) {
  if (AncF == DesF) return false;

  auto IsCachedAnc = [this](llvm::Function *AncF, llvm::Function *DesF) {
    return (AncCache[AncF].find(DesF) !=
        AncCache[AncF].end());
  };

  auto IsCachedNegAnc = [this](llvm::Function *AncF, llvm::Function *DesF) {
    return (NegAncCache[AncF].find(DesF) !=
        NegAncCache[AncF].end());
  };

  auto &AncCacheDF = AncCache[DesF];
  auto &NegAncCacheDF = NegAncCache[DesF];

  if (AncCacheDF.find(AncF) != AncCacheDF.end())
    return true;

  if (NegAncCacheDF.find(AncF) != NegAncCacheDF.end())
    return false;

  // Non-cached case: trace the callgraph to figure it out.
  bool is_anc = false;
  VisitSubtreeBFS(AncF, GlobalContext::DOWN,
    [&](llvm::Function *CurF) {
      if (CurF == DesF || IsCachedAnc(CurF, DesF)) {
        is_anc = true;
        return GlobalContext::STOP;
      } else if (IsCachedNegAnc(CurF, DesF)) 
        return GlobalContext::SKIP;
      else
        return GlobalContext::NEXT;
    });

  if (is_anc) AncCacheDF.insert(AncF);
  else        NegAncCacheDF.insert(AncF);

  return is_anc;
}

#if 0
// Check if the name starts with "__.*_compat_sys".
bool GlobalContext::IsSyscall(llvm::Function *F) {
  if (!F) return false;
  std::string name = F->getName().str();

  int third_underbar = -1; {
    int nth = 0;
    int pos = 0;
    while (nth < 3) {
      pos = name.find("_", pos);
      if (pos == std::string::npos)
        return false;
      pos++;
      nth++;
    }
  }

  return (name.substr(third_underbar, 11) == "_compat_sys");
}
#endif
