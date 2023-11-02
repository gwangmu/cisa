#pragma once

#include <string>
#include <vector>
#include <llvm/IR/Function.h>

namespace AnalyzerManager {
  struct ChangeInfo {
    std::string bc_path;
    std::vector<int> del_lines;
    std::vector<int> add_lines;
  };

  void Initialize(std::vector<std::string> bclist_paths, std::string out_path);
  void UpdateModules(std::string hexsha, std::vector<std::string> up_paths,
    std::vector<ChangeInfo> cis, bool do_agg);
  void Finalize();
}
