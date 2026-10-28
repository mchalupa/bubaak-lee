; REQUIRES: geq-llvm-16.0
; RUN: %llvmas %s -o=%t.bc
; RUN: rm -rf %t.klee-out
; RUN: %klee -exit-on-error --output-dir=%t.klee-out --optimize=false %t.bc 2>&1 | FileCheck %s
; CHECK-NOT: unimplemented intrinsic
; CHECK: KLEE: done: completed paths = 1

@tls = thread_local global i32 1

declare ptr @llvm.threadlocal.address.p0(ptr)
declare void @abort()

define i32 @main() {
  %p = call ptr @llvm.threadlocal.address.p0(ptr @tls)
  store i32 2, ptr %p
  %q = call ptr @llvm.threadlocal.address.p0(ptr @tls)
  %v = load i32, ptr %q
  %ok = icmp eq i32 %v, 2
  br i1 %ok, label %exit, label %fail

fail:
  call void @abort()
  unreachable

exit:
  ret i32 0
}
