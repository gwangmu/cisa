#include "common.h"

int zar(int x) {
  return x + 100;
}

int zoo(int x) {
  zar(x);
  return x * 10;
}
