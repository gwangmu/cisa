#include "analyzer.h"

#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/DebugInfoMetadata.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/Function.h>

#include <iostream>
#include <queue>
#include <string>

#include <cisa/log.h>
#include <cisa/globalctx.h>

using namespace llvm;

typedef std::string SubTag;

class SubsystemLevelAnalyzer: public Analyzer<SubTag> {
private:
  // XXX: libsndfile, from include/sndfile.h.
  const std::set<std::string> subsyses = { "wav", "aiff", "au", "raw", "paf",
    "svx", "nist", "voc", "ircam", "w64", "mat4", "mat5", "pvf", "xi", "htk",
    "sds", "avr", "wavex", "sd2", "flac", "caf", "wve", "ogg", "mpc2k", "rf64",
    "mpeg" };

  GlobalContext &gctx;

public:
  SubsystemLevelAnalyzer(): Analyzer("sub"), 
      gctx(CISA::GetGlobalContext()) {}

  void Update(Function *F) {
    SubTag prefix; {
      int first_us = F->getName().str().find("_");
      if (first_us != std::string::npos)
        prefix = F->getName().str().substr(0, first_us);
    }

    if (subsyses.find(prefix) != subsyses.end())
      AddTag(prefix, F);
  }

  AnalyzerReturn Aggregate(llvm::Function *F) {
    RelSet subsys;

    gctx.VisitSubtreeBFS(F, GlobalContext::UP,
      [&](llvm::Function *CurF) {
        auto &cur_tags = GetTags(CurF);
        subsys.insert(cur_tags.begin(), cur_tags.end());
        return GlobalContext::NEXT;
      });

    if (subsys.empty())
      subsys.insert(subsyses.begin(), subsyses.end());

    return AnalyzerReturn{{ "subsys", subsys }};
  }
};

REGISTER_ANALYZER(SubsystemLevelAnalyzer)
