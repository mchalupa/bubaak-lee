// Every call of the allocating functions from the program takes exactly one
// decision __bbk_alloc_fail (also strdup and strndup of the libc, which call
// malloc); the allocations inside the other libc functions take none.
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out && %klee --output-dir=%t.klee-out --libc=klee --alloc-may-fail --search=dfs %t.bc > %t.log 2> %t.err
// RUN: not grep -q "KLEE: ERROR" %t.err
// (one decision per call: 5 before the malloc, the malloc, 1 after it either
// way (posix_memalign with a bad alignment takes one too, the overflowing
// reallocarray none): 2^5 * (2 + 2) paths, the tests have 7 decisions)
// RUN: grep -q "completed paths = 128" %t.err
// RUN: %ktest-tool %t.klee-out/*.ktest | grep -c "num objects: 7$" | grep -qx 128
// RUN: %ktest-tool %t.klee-out/*.ktest | grep -c "name: '__bbk_alloc_fail'$" | grep -qx 896
// RUN: grep -q "^posix_memalign: ENOMEM, unchanged$" %t.log
// RUN: grep -q "^posix_memalign: 0, aligned$" %t.log
// RUN: grep -q "^bad alignment: EINVAL$" %t.log
// RUN: grep -q "^reallocarray overflow: NULL, kept$" %t.log
// RUN: not grep -q "^asprintf: 1$" %t.log
// RUN: grep -q "strdup: NULL" %t.log
// RUN: grep -q "strdup: abc" %t.log
// RUN: grep -q "strndup: ab" %t.log
// RUN: grep -q "valloc: aligned" %t.log
// RUN: grep -q "reallocarray: 7" %t.log
// RUN: grep -q "reallocarray: NULL, kept 7" %t.log
//
// Without -alloc-may-fail, nothing fails (one path)
// RUN: rm -rf %t.klee-out && %klee --output-dir=%t.klee-out --libc=klee %t.bc > %t.log 2> %t.err
// RUN: grep -q "completed paths = 1" %t.err
// RUN: not grep -q ": NULL$\|ENOMEM\|NULL, kept 7" %t.log
// RUN: grep -q "^reallocarray overflow: NULL, kept$" %t.log
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *valloc(size_t size);
void *reallocarray(void *ptr, size_t nmemb, size_t size);

int main(void) {
  char *s = strdup("abc");                           // 1
  printf("strdup: %s\n", s ? s : "NULL");
  char *t = strndup("abc", 2);                       // 2
  printf("strndup: %s\n", t ? t : "NULL");
  void *v = valloc(10);                              // 3
  printf("valloc: %s\n", !v ? "NULL" : ((uintptr_t)v % 4096 == 0 ? "aligned" : "unaligned"));
  void *m = (void *)&m;
  int r = posix_memalign(&m, 64, 10);                // 4
  if (r == ENOMEM)
    printf("posix_memalign: ENOMEM, %s\n", m == (void *)&m ? "unchanged" : "changed");
  else if (r == 0)
    printf("posix_memalign: 0, %s\n", (uintptr_t)m % 64 == 0 ? "aligned" : "unaligned");
  void *b = (void *)&b;
  r = posix_memalign(&b, 3, 10);                     // 5
  if (r == EINVAL && b == (void *)&b)
    printf("bad alignment: EINVAL\n");
  else if (r != ENOMEM || b != (void *)&b)
    return 1;
  int *a = malloc(sizeof(int));                      // 6
  if (a) {
    *a = 7;
    int *o = reallocarray(a, SIZE_MAX / 2, 4);       // 7
    if (!o && *a == 7)
      printf("reallocarray overflow: NULL, kept\n");
    int *n = reallocarray(a, 2, sizeof(int));        // 8
    if (n) {
      printf("reallocarray: %d\n", n[0]);
      a = n;
    } else {
      printf("reallocarray: NULL, kept %d\n", a[0]);
    }
  } else {
    // (the same number of decisions on both sides)
    printf("reallocarray overflow: NULL, kept\n");
    free(malloc(1));                                 // 7
  }
  // the allocation inside asprintf takes no decision
  char *str;
  printf("asprintf: %d\n", asprintf(&str, "%d", 0) == 1 ? 0 : 1);
  free(str);
  free(a);
  free(m == (void *)&m ? NULL : m);
  free(v);
  free(t);
  free(s);
  return 0;
}
