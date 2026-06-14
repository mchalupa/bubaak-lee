; Opaque-pointer version of GlobalVariable.ll for LLVM >= 16, where typed
; pointers and the icmp/bitcast constant expressions used by the legacy test
; are no longer available in textual IR.
; REQUIRES: geq-llvm-16.0
; RUN: %llvmas %s -f -o %t1.bc
; RUN: rm -rf %t.klee-out
; Run KLEE — the fork makes the external global symbolic and catches the
; invalid function pointer call gracefully (no crash).
; RUN: %klee --output-dir=%t.klee-out --optimize=false %t1.bc 2>&1 | FileCheck %s
; CHECK: memory error: invalid function pointer

@external_function = extern_weak global i8
@bar = internal thread_local global <{ [56 x i8] }> zeroinitializer, align 32
@handle = global ptr null, align 8

define internal void @foo(ptr nocapture) {
entry:
    ret void
}

define i32 @main(i32 %argc, ptr nocapture %argv) nounwind readnone {
entry:
  %cmp = icmp ne ptr @external_function, null
  br i1 %cmp, label %bbtrue, label %bbfalse

bbtrue:
  %0 = tail call i32 @external_function(ptr nonnull @foo, ptr getelementptr inbounds (<{ [56 x i8] }>, ptr @bar, i64 0, i32 0, i64 0), ptr @handle)
  ret i32 0

bbfalse:
  ret i32 1
}
