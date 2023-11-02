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

#include "TypeDive.h"
#include "CallGraph.h"
#include "Config.h"
#include "Common.h"

// gwangmu: the best solution would be using somewhat of the class, but I'm too
// lazy to modify the relevant class the right way. Just making some global
// functions will do.
int TypeDive::Initialize(std::vector<std::string> vecBCPaths, 
    std::string strLogPath, bool strict)
{
	SMDiagnostic Err;
  std::string _prefix = (strict) ? "error: " : "warning: ";

  // Set the log ofstream.
  if (!strLogPath.empty()) {
    std::ofstream *_OP = new std::ofstream(strLogPath);
    if (!_OP->is_open()) {
      std::cerr << _prefix << "cannot open log file.\n";
      if (strict) return -1;
    } else {
      OP = _OP;
    }
  }

  // Load BC files.
  for (auto strBCPath : vecBCPaths) {
		LLVMContext *LLVMCtx = new LLVMContext();
		std::unique_ptr<Module> M = parseIRFile(strBCPath, Err, *LLVMCtx);

		if (M == nullptr) {
      std::cerr << _prefix << "cannot load file '" << strBCPath << "'.\n";
      continue;
      if (strict) return -1;
			continue;
		}

		Module *Module = M.release();
		StringRef MName = StringRef(strdup(strBCPath.data()));
		GlobalCtx.Modules.push_back(std::make_pair(Module, MName));
		GlobalCtx.ModuleMaps[Module] = strBCPath;
    GlobalCtx.ModuleRevMaps[strBCPath] = Module;
	}

  // Build a callgraph.
  CGPass = new CallGraphPass(&GlobalCtx);
  CGPass->run(GlobalCtx.Modules);

  return 0;
}

int TypeDive::UpdateModules(std::vector<std::string> vecBCPaths)
{
	SMDiagnostic Err;
  ModuleList mlist;
  std::set<std::string> aff_set;

  // Invalidate results from BCs in the list.
  CGPass->initTempStates();

  // Load BC files.
  for (auto strBCPath : vecBCPaths) {
		LLVMContext *LLVMCtx = new LLVMContext();
		std::unique_ptr<Module> M = parseIRFile(strBCPath, Err, *LLVMCtx);

    // NOTE: in case of delete files, M can be nullptr.

		Module *Module = M.release();
		StringRef MName = StringRef(strdup(strBCPath.data()));

    mlist.push_back(std::make_pair(Module, MName));

    if (Module) {
      auto AddAff = [&](std::string path) {
        if (std::find(vecBCPaths.begin(), vecBCPaths.end(), path) 
            != vecBCPaths.end())
          return;
        StringRef _AffMName = StringRef(strdup(path.data()));
        if (aff_set.find(path) == aff_set.end()) {
          mlist.push_back(std::make_pair(GlobalCtx.ModuleRevMaps[path],
                _AffMName));
          aff_set.insert(path);
        }
      };

      // Update "CandCallerPaths".
      for (auto &_p : GlobalCtx.CandCallerPaths)
        _p.second.erase(strBCPath);

      // Search for address-taken functions.
      // Update "_extended_mlist" that "can" call original "mlist".
      for (auto &_F : Module->functions()) {
        Function *F = &_F;
        
        if (F->hasAddressTaken()) {
          for (auto &path : GlobalCtx.CandCallerPaths[funcHash(&_F)]) 
            AddAff(path);
        }
      }

      for (auto &path : GlobalCtx.CallerPaths[strBCPath])
        AddAff(path);

      GlobalCtx.CallerPaths.erase(strBCPath);
      for (auto &_p : GlobalCtx.CallerPaths)
        _p.second.erase(strBCPath);
    }
  }

  // Invalidate post-analysis results.
  for (auto &_pp : mlist) {
    std::string strBCPath = _pp.second.str();
    for (auto &_p : GlobalCtx.BCFuncs[strBCPath]) {
      auto func = _p.second;
      for (auto funcCallee : GlobalCtx.Callees[func])
        GlobalCtx.Callers[funcCallee].erase(func);
      GlobalCtx.Callees.erase(func);
      GlobalCtx.AncCache.erase(func);
      for (auto &_p : GlobalCtx.AncCache)
        _p.second.erase(func);
      GlobalCtx.NegAncCache.erase(func);
      for (auto &_p : GlobalCtx.NegAncCache)
        _p.second.erase(func);
    }

    GlobalCtx.BCFuncs.erase(strBCPath);
  }

  // Invalidate pre-analysis states.
  for (auto &_pp: mlist) {
    Module *Module = _pp.first;
    StringRef MName = _pp.second;
    std::string strBCPath = MName.str();
    bool is_aff = (aff_set.find(strBCPath) != aff_set.end());

    // Update the module map in GlobalCtx.
    int _idx = -1;
    for (auto _it = GlobalCtx.Modules.begin(); 
        _it != GlobalCtx.Modules.end(); _it++) {
      if (_it->second.str() == strBCPath) {
        llvm::Module *M = _it->first;

        CGPass->invalidateStates(M);
        for (auto &_GV : M->globals()) {
          GlobalVariable *GV = &_GV;
          GlobalCtx.Globals.erase(GV->getGUID());
        }
        for (auto &_F : M->functions()) {
          Function *F = &_F;
          if (F->isDeclaration()) continue;
          GlobalCtx.GlobalFuncMap.erase(F->getGUID());
          GlobalCtx.AddressTakenFuncs.erase(F);
          GlobalCtx.sigFuncsMap.erase(funcHash(F, false));
        }

        // Remove modules only if they are NOT "affected" one.
        if (!is_aff) {
          delete _it->first;
          GlobalCtx.Modules.erase(_it);
          for (auto it = GlobalCtx.ModuleMaps.begin(); 
              it != GlobalCtx.ModuleMaps.end(); it++) {
            if (it->second == strBCPath) {
              GlobalCtx.ModuleMaps.erase(it);
              break;
            }
          }
          GlobalCtx.ModuleRevMaps.erase(strBCPath);
          break;
        }
      }
    }

    if (!is_aff && Module) {
      GlobalCtx.Modules.push_back(std::make_pair(Module, MName));
      GlobalCtx.ModuleMaps[Module] = strBCPath;
      GlobalCtx.ModuleRevMaps[strBCPath] = Module;
    }
	}

  // TODO: Remove null modules from "mlist".

  // Build a callgraph.
  CGPass->run(mlist);

  return 0;
}

int TypeDive::Finalize() {
  if (OP != &std::cerr)
    static_cast<std::ofstream *>(OP)->close();
  return 0;
}


void TypeDive::Dump()
{
  for (auto &p : GlobalCtx.Callees) {
    std::cout << p.first->getName().str() << " ->\n"; 
    for (auto func : p.second) {
      if (func->isIntrinsic()) continue;
      std::cout << "  " << func->getName().str();
      auto &set = GlobalCtx.Callers[func];
      if (set.find(p.first) == set.end()) 
        std::cout << "[!]\n";
      else
        std::cout << "\n";
    }
  }
}
