/* { dg-do compile } */
/* { dg-options "-march=rv32gc_zbb -mabi=ilp32 -fno-lto -O2" } */
/* { dg-skip-if "" { *-*-* } { "-g" } } */
/* { dg-final { check-function-bodies "**" "" } } */
<<<<<<< HEAD
/* { dg-final { scan-assembler-not {\mand} } } */
=======
/* { dg-final { scan-assembler-not "and" } } */
>>>>>>> 6da6ed95c9c (RISC-V: Remove masking third operand of rotate instructions)

/*
**foo1:
**	rori	a0,a0,20
**	ret
*/
unsigned int foo1(unsigned int rs1)
{
    return (rs1 >> 20) | (rs1 << 12);
}

/*
**foo2:
**	rori	a0,a0,22
**	ret
*/
unsigned int foo2(unsigned int rs1)
{
    return (rs1 << 10) | (rs1 >> 22);
}
