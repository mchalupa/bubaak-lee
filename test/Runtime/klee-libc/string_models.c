// The models of the string functions: strncat, strstr, strdup, snprintf and
// the functions on wide strings. The writes are checked.
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --exit-on-error %t.bc 2>&1 | FileCheck %s
// RUN: %clang %s -DOVERFLOW=1 -emit-llvm %O0opt -g -c -o %t1.bc
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out --libc=klee --exit-on-error %t1.bc 2>&1 | FileCheck %s -check-prefix=CHECK-OVERFLOW
// RUN: %clang %s -DOVERFLOW=2 -emit-llvm %O0opt -g -c -o %t2.bc
// RUN: rm -rf %t.klee-out
// RUN: not %klee --output-dir=%t.klee-out --libc=klee --exit-on-error %t2.bc 2>&1 | FileCheck %s -check-prefix=CHECK-OVERFLOW2

// CHECK-NOT: ERROR
// CHECK: KLEE: done

#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

void overflow(void) {
#if OVERFLOW == 1
  char small[4];
  // CHECK-OVERFLOW: KLEE: ERROR: {{.*}}string_models.c:[[@LINE+1]]: memory error: out of bound pointer
  snprintf(small, 10, "%s", "abcdefgh");
#elif OVERFLOW == 2
  char small[4] = "ab";
  // CHECK-OVERFLOW2: KLEE: ERROR: {{.*}}string_models.c:[[@LINE+1]]: memory error: out of bound pointer
  strncat(small, "cdef", 3);
#endif
}

int main(void) {
  char buf[32];
  wchar_t wbuf[16];
  char *d;
  int n;

  strcpy(buf, "ab");
  strncat(buf, "cdef", 2);
  assert(strcmp(buf, "abcd") == 0);
  strncat(buf, "xy", 10);
  assert(strcmp(buf, "abcdxy") == 0);

  assert(strstr(buf, "cdx") == buf + 2);
  assert(strstr(buf, "cdy") == NULL);
  assert(strstr(buf, "") == buf);

  d = strdup(buf);
  assert(strcmp(d, buf) == 0);
  free(d);
  d = strndup(buf, 3);
  assert(strcmp(d, "abc") == 0);
  free(d);

  n = snprintf(buf, sizeof(buf), "%s-%d-%u-%x-%c-%5s|%-3d|%03d%%", "str", -42,
               7u, 255u, 'z', "ab", 1, 5);
  assert(n == (int)strlen(buf));
  assert(strcmp(buf, "str--42-7-ff-z-   ab|1  |005%") == 0);
  n = snprintf(buf, 4, "%ld", 123456L);
  assert(n == 6 && strcmp(buf, "123") == 0);
  n = sprintf(buf, "%lld:%zu", -9000000000LL, (size_t)12);
  assert(n == 14 && strcmp(buf, "-9000000000:12") == 0);
  n = snprintf(buf, sizeof(buf), "%#o|%#o|%.0d|%.3s|%5.1s|", 8u, 0u, 0, "abcdef", "xy");
  assert(n == (int)strlen(buf) && strcmp(buf, "010|0||abc|    x|") == 0);

  wcscpy(wbuf, L"ab");
  wcsncat(wbuf, L"cdef", 2);
  assert(wcslen(wbuf) == 4 && wcscmp(wbuf, L"abcd") == 0);
  wmemset(wbuf, L'x', 3);
  assert(wcsncmp(wbuf, L"xxxd", 4) == 0);
  assert(wcschr(wbuf, L'd') == wbuf + 3);

  assert(strspn("aabxc", "ab") == 3 && strcspn("aabxc", "xc") == 3);
  assert(strpbrk("aabxc", "cx") != NULL && *strpbrk("aabxc", "cx") == 'x');
  strcpy(buf, " a,b ,,c");
  assert(strcmp(strtok(buf, " ,"), "a") == 0);
  assert(strcmp(strtok(NULL, " ,"), "b") == 0);
  assert(strcmp(strtok(NULL, " ,"), "c") == 0);
  assert(strtok(NULL, " ,") == NULL);
  n = asprintf(&d, "%s=%d", "x", 42);
  assert(n == 4 && strcmp(d, "x=42") == 0);
  free(d);

  overflow();
  return 0;
}
