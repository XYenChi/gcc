/* { dg-do compile } */
/* { dg-options "-march=rv32gc_zbb -mabi=ilp32" } */
/* { dg-skip-if "" { *-*-* } { "-g" } } */

unsigned int foo1(unsigned int rs1, unsigned int rs2)
{
    unsigned int shamt = rs2 & (32 - 1);
    return (rs1 << shamt) | (rs1 >> ((32 - shamt) & (32 - 1)));
}
unsigned int foo2(unsigned int rs1, unsigned int rs2)
{
    unsigned int shamt = rs2 & (32 - 1);
    return (rs1 >> shamt) | (rs1 << ((32 - shamt) & (32 - 1)));
}

<<<<<<< HEAD
/* { dg-final { scan-assembler-times {\mrol} 2 } } */
/* { dg-final { scan-assembler-times {\mror} 2 } } */
/* { dg-final { scan-assembler-not {and} { target { no-opts "-O0" } } } } */
=======
/* { dg-final { scan-assembler-times "rol" 2 } } */
/* { dg-final { scan-assembler-times "ror" 2 } } */
/* { dg-final { scan-assembler-not {and} { target { no-opts "-O0" } } } } */
>>>>>>> 6da6ed95c9c (RISC-V: Remove masking third operand of rotate instructions)
