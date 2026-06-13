; RUN: %llvmas %s -f -o %t1.bc
; RUN: rm -rf %t.klee-out
; Run KLEE — the fork makes the external global symbolic and catches the
; invalid function pointer call gracefully (no crash).
; RUN: %klee --output-dir=%t.klee-out --optimize=false %t1.bc 2>&1 | FileCheck %s
; CHECK: memory error: invalid function pointer

@external_function = extern_weak global i8
@bar = internal thread_local global <{ [56 x i8] }> zeroinitializer, align 32
@handle = global i8* null, align 8

define internal void @foo(i8* nocapture) {
entry:
    ret void
}

define i32 @main(i32 %argc, i8** nocapture %argv) nounwind readnone {
entry:
  br i1 icmp ne (i8* @external_function, i8* null), label %bbtrue, label %bbfalse

bbtrue:
  %0 = tail call i32 bitcast (i8* @external_function to i32 (void (i8*)*, i8*, i8*)*)(void (i8*)* nonnull @foo, i8* getelementptr inbounds (<{ [56 x i8] }>, <{ [56 x i8] }>* @bar, i64 0, i32 0, i64 0), i8* bitcast (i8** @handle to i8*))
  ret i32 0

bbfalse:
  ret i32 1
}
