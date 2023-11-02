#pragma once

#include <set>
#include <string>
#include <utility>
#include <vector>

#include <cisa/globalctx.h>
#include <llvm/IR/Module.h>

#include "CallGraph.h"

class TypeDive {
private:
  GlobalContext &GlobalCtx;
  CallGraphPass *CGPass;

public:
  TypeDive(GlobalContext &GlobalCtx) : GlobalCtx(GlobalCtx) {}

  int Initialize(std::vector<std::string> vecBCPaths, 
      std::string strLogPath = "", bool strict = false);
  int UpdateModules(std::vector<std::string> vecBCPaths);
  int Finalize();

  void Dump();
};
