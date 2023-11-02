#pragma once

typedef int (*fptr_t)(int);
struct AAA {
  fptr_t fp;
};

int foo(struct AAA *aaa);
int zoo(int x);
