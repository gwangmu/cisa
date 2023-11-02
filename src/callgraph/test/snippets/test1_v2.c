#include "common.h"

int (*moo)(int);
int zar(int);

int baz(int x) {
  return x + 20;
}

int ads(int x) {
  return x + 50;
}

int bar(struct AAA *aaa) {
  aaa->fp = moo;
  return 1;
}

int foo(struct AAA *aaa) {
  moo = zar;
  bar(aaa);
  return 0;
}
