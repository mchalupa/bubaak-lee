; REQUIRES: x86_64

; RUN: %llvmas %s -f -o %t1.bc
; RUN: rm -rf %t.klee-out
; RUN: %klee --output-dir=%t.klee-out --optimize=false %t1.bc
; RUN: FileCheck %s --input-file=%t.klee-out/assembly.ll

define i32 @asm_free() nounwind {
entry:
  call void asm sideeffect "", "~{memory},~{dirflag},~{fpsr},~{flags}"()
  ; Make sure simple memory barrier is lifted
  ; CHECK-NOT: call void asm sideeffect "", "~{memory},~{dirflag},~{fpsr},~{flags}"()
  ret i32 0
}

define i8* @lifted_tied_asm() nounwind {
entry:
  %0 = alloca [47 x i8], align 16
  %1 = getelementptr inbounds [47 x i8], [47 x i8]* %0, i64 0, i64 0
  ; Make sure memory barrier with an output tied to an input is lifted, the
  ; output is the input
  %2 = call i8* asm sideeffect "", "=r,0,~{memory},~{dirflag},~{fpsr},~{flags}"(i8* nonnull %1)
  ; CHECK-LABEL: @lifted_tied_asm
  ; CHECK-NOT: asm sideeffect
  ; CHECK: ret ptr %1
  ret i8* %2
}

define i32 @unlifted_asm() nounwind {
entry:
  ; Make sure memory barrier with an output that is not tied is kept
  %0 = call i32 asm sideeffect "", "=r,~{memory},~{dirflag},~{fpsr},~{flags}"()
  ; CHECK-LABEL: @unlifted_asm
  ; CHECK: %0 = call i32 asm sideeffect "", "=r,~{memory},~{dirflag},~{fpsr},~{flags}"()
  ret i32 %0
}

define i32 @main() nounwind {
entry:
  ret i32 0
}
