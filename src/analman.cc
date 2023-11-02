#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>

#include <cisa/globalctx.h>
#include <cisa/log.h>

#include "callgraph/TypeDive.h"
#include "analyzer/analyzer.h"
#include "analman.h"

#include <llvm/IR/DebugInfoMetadata.h>

namespace AnalyzerManager {
  TypeDive *td;
  GlobalContext *gctx;

  std::string out_path;
  std::vector<IAnalyzer *> analyzers;

  typedef std::pair<llvm::Function *, int> FunctionLinePair;

  struct Context {
    std::string hexsha;
    std::map<std::string, ChangeInfo> cis;
    std::map<llvm::Function *, std::map<std::string, AnalyzerReturn>> res;

    void Initialize(std::string _hexsha, std::vector<ChangeInfo> _cis) {
      hexsha = _hexsha;
      cis.clear();
      res.clear();

      for (auto &ci : _cis)
        cis[ci.bc_path] = ci;
    }
  } cur;

  void _Update(llvm::Function *F) {
    for (auto anal : analyzers)
      anal->Remove(F);
    for (auto anal : analyzers)
      anal->Update(F);
  }
  
  void _Aggregate(llvm::Function *F) {
    for (auto anal : analyzers) {
      auto ret = anal->Aggregate(F);

      Logger::Log(0, "AGG:", F->getName().str(), *((std::set<std::string>*)&ret.begin()->second));

      auto &res = cur.res[F][anal->Name()];
      for (auto &[name, relset] : ret)
        res[name].insert(relset.begin(), relset.end());
    }
  }

  int _MapPostToPreLine(std::string path, int post_line) {
    // NOTE: lines should have been sorted in ascending order.
    auto &del_lines = cur.cis[path].del_lines;
    auto &add_lines = cur.cis[path].add_lines; 

    int n_unchanged = post_line; {
      for (auto add_l : add_lines) {
        if (add_l > post_line) break;
        n_unchanged--;
      }
    }

    int pre_line = 0; {
      for (auto del_l : del_lines) {
        int new_nn = n_unchanged - (del_l - pre_line - 1);
        if (new_nn <= 0) break;
        pre_line = del_l;
        n_unchanged = new_nn;
      }
      pre_line += n_unchanged;
    }

    return pre_line;
  }

  void _CorrectChanges(std::string path, std::vector<FunctionLinePair> del_flps,
      std::vector<FunctionLinePair> new_flps) {
    // FIXME: added functions' tags are matched to functions in promixity.
    for (auto &newFL : new_flps) {
      // Find the matching deleted function.
      // Assume a deleted function is matching if its declaration is within
      // the range of [-3, +3] around the pre-commit-mapped line number of the
      // declaration of "newF".
      auto newF = newFL.first;
      llvm::Function *matching_delF = nullptr; 
      int matching_line = 0;
      for (int r = 0; r <= 3 && !matching_delF; r++) {
        int pre_new_line = _MapPostToPreLine(path, newFL.second); 
        for (auto &delFL : del_flps) {
          int diff = delFL.second - pre_new_line;
          if (diff >= -r && diff <= r) {
            matching_delF = delFL.first;
            matching_line = delFL.second;
            break;
          }
        }
      }

      if (matching_delF) {
        for (auto anal : analyzers) {
          Logger::Log(0, "MOVE:", std::make_pair(path + ":" + std::to_string(matching_line), newF->getName().str()));
          anal->Move(matching_delF, newF);
        }
        for (auto &[anal_name, aret] : cur.res[matching_delF]) {
          auto &aret_res = cur.res[newF][anal_name];
          for (auto &[name, relset] : aret)
            aret_res[name].insert(relset.begin(), relset.end());
        }
      }
    }

    // Remove state data.
    for (auto &[delF, _] : del_flps) {
      for (auto anal : analyzers)
        anal->Remove(delF);
      cur.res.erase(delF);
    }
  }

  void _Commit() {
    auto PrintJson = [](std::ofstream& ofs, AnalyzerReturn &aret) {
      ofs << "{\n";
      int idx = 0;
      for (auto &[name, relset] : aret) {
        ofs << "  \"" << name << "\": [\n";
        for (auto sc : relset)
          ofs << "    \"" << sc << "\",\n";

        if (++idx >= aret.size())   ofs << "  ]\n";
        else                        ofs << "  ],\n";
      }
      ofs << "}\n";
    };

    auto PrintOneAnalRes = [&PrintJson](std::string analname, llvm::Function *F) {
      if (cur.res[F][analname].empty()) return;

      std::string funcname = F->getName().str();
      std::filesystem::path func_path(out_path + "/" + cur.hexsha + "/" + 
          funcname);

      if (!std::filesystem::is_directory(func_path)) {
        bool ret = std::filesystem::create_directory(func_path);
        assert(ret);
      }

      std::string res_path = out_path + "/" + cur.hexsha + "/" + 
        funcname + "/" + analname;

      std::ofstream ofs(res_path);
      PrintJson(ofs, cur.res[F][analname]);
      ofs.close();
    };

    auto PrintOneFunction = [&PrintOneAnalRes](llvm::Function *F) {
      std::filesystem::path com_path(out_path + "/" + cur.hexsha);
      if (!std::filesystem::is_directory(com_path))
        assert(std::filesystem::create_directory(com_path));

      for (auto &[analname, _] : cur.res[F])
        PrintOneAnalRes(analname, F);
    };

    for (auto &[F, _] : cur.res) 
      PrintOneFunction(F);
  }

  int _GetPrototypeLine(llvm::Function *F) {
    auto DISub = F->getSubprogram();
    if (!DISub) return 0;
    return DISub->getLine();
  }

  std::vector<llvm::Function *> _GetChangedFuncs(
      const std::set<llvm::Function *>& funcs,
      const std::vector<int>& ch_lines) {
    std::vector<llvm::Function *> ret;

    Logger::Log(0, "FUNCS:", funcs); 
    Logger::Log(0, "CH_LINES:", ch_lines); 
    
    for (auto &F : funcs) {
      int head_line = _GetPrototypeLine(F);
      int tail_line = 0; {
        // XXX: is it really the last instruction?
        auto DLoc = F->back().back().getDebugLoc();
        if (DLoc) tail_line = DLoc.getLine();
      }

      for (auto ch_line : ch_lines) {
        if (ch_line >= head_line && ch_line <= tail_line) {
          ret.push_back(F);
          break;
        }
      }
    }

    return ret;
  }

  // "Redef" = deleted or added.
  std::vector<FunctionLinePair> _GetRedefFuncs(
      const std::set<llvm::Function *>& funcs,
      const std::vector<int>& ch_lines) {
    std::vector<FunctionLinePair> ret; {
      for (auto &F : funcs) {
        int proto_line = _GetPrototypeLine(F);
        ret.push_back(FunctionLinePair(F, proto_line));

        // NOTE: Non-name-changed functions need to be matched too.
        //if (std::find(ch_lines.begin(), ch_lines.end(), proto_line) !=
        //    ch_lines.end())
      }
    }

    return ret;
  }

  void Initialize(std::vector<std::string> bclist_paths, std::string _out_path) {
    const bool LOG_TO_STDOUT = true;

    // Initialize output dir.
    out_path = _out_path;
    
    // Initialize call graph generator (TypeDive for now).
    // FIXME: module management should be detached from TypeDive. 
    gctx = &CISA::GetGlobalContext();
    td = new TypeDive(*gctx);
    
    std::string log_path = out_path + "/log.typedive";
    if (LOG_TO_STDOUT) log_path = "";
    td->Initialize(bclist_paths, log_path, true);

    std::set<FunctionPathPair> funcs;
    gctx->GetAllFunctions(funcs);
    for (auto &[F, path] : funcs) 
      _Update(F);
  }

  void UpdateModules(std::string hexsha, std::vector<std::string> up_paths,
      std::vector<ChangeInfo> cis, bool do_agg) {
    // Initialize analyzer context
    cur.Initialize(hexsha, cis);

    std::map<std::string, std::set<llvm::Function *>> funcs_map_pre; {
      for (auto p : up_paths) {
        auto &_funcs = funcs_map_pre[p];
        gctx->GetAllModuleFunctions(p, _funcs);
      }
    }

    // Get pre-commit change info.
    std::vector<llvm::Function *> pre_funcs; 
    std::map<std::string, std::vector<FunctionLinePair>> del_infos; {
      for (auto p : up_paths) {
        auto _ch_fs = _GetChangedFuncs(funcs_map_pre[p], cur.cis[p].del_lines);

        Logger::Log(0, "PATH:", p);
        Logger::Log(0, "CH_FUNCS:", _ch_fs);

        pre_funcs.insert(pre_funcs.end(), _ch_fs.begin(), _ch_fs.end());
        del_infos[p] = _GetRedefFuncs(funcs_map_pre[p], cur.cis[p].del_lines);
      }
    }

    // Aggregate old (pre-commit) relevance sets.
    if (do_agg) {
      for (auto &preF : pre_funcs) 
        _Aggregate(preF);
    }

    // Update the call graph & analyzers.
    td->UpdateModules(up_paths);

    std::map<std::string, std::set<llvm::Function *>> funcs_map_post; {
      for (auto p : up_paths) {
        auto &_funcs = funcs_map_post[p];
        gctx->GetAllModuleFunctions(p, _funcs);
      }
    }

    // Get post-commit change info.
    std::vector<llvm::Function *> post_funcs; 
    std::map<std::string, std::vector<FunctionLinePair>> add_infos; {
      for (auto p : up_paths) {
        auto _ch_fs = _GetChangedFuncs(funcs_map_post[p], cur.cis[p].add_lines);
        post_funcs.insert(post_funcs.end(), _ch_fs.begin(), _ch_fs.end());
        add_infos[p] = _GetRedefFuncs(funcs_map_post[p], cur.cis[p].add_lines);
      }
    }

    // NOTE: add_funcs := set of "(func *, line)"
    // Map old per-function analysis data to new functions.
    // Old pointers may be freed by now, so we'd better be careful.
    for (auto &[path, add_flps] : add_infos) {
      auto &del_flps = del_infos[path];
      _CorrectChanges(path, del_flps, add_flps);
    }

    for (auto &postF : post_funcs)
      _Update(postF);

    // Aggregate new (post-commit) relevance sets.
    if (do_agg) {
      for (auto &postF : post_funcs) 
        _Aggregate(postF);
    }

    // Print the aggregated relevance sets.
    if (do_agg) _Commit();
  }

  void Finalize() {
    td->Finalize();
  }
}
