#include "common.h"

int (*moo)(int);
int zar(int);

int baz(int x) {
  return x + 20;
}

int bar(struct AAA *aaa) {
  aaa->fp = moo;
  return 1;
}

int foo(struct AAA *aaa) {
  moo = baz;
  bar(aaa);
  return 0;
}
