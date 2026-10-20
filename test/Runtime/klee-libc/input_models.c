// The models of the functions that read the environment or the input of the
// program explore only real behaviors and report that they are
// under-approximations.
// RUN: %clang %s -emit-llvm %O0opt -g -c -o %t.bc
// RUN: rm -rf %t.klee-out
// RUN: %klee --output-dir=%t.klee-out --libc=klee --exit-on-error %t.bc > %t.out 2>&1
// RUN: FileCheck %s -input-file=%t.out
// RUN: grep "KLEE: done: completed paths" %t.out
// RUN: not grep "ERROR" %t.out

// CHECK-DAG: KLEE: WARNING: under-approximation: getenv
// CHECK-DAG: KLEE: WARNING: under-approximation: fgets
// CHECK-DAG: KLEE: WARNING: under-approximation: fgetc
// CHECK-DAG: KLEE: WARNING: under-approximation: recv
// CHECK-DAG: KLEE: WARNING: under-approximation: fread
// CHECK-DAG: KLEE: WARNING: under-approximation: fscanf
// CHECK-DAG: getenv: null
// CHECK-DAG: rand: big
// CHECK-DAG: fgets: eof
// CHECK-DAG: fgets: newline
// CHECK-DAG: fgets: full
// CHECK-DAG: fgets: partial
// CHECK-DAG: fgetc: eof
// CHECK-DAG: fgetc: 255
// CHECK-DAG: recv: closed
// CHECK-DAG: recv: full
// CHECK-DAG: recv: data
// CHECK-DAG: fread: all
// CHECK-DAG: fread: eof
// CHECK-DAG: fscanf: eof
// CHECK-DAG: fscanf: none
// CHECK-DAG: fscanf: two

#include "klee/klee.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

int main(void) {
  char buf[10];
  char *e;
  int r, c, x, y;
  ssize_t n;
  size_t len;

  /* one function on each path (to avoid the explosion of the paths) */
  switch (klee_range(0, 7, "which")) {
  case 0:
    e = getenv("HOME");
    assert(!e);
    printf("getenv: null\n");
    break;
  case 1:
    r = rand();
    assert(r >= 0);
    if (r > 1000)
      printf("rand: big\n");
    break;
  case 2:
    memset(buf, 'x', sizeof(buf));
    if (!fgets(buf, 5, stdin)) {
      assert(buf[0] == 'x'); /* not changed */
      assert(!fgets(buf, 5, stdin)); /* the end is sticky */
      printf("fgets: eof\n");
      break;
    }
    assert(buf[5] == 'x');
    len = strlen(buf);
    assert(len <= 4);
    if (buf[0] == 'a' && buf[1] == '\n') {
      assert(buf[2] == '\0');
      printf("fgets: newline\n");
    } else if (len == 4 && buf[0] == '\n') {
      assert(0 && "newline in the middle");
    } else if (len == 4) {
      printf("fgets: full\n");
    } else if (len == 2 && buf[1] == 'b' && buf[3] == 'x' && buf[4] == 'x') {
      /* "?b" at the end of the input */
      assert(!fgets(buf, 5, stdin));
      printf("fgets: partial\n");
    }
    break;
  case 3:
    c = fgetc(stdin);
    assert(c >= -1 && c <= 255);
    if (c == EOF) {
      assert(fgetc(stdin) == EOF);
      printf("fgetc: eof\n");
    } else if (c == 255)
      printf("fgetc: 255\n");
    break;
  case 4:
    memset(buf, 0, sizeof(buf));
    n = recv(3, buf, 4, 0);
    assert(n >= 0 && n <= 4);
    assert(buf[4] == 0);
    if (n < 4)
      assert(buf[n] == 0); /* not written */
    if (n == 0) {
      assert(recv(3, buf, 4, 0) == 0);
      printf("recv: closed\n");
    } else if (n == 4)
      printf("recv: full\n");
    else if (n == 2 && buf[0] == 'x')
      printf("recv: data\n");
    break;
  case 5:
    if (fread(buf, 2, 3, stdin) == 3)
      printf("fread: all\n");
    else
      printf("fread: eof\n");
    break;
  case 6:
    x = y = 7;
    r = fscanf(stdin, "%d %d", &x, &y);
    if (r == EOF)
      printf("fscanf: eof\n");
    else if (r == 0)
      assert(x == 7 && y == 7), printf("fscanf: none\n");
    else if (r == 2 && x == -5 && y == 1000)
      printf("fscanf: two\n");
    break;
  }
  return 0;
}
