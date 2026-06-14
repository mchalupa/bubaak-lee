; Opaque-pointer version of ConstantExpr.ll for LLVM >= 16.  Starting with
; LLVM 16 (textual IR) the icmp/select/sext/zext/and/or/mul/shl/lshr/ashr
; constant expressions used by the legacy test are no longer accepted, so they
; are expressed as ordinary instructions here.  The surviving constant
; expressions (ptrtoint, inttoptr, getelementptr, add, sub, trunc, xor) are
; kept so that KLEE's constant-expression evaluation is still exercised.
; REQUIRES: geq-llvm-16.0
; RUN: %S/ConcreteTest.py --klee='%klee' --lli=%lli %s

; Most of the test below use the *address* of gInt as part of their computation,
; and then perform some operation (like x | ~x) which makes the result
; deterministic. They do, however, assume that the sign bit of the address as a
; 64-bit value will never be set.
@gInt = global i32 10
@gIntWithConstant = global i32 sub(i32 ptrtoint(ptr @gInt to i32),
                                 i32 ptrtoint(ptr @gInt to i32))

define void @"test_int_to_ptr"() {
  %t1 = add i8 ptrtoint(ptr inttoptr(i32 100 to ptr) to i8), 0
  %t2 = add i32 ptrtoint(ptr inttoptr(i8 100 to ptr) to i32), 0
  %t3 = add i32 ptrtoint(ptr inttoptr(i64 100 to ptr) to i32), 0
  %t4 = add i64 ptrtoint(ptr inttoptr(i32 100 to ptr) to i64), 0

  call void @print_i8(i8 %t1)
  call void @print_i32(i32 %t2)
  call void @print_i32(i32 %t3)
  call void @print_i64(i64 %t4)

  ret void
}

define void @"test_constant_ops"() {
  %t1 = add i8 trunc(i64 add(i64 ptrtoint(ptr @gInt to i64), i64 -10) to i8), 10

  %a32 = ptrtoint ptr @gInt to i32
  %sext64 = sext i32 %a32 to i64
  %sub2 = sub i64 %sext64, ptrtoint(ptr @gInt to i64)
  %t2 = and i64 %sub2, 4294967295

  %zext64 = zext i32 %a32 to i64
  %sub3 = sub i64 %zext64, ptrtoint(ptr @gInt to i64)
  %t3 = and i64 %sub3, 4294967295

  ; NOTE: lli's interpreter mis-evaluates a trunc constant expression used
  ; directly as an icmp operand, so hoist it into an instruction.
  %truncAddr = trunc i64 ptrtoint(ptr @gInt to i64) to i8
  %t4 = icmp eq i8 %truncAddr, %t1
  %t5 = zext i1 %t4 to i8

  call void @print_i8(i8 %t5)
  call void @print_i64(i64 %t2)
  call void @print_i64(i64 %t3)

  ret void
}

define void @"test_logical_ops"() {
  %a32 = ptrtoint ptr @gInt to i32
  %and1 = and i32 %a32, xor(i32 ptrtoint(ptr @gInt to i32), i32 -1)
  %t1 = add i32 -10, %and1
  %or2 = or i32 %a32, xor(i32 ptrtoint(ptr @gInt to i32), i32 -1)
  %t2 = add i32 -10, %or2
  %t3 = add i32 -10, xor(i32 xor(i32 ptrtoint(ptr @gInt to i32), i32 1024),  i32 ptrtoint(ptr @gInt to i32))

  call void @print_i32(i32 %t1)
  call void @print_i32(i32 %t2)
  call void @print_i32(i32 %t3)

  ; or the address with 1 to ensure the addresses will differ in 'ne' below
  %or = or i64 ptrtoint(ptr @gInt to i64), 1
  %lshr4 = lshr i64 %or, 8
  %t4 = shl i64 %lshr4, 8
  %ashr5 = ashr i64 %or, 8
  %t5 = shl i64 %ashr5, 8
  %shl6 = shl i64 %or, 8
  %t6 = lshr i64 %shl6, 8

  %t7 = icmp eq i64 %t4, %t5
  %t8 = icmp ne i64 %t4, %t6

  %t9 = zext i1 %t7 to i8
  %t10 = zext i1 %t8 to i8

  call void @print_i8(i8 %t9)
  call void @print_i8(i8 %t10)

  ret void
}

%test.struct.type = type { i32, i32 }
@test_struct = global %test.struct.type { i32 0, i32 10 }

define void @"test_misc"() {
  ; probability that @gInt == 100 is very very low
  %cmp = icmp eq ptr @gInt, inttoptr(i32 100 to ptr)
  %sel = select i1 %cmp, i32 10, i32 0
  %t1 = add i32 %sel, 0
  call void @print_i32(i32 %t1)

  %t2 = load i32, ptr getelementptr(%test.struct.type, ptr @test_struct, i32 0, i32 1)
  call void @print_i32(i32 %t2)

  ret void
}

define void @"test_simple_arith"() {
  %t1 = add i32 add(i32 ptrtoint(ptr @gInt to i32), i32 0), 0
  %t2 = add i32 sub(i32 0, i32 ptrtoint(ptr @gInt to i32)), %t1
  %innermul = mul i32 ptrtoint(ptr @gInt to i32), 10
  %t3 = mul i32 %innermul, %t2

  call void @print_i32(i32 %t3)

  ret void
}

define void @test_cmp() {
  %addr = ptrtoint ptr @gInt to i64
  %c1 = icmp ult i64 %addr, 0
  %z1 = zext i1 %c1 to i8
  %t1 = add i8 %z1, 1
  %c2 = icmp ule i64 %addr, 0
  %z2 = zext i1 %c2 to i8
  %t2 = add i8 %z2, 1
  %c3 = icmp uge i64 %addr, 0
  %z3 = zext i1 %c3 to i8
  %t3 = add i8 %z3, 1
  %c4 = icmp ugt i64 %addr, 0
  %z4 = zext i1 %c4 to i8
  %t4 = add i8 %z4, 1
  %c5 = icmp slt i64 %addr, 0
  %z5 = zext i1 %c5 to i8
  %t5 = add i8 %z5, 1
  %c6 = icmp sle i64 %addr, 0
  %z6 = zext i1 %c6 to i8
  %t6 = add i8 %z6, 1
  %c7 = icmp sge i64 %addr, 0
  %z7 = zext i1 %c7 to i8
  %t7 = add i8 %z7, 1
  %c8 = icmp sgt i64 %addr, 0
  %z8 = zext i1 %c8 to i8
  %t8 = add i8 %z8, 1
  %c9 = icmp eq i64 %addr, 10
  %z9 = zext i1 %c9 to i8
  %t9 = add i8 %z9, 1
  %c10 = icmp ne i64 %addr, 10
  %z10 = zext i1 %c10 to i8
  %t10 = add i8 %z10, 1

  call void @print_i1(i8 %t1)
  call void @print_i1(i8 %t2)
  call void @print_i1(i8 %t3)
  call void @print_i1(i8 %t4)
  call void @print_i1(i8 %t5)
  call void @print_i1(i8 %t6)
  call void @print_i1(i8 %t7)
  call void @print_i1(i8 %t8)
  call void @print_i1(i8 %t9)
  call void @print_i1(i8 %t10)

  ret void
}

define i32 @main() {
    call void @test_simple_arith()

    call void @test_cmp()

    call void @test_int_to_ptr()

    call void @test_constant_ops()

    call void @test_logical_ops()

    call void @test_misc()

    ret i32 0
}

; defined in print_int.c
declare void @print_i1(i8)
declare void @print_i8(i8)
declare void @print_i16(i16)
declare void @print_i32(i32)
declare void @print_i64(i64)
