#pragma once 

#include <map>
#include <set>
#include <string>
#include <llvm/IR/Function.h>

class RelSet: public std::set<std::string> {
public:
  using set::set;
  static const RelSet All;
  static const RelSet None;
};

typedef std::map<std::string, RelSet> AnalyzerReturn; 

class IAnalyzer {
public:
  // (Short) name of this analyzer.
  virtual const std::string& Name() = 0;

  // Move "PreF" tags to "PostF".
  virtual void Move(llvm::Function *PreF, llvm::Function *PostF) = 0;

  // Remove tags attached to "F".
  virtual void Remove(llvm::Function *F) = 0;

  // Update tags for this function.
  virtual void Update(llvm::Function *F) = 0;

  // Aggregate tags and return relevance sets.
  // NOTE: "(all)" selects all entities.
  virtual AnalyzerReturn Aggregate(llvm::Function *F) = 0;
};

template <typename T, int N = 1>
class Analyzer: public IAnalyzer {
private:
  const std::string name;
  const std::set<T> empty_tags;
  std::map<llvm::Function *, std::set<T>> tags[N];

protected:
  const std::set<T>& GetTags(llvm::Function *F, int idx = 0);
  void AddTag(T tag, llvm::Function *F, int idx = 0);

public:
  Analyzer(std::string name): name(name) {}
  virtual const std::string& Name() { return name; };
  virtual void Move(llvm::Function *PreF, llvm::Function *PostF);
  virtual void Remove(llvm::Function *F);
};

template <typename T, int N>
const std::set<T>& Analyzer<T, N>::GetTags(llvm::Function *F, int idx) {
  if (idx >= N || idx < 0 || tags[idx].find(F) == tags[idx].end())
    return empty_tags;
  else if (tags[idx].find(F) != tags[idx].end())
    return tags[idx][F];
  return empty_tags;
}

template <typename T, int N>
void Analyzer<T, N>::AddTag(T tag, llvm::Function *F, int idx) {
  if (idx >= N || idx < 0) return;
  tags[idx][F].insert(tag);
}

//#include <iostream>

template <typename T, int N>
void Analyzer<T, N>::Move(llvm::Function *PreF, llvm::Function *PostF) {
//std::cout << "MOVING: [";
//for (auto t : tags[PreF])
//  std::cout << t << ", ";
//std::cout << "]\n";
  for (int i = 0; i < N; i++) {
    tags[i][PostF] = tags[i][PreF];
    tags[i].erase(PreF);
  }
//std::cout << "MOVED: [";
//for (auto t : tags[PostF])
//  std::cout << t << ", ";
//std::cout << "]\n";
}

template <typename T, int N>
void Analyzer<T, N>::Remove(llvm::Function *F) {
  for (int i = 0; i < N; i++) 
    tags[i].erase(F);
}

namespace AnalyzerManager {
  extern std::vector<IAnalyzer *> analyzers;
}

#define REGISTER_ANALYZER(x) \
  namespace CISA { \
    using namespace AnalyzerManager; \
    x* x##_inst = new x(); \
    int x##_create = ([](){ analyzers.push_back(x##_inst); }(), 0); \
    template <typename x> x* GetAnalysis() { return x##_inst; } \
  }

#define REGISTER_ANALYZER_PRIORITY(x) // TODO
