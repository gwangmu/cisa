#include <TypeDive.h>
#include <string>
#include <vector>
#include <iostream>
#include <cstdlib>
#include <unistd.h>

std::string GetPrefix() {
  char buf[512];
  readlink("/proc/self/exe", buf, 512-1);
  auto path = std::string(buf);
  return path.substr(0, path.rfind('/'));
}

void AttachPrefix(std::vector<std::string> &vec, std::string prefix) {
  for (auto &s : vec)
    s = prefix + "/" + s;
}

void ReplaceSource(std::string prefix, std::string target, std::string to) {
  std::string cmd = "cp ";
  cmd += prefix + "/";
  cmd += to + " ";
  cmd += prefix + "/";
  cmd += target;
  system(cmd.c_str());
}

int main() {
  std::vector<std::string> vecV1BCs{
    "test2.bc",
    "main.bc",
    "test1.bc"
  };

  std::vector<std::string> vecV2BCs{
    "test1.bc"
  };

  std::string prefix = GetPrefix();

  AttachPrefix(vecV1BCs, prefix);
  AttachPrefix(vecV2BCs, prefix);

  ReplaceSource(prefix, "test1.bc", "test1_v1.bc");
  TypeDive::Initialize(vecV1BCs);
  TypeDive::Dump();

  std::cout << "-------\n";

  ReplaceSource(prefix, "test1.bc", "test1_v2.bc");
  TypeDive::UpdateModules(vecV2BCs);
  TypeDive::Dump();

  return 0;
}
