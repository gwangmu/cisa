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

typedef std::string OLATag;
enum OLATagIdx { MOD, REF, N_OLATags };

class OperationLevelAnalyzer: public Analyzer<OLATag, N_OLATags> {
private:
  GlobalContext &gctx;

  std::map<std::string, std::set<OLATag>> toplv_tags[N_OLATags];

  std::set<OLATag>& GetOrFindTopLVTags(std::string sc_name, OLATagIdx idx) {
    auto &tags = toplv_tags[idx][sc_name];

    if (toplv_tags[idx][sc_name].empty()) {
      // Gather all descendants.
      gctx.VisitSubtreeBFS(
        gctx.GetFunction(sc_name), GlobalContext::DOWN,
        [&](llvm::Function *CurF) {
          auto _tags = GetTags(CurF, idx);
          tags.insert(_tags.begin(), _tags.end());
          return GlobalContext::NEXT;
        });
    }

    return tags;
  }

public:
  OperationLevelAnalyzer(): Analyzer("oper"),
      gctx(CISA::GetGlobalContext()) {}

  OLATag GetStructFieldTag(StructType *STy, int no, 
      std::map<std::string, std::vector<std::string>> fnames) { 
    auto processed_name = STy->getName().str();
    int _dotpos = processed_name.find(".");
    if (_dotpos == std::string::npos) return "TAG_ERROR";
    processed_name = processed_name.substr(_dotpos + 1);
    if (fnames.find(processed_name) == fnames.end() ||
        fnames[processed_name].size() <= no)  
      return STy->getName().str() + "::_" + std::to_string(no); 
    else
      return STy->getName().str() + "::" + fnames[processed_name][no]; 
  }

  OLATag GetTypedefTag(DIDerivedType *DI)
  { return DI->getName().str(); }

  // TODO: testing purposes.
  bool IsTopLV(Function *F) 
  { return (F->getName().str().substr(0, 3) == "sf_"); }

  void Update(Function *F) {
    // Manage "toplv_tags".
    if (IsTopLV(F)) {
      for (int i = 0; i < N_OLATags; i++)
        toplv_tags[i][F->getName().str()].clear();
    } else {
      // TODO: invalidate pre-change toplv deps.
      // TODO: update toplv deps after changes.
      for (auto &[sc_name, sc_tags] : toplv_tags[0]) {
        if (sc_tags.empty()) continue;
        auto TopLVF = gctx.GetFunction(sc_name);
        assert(TopLVF);
        auto is_anc = gctx.IsAncestor(TopLVF, F);

        Logger::Log(0, std::make_pair(sc_name, F->getName().str()), is_anc);

        if (is_anc)
          sc_tags.clear();
      }
    }

    // FIXME: this is done in every function in a module. INEFFICIENT!
    DebugInfoFinder Finder;
    Finder.processModule(*F->getParent());

    std::map<std::string, std::vector<std::string>> fnames;
    for (auto DT : Finder.types()) {
      auto _DT = DT;
      std::string _dname = "";

      if (auto DDT = dyn_cast<DIDerivedType>(DT)){
        if (DDT->getBaseType()) {
          _DT = DDT->getBaseType();
          _dname = DDT->getName().str();
        }
      }

      if (auto DCT = dyn_cast<DICompositeType>(_DT)) {
        if (_dname.empty())
          _dname = DCT->getName().str();

        auto &_fnames = fnames[_dname];
        auto elems = DCT->getElements();
        for (int i = 0; i < elems.size(); i++) {
          if (auto FDT = dyn_cast<DIType>(elems[i])) 
            _fnames.push_back(FDT->getName().str());
          else
            _fnames.push_back("");
        }
      }
    }

    for (inst_iterator i = inst_begin(F), e = inst_end(F); i != e; ++i) {
      Instruction *I = &*i;
      
      // Search for all struct field dereferences.
      std::queue<User *> wq({I});
      while (!wq.empty()) {
        auto V = wq.front();
        wq.pop();

        if (auto GEPOp = dyn_cast<GEPOperator>(V)) {
          bool refed = false;
          bool moded = false;
          for (auto U : V->users()) {
            if (auto SI = dyn_cast<StoreInst>(U)) {
              if (SI->getPointerOperand() == GEPOp) 
                moded = true;
            } else if (isa<LoadInst>(U)) 
              refed = true;
          }

          if (refed || moded) {
            Type *PTy = GEPOp->getPointerOperandType();
            Type *NextTy = PTy->getNonOpaquePointerElementType();

            for (int gep_i = 2; gep_i < GEPOp->getNumOperands(); gep_i++) {
              auto *STy = dyn_cast<StructType>(NextTy);
              int fidx = 0;

              if (STy && (STy->getName().str().substr(0, 7) == "struct." ||
                    STy->getName().str().substr(0, 6) == "union.") && 
                  STy->getName().str().find("anon") == std::string::npos &&
                  STy->getName().str().find("_IO_FILE") == std::string::npos &&
                  STy->getName().str().find("sigaction") == std::string::npos &&
                  STy->getName().str().find("buffer") == std::string::npos &&
                  STy->getName().str().find("ebug") == std::string::npos &&
                  STy->getName().str().find("_va_list") == std::string::npos) {
                if (auto FIdx = dyn_cast<ConstantInt>(GEPOp->getOperand(gep_i))) {
                  fidx = FIdx->getValue().getLimitedValue();
                  std::string _tag = GetStructFieldTag(STy, fidx, fnames);
                  if (_tag.find("err") == std::string::npos &&
                      _tag.find("log") == std::string::npos &&
                      _tag.find("Error") == std::string::npos) {
                    if (moded) AddTag(_tag, F, MOD);
                    if (refed) AddTag(_tag, F, REF);
                  }
                }
              }

              if (NextTy->getNumContainedTypes() == 0) break;
              NextTy = NextTy->getContainedType(fidx);
            }
          }
        }

        for (const auto &_O : V->operand_values()) {
          if (isa<Instruction>(&*_O)) continue;
          if (auto UU = dyn_cast<User>(&*_O)) 
            wq.push(UU);
        }
      }

      /* XXX: temporarily disabled.
      // Search for all used typedef types.
      if (auto DVI = dyn_cast<DbgVariableIntrinsic>(I)) {
        auto VI = DVI->getVariable();
        auto DTDI = dyn_cast<DIDerivedType>(VI->getType());
        if (DTDI && DTDI->getBaseType() &&
            isa<DIBasicType>(DTDI->getBaseType())) {
          auto tname = DTDI->getName().str();
          if (!tname.empty() &&
              tname.find("int") == std::string::npos &&
              tname.find("size") == std::string::npos &&
              tname.find("intptr") == std::string::npos &&
              tname.find("ptrdiff") == std::string::npos &&
              tname.find("nteger") == std::string::npos &&
              tname.find("nsigned") == std::string::npos &&
              tname.find("umber") == std::string::npos &&
              tname.find("idx") == std::string::npos &&
              tname.find("Idx") == std::string::npos &&
              tname.find("yte") == std::string::npos &&
              tname != "u8" && tname != "u16" &&
              tname != "u32" && tname != "u64" &&
              tname != "s8" && tname != "s16" &&
              tname != "s32" && tname != "s64")
            AddTag(GetTypedefTag(DTDI), F);
        }
      }*/
    }

    Logger::Log(0, "UPDATE:", F->getName().str(), GetTags(F, MOD)); 
  }

  AnalyzerReturn Aggregate(llvm::Function *F) {
    RelSet toplvs;

    std::pair<OLATagIdx, OLATagIdx> tp[2] = {
      std::make_pair(MOD, REF),
      std::make_pair(REF, MOD),
    };

    for (auto &_p : tp) {
      std::set<OLATag> accum_tags;
      auto ch_ti = _p.first;
      auto sc_ti = _p.second;

      gctx.VisitSubtreeBFS(F, GlobalContext::DOWN,
        [&](llvm::Function *CurF) {
          auto &cur_tags = GetTags(CurF, ch_ti);
          Logger::Log(0, "CHILD:", CurF->getParent()->getName().str(), CurF->getName().str(), cur_tags);

          accum_tags.insert(cur_tags.begin(), cur_tags.end());
          return GlobalContext::NEXT;
        });

      Logger::Log(0, "ACCUM_TAGS:", F->getName().str(), accum_tags);

      for (auto &[sc_name, _] : toplv_tags[0]) {
        auto &sc_tag = GetOrFindTopLVTags(sc_name, sc_ti);
        std::vector<OLATag> res;

        Logger::Log(0, "SC_TAG:", sc_name, sc_tag);

        std::set_intersection(sc_tag.begin(), sc_tag.end(),
            accum_tags.begin(), accum_tags.end(), 
            std::inserter(res, res.begin()));
        if (!res.empty())
          toplvs.insert(sc_name);
      }
    }

    // If empty, put all top level functions to the set.
    if (toplvs.empty()) {
      for (auto &[sc_name, _] : toplv_tags[0])
        toplvs.insert(sc_name);
    }

    return AnalyzerReturn{{ "toplvs", toplvs }};
  }
};

REGISTER_ANALYZER(OperationLevelAnalyzer)
