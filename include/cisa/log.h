#pragma once

#include <cstdint>
#include <iostream>
#include <string>
#include <sstream>

namespace Logger {
  // level 0: absolutly spit out everything.
  // level 1+: abstrain from printing logs protortional to the level.
  static const int LOG_LEVEL = 0;

  template <typename E1, typename E2>
  std::string ToString(std::pair<E1, E2> &p);

  template <typename E>
  std::string ToString(std::vector<E> &v); 

  template <typename E>
  std::string ToString(std::set<E> &v); 

  template <typename E1, typename E2>
  std::string ToString(std::map<E1, E2> &v); 

  template <typename T, typename... Args>
  std::string ToString(T t, Args... args); 

  template <typename T>
  std::string ToString(T t); 

  std::string ToString(const char *s); 
  std::string ToString(std::string s); 
  std::string ToString(void *s); 
  std::string ToString(); 

  inline std::string ToString(llvm::Function *F) {
    return " " + F->getName().str();
  }

  template <typename E1, typename E2>
  std::string ToString(std::pair<E1, E2> &p) {
    std::string ret = " (" + ToString(p.first).substr(1) +
      " -> " + ToString(p.second).substr(1) + ")";
    return ret;
  }

  template <typename E>
  std::string ToString(std::vector<E> &v) {
    std::string ret = " [";
    int idx = 0;
    for (auto const &t : v) {
      if (++idx < v.size())
        ret += ToString(t).substr(1) + ", ";
      else
        ret += ToString(t).substr(1);
    }
    ret += "]";
    return ret;
  }

  template <typename E>
  std::string ToString(std::set<E> &v) {
    std::string ret = " [";
    int idx = 0;
    for (auto const &t : v) {
      if (++idx < v.size())
        ret += ToString(t).substr(1) + ", ";
      else
        ret += ToString(t).substr(1);
    }
    ret += "]";
    return ret;
  }

  template <typename E1, typename E2>
  std::string ToString(std::map<E1, E2> &v) {
    std::string ret = " {";
    int idx = 0;
    for (auto const &t : v) {
      if (++idx < v.size())
        ret += ToString(t).substr(1) + ", ";
      else
        ret += ToString(t).substr(1);
    }
    ret += "}";
    return ret;
  }

  inline std::string ToString(const char *s) {
    std::string ret = " " + std::string(s);
    return ret;
  }

  inline std::string ToString(std::string s) {
    std::string ret = " " + s;
    return ret;
  }

  inline std::string ToString(uintptr_t s) {
    std::stringstream stream;
    stream << std::hex << (uintptr_t)s;
    std::string result(stream.str());
    return " 0x" + result;
  } 

  template <typename T>
  std::string ToString(T t) {
    return " " + std::to_string(t);
  }

  inline std::string ToString() {
    return " ";
  }

  template <typename T, typename... Args>
  std::string ToString(T t, Args... args) {
    std::string ret = ToString(t);
    ret += ToString(args...);
    return ret;
  }

  template <typename... Args>
  void Log(int lv, Args... args) {
    if (lv < LOG_LEVEL) return;
    std::string ret = ToString(args...).substr(1);
    std::cout << ret << "\n";
  }
}
