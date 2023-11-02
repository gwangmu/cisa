#include <pybind11/iostream.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <cassert>
#include <string>
#include <vector>
#include <iostream>

#include <cisa/globalctx.h>
#include "analman.h"
  
GlobalContext globalctx;

void Initialize(std::vector<std::string> bclist_paths, 
    std::string out_path) {
  AnalyzerManager::Initialize(bclist_paths, out_path);
}

// up_paths: all header dependencies to source files, including deleted paths.
// up_funcs: list of "[<old_path>, <old_func>, <new_path>, <new_func>]".
void UpdateModules(std::string hexsha, std::vector<std::string> up_paths,
    std::vector<std::vector<int>> del_lines, 
    std::vector<std::vector<int>> add_lines, bool do_agg) {
  // Convert to ChangeInfo.
  std::vector<AnalyzerManager::ChangeInfo> cis; {
    assert(up_paths.size() == del_lines.size());
    assert(up_paths.size() == add_lines.size());
    for (int i = 0; i < up_paths.size(); i++) 
      cis.push_back(AnalyzerManager::ChangeInfo{
          up_paths[i], del_lines[i], add_lines[i]});
  }

  // Call inner method.
  AnalyzerManager::UpdateModules(hexsha, up_paths, cis, do_agg);
}

void Finalize() {
  AnalyzerManager::Finalize();
}

PYBIND11_MODULE(cisa, m) {
  m.def("Initialize", &Initialize,
      pybind11::call_guard<pybind11::scoped_ostream_redirect, 
        pybind11::scoped_estream_redirect>());
  m.def("UpdateModules", &UpdateModules,
      pybind11::call_guard<pybind11::scoped_ostream_redirect, 
        pybind11::scoped_estream_redirect>());
  m.def("Finalize", &Finalize,
      pybind11::call_guard<pybind11::scoped_ostream_redirect, 
        pybind11::scoped_estream_redirect>());
}

GlobalContext& CISA::GetGlobalContext() {
  return globalctx;
}
