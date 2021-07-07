/*
  puce65c02, a WDC 65c02 cpu emulator, based on puce6502 by the same author
  Last modified 1st of July 2021

  Copyright (c) 2021 Arthur Ferreira (arthur.ferreira2@gmail.com)

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions :

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#include "puce65c02.h"


#if __TESTS__

  // functionnal and timing tests - see also bottom of file

  // Theses tests require 65C02_extended_opcodes_test.bin and 6502_functional_test.bin
  // from https://github.com/Klaus2m5/6502_65C02_functional_tests
  // and timingtest-1.bin from http://forum.6502.org/viewtopic.php?f=8&t=3340

  // compile and run :

  //    g++ -Wall -O3 -D __TESTS__ puce65c02.cpp -o puce65c02 && ./puce65c02
  // or, slower but with detailled info :
  //     g++ -Wall -O3 -D __TESTS__ -D VERBOSE puce65c02.cpp -o puce65c02 && ./puce65c02
  // or, if you just want to check the cycle accuracy :
  //     g++ -Wall -O3 -D __TESTS__ -D TIMING puce65c02.cpp -o puce65c02 && ./puce65c02

  uint8_t RAM[65536];
  inline uint8_t readMem(uint16_t address) { return RAM[address]; }
  inline void writeMem(uint16_t address, uint8_t value8) { RAM[address] = value8; }

#else
  // user is supposed to provide these two functions
  extern uint8_t readMem(uint16_t address);
  extern void writeMem(uint16_t address, uint8_t value8);

#endif


void puce65c02::RST() {
  PC = readMem(0xFFFC) | (readMem(0xFFFD) << 8);
  SP = 0xFF;
  P.I = 1;
  P.U = 1;
  state = run;
  ticks += 7;
}


void puce65c02::IRQ() {
  state = run;                    // always ??
  if (!P.I) return;                // consume clock cycles ?
  PC++;
  writeMem(0x100 + SP, (PC >> 8) & 0xFF);
  SP--;
  writeMem(0x100 + SP, PC & 0xFF);
  SP--;
  writeMem(0x100 + SP, P.byte & ~BREAK);
  SP--;
  PC = readMem(0xFFFE) | (readMem(0xFFFF) << 8);
  ticks += 7;
}


void puce65c02::NMI() {
  state = run;
  P.I = 1;  // ???
  PC++;
  writeMem(0x100 + SP, (PC >> 8) & 0xFF);
  SP--;
  writeMem(0x100 + SP, PC & 0xFF);
  SP--;
  writeMem(0x100 + SP, P.byte & ~BREAK);
  SP--;
  PC = readMem(0xFFFA) | (readMem(0xFFFB) << 8);
  ticks += 7;
}


/*
  Addressing modes abreviations used in the comments below :

  IMP  : Implied or Implicit : DEX, RTS, CLC - 61 instructions
  ACC  : Accumulator : ASL A, ROR A, DEC A - 6 instructions
  IMM  : Immediate : LDA #$A5 - 19 instructions
  ZPG  : Zero Page : LDA $81 - 41 instructions
  ZPX  : Zero Page Indexed with X : LDA $55,X - 21 instructions
  ZPY  : Zero Page Indexed with Y : LDX $55,Y - 2 instructions
  REL  : Relative : BEQ LABEL12 - 9 instructions
  ABS  : Absolute : LDA $2000 - 29 instructions
  ABX  : Absolute Indexed with X : LDA $2000,X - 17 instructions
  ABY  : Absolute Indexed with Y : LDA $2000,Y - 9 instructions
  IND  : Indirect : JMP ($1020) - 1 instruction
  IZP  : Indirect Zero Page : LDA ($55) (65c02 only) - 8 instructions
  IZX  : ZP Indexed Indirect with X (Preindexed) : LDA ($55,X) - 8 instructions
  IZY  : ZP Indirect Indexed with Y (Postindexed) : LDA ($55),Y - 8 instructions
  IAX  : Absolute Indexed Indirect : JMP ($2000,X) (65c02 only) - 1 instruction
  ZPR  : Zero Page Relative : BBS0 $23, LABEL (65c02 only) - 16 instructions
*/


uint16_t puce65c02::exec(unsigned long long int cycleCount) {

  cycleCount += ticks;  // cycleCount becomes the targeted ticks value8
  while (ticks < cycleCount) {
    if (state == run || state == step) {

      uint8_t value8;
      uint16_t value16;
      uint16_t address;

      switch(readMem(PC++)) {  // fetch instruction and increment Program Counter

        case 0x00 :  // IMP BRK
          PC++;
          writeMem(0x100 + SP, ((PC) >> 8) & 0xFF);
          SP--;
          writeMem(0x100 + SP, PC & 0xFF);
          SP--;
          writeMem(0x100 + SP, P.byte | BREAK);
          SP--;
          P.I = 1;
          P.D = 0;
          PC = readMem(0xFFFE) | (readMem(0xFFFF) << 8);
          ticks += 7;
        break;

        case 0x01 :  // IZX ORA
          value8 = readMem(PC) + X;
          PC++;
          address = readMem(value8);
          value8++;
          address |= readMem(value8) << 8;
          A |= readMem(address);
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 6;
        break;

        case 0x02 :  // IMM NOP
          PC++;
          ticks += 2;
        break;

        case 0x03 :  // IMP NOP
          ticks++;
        break;

        case 0x04 :  // ZPG TSB
          address = readMem(PC);
          PC++;
          value8 = readMem(address);
          P.Z = (value8 & A) == 0;
          writeMem(address, value8 | A);
          ticks += 5;
        break;

        case 0x05 :  // ZPG ORA
          A |= readMem(readMem(PC));
          PC++;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 3;
        break;

        case 0x06 :  // ZPG ASL
          address = readMem(PC);
          PC++;
          value16 = readMem(address) << 1;
          P.C = value16 > 0xFF;
          value16 &= 0xFF;
          writeMem(address, value16);
          P.Z = value16 == 0;
          P.S = value16 > 0x7F;
          ticks += 5;
        break;

        case 0x07 :  // ZPG RMB
          address = readMem(PC);
          PC++;
          writeMem(address, readMem(address) & ~1);
          ticks += 5;
        break;

        case 0x08 :  // IMP PHP
          writeMem(0x100 + SP, P.byte | BREAK);
          SP--;
          ticks += 3;
        break;

        case 0x09 :  // IMM ORA
          A |= readMem(PC);
          PC++;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 2;
        break;

        case 0x0A :  // ACC ASL
          value16 = A << 1;
          A = value16 & 0xFF;
          P.C = value16 > 0xFF;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 2;
        break;

        case 0x0B :  // IMP NOP
          ticks++;
        break;

        case 0x0C :  // ABS TSB
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          PC++;
          value8 = readMem(address);
          P.Z = (value8 & A) == 0;
          writeMem(address, value8 | A);
          ticks += 6;
        break;

        case 0x0D :  // ABS ORA
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          PC++;
          A |= readMem(address);
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 4;
        break;

        case 0x0E :  // ABS ASL
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          PC++;
          value16 = readMem(address) << 1;
          P.C = value16 > 0xFF;
          value16 &= 0xFF;
          writeMem(address, value16);
          P.Z = value16 == 0;
          P.S = value16 > 0x7F;
          ticks += 6;
        break;

        case 0x0F :  // ZPR BBR
          value8 = readMem(readMem(PC));
          PC++;
          address = readMem(PC);
          PC++;
          if (address & SIGN)
            address |= 0xFF00;  // jump backward
          if (!(value8 & 1))
            PC += address;
          ticks += 5;
        break;

        case 0x10 :  // REL BPL
          address = readMem(PC);
          PC++;
          if (!P.S) {  // jump taken
            ticks++;
            if (address & SIGN)
              address |= 0xFF00;  // jump backward
            if (((PC & 0xFF) + address) & 0xFF00)  // page crossing
              ticks++;
            PC += address;
          }
          ticks += 2;
        break;

        case 0x11 :  // IZY ORA
          value8 = readMem(PC);
          PC++;
          address = readMem(value8);
          value8++;
          address |= readMem(value8) << 8;
          ticks += (((address & 0xFF) + Y) & 0xFF00) ? 6 : 5;  // page crossing
          address += Y;
          A |= readMem(address);
          P.Z = A == 0;
          P.S = A > 0x7F;
        break;

        case 0x12 :  // IZP ORA
          value8 = readMem(PC);
          PC++;
          address = readMem(value8);
          value8++;
          address |= readMem(value8) << 8;
          A |= readMem(address);
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 5;
        break;

        case 0x13 :  // IMP NOP
          ticks++;
        break;

        case 0x14 :  // ZPG TRB
          address = readMem(PC);
          PC++;
          value8 = readMem(address);
          writeMem(address, value8 & ~A);
          P.Z = (value8 & A) == 0;
          ticks += 5;
        break;

        case 0x15 :  // ZPX ORA
          A |= readMem(readMem(PC) + X);
          PC++;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 4;
        break;

        case 0x16 :  // ZPX ASL
          address = readMem(PC) + X;
          PC++;
          value16 = readMem(address) << 1;
          writeMem(address, value16 & 0xFF);
          P.C = value16 > 0xFF;
          P.Z = value16 == 0;
          P.S = (value16 & 0xFF) > 0x7F;
          ticks += 6;
        break;

        case 0x17 :  // ZPG RMB
          address = readMem(PC);
          PC++;
          writeMem(address, readMem(address) & ~2);
          ticks += 5;
        break;

        case 0x18 :  // IMP CLC
          P.C = 0;
          ticks += 2;
        break;

        case 0x19 :  // ABY ORA
          address = readMem(PC);
          PC++;
          ticks += ((address + Y) & 0xFF00) ? 5 : 4;  // page crossing
          address |= readMem(PC) << 8;
          PC++;
          address += Y;
          A |= readMem(address);
          P.Z = A == 0;
          P.S = A > 0x7F;
        break;

        case 0x1A :  // ACC INC
          A++;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 2;
        break;

        case 0x1B :  // IMP NOP
          ticks++;
        break;

        case 0x1C :  // ABS TRB
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          PC++;
          value8 = readMem(address);
          P.Z = (value8 & A) == 0;
          writeMem(address, value8 & ~A);
          ticks += 6;
        break;

        case 0x1D :  // ABX ORA
          address = readMem(PC);
          PC++;
          ticks += ((address + X) & 0xFF00) ? 5 : 4;  // page crossing
          address |= readMem(PC) << 8;
          PC++;
          address += X;
          A |= readMem(address);
          P.Z = A == 0;
          P.S = A > 0x7F;
        break;

        case 0x1E :  // ABX ASL
          address = readMem(PC);
          PC++;
          ticks += address + X > 0xFF ? 7 : 6;
          address |= readMem(PC) << 8;
          PC++;
          address += X;
          value16 = readMem(address) << 1;
          P.C = value16 > 0xFF;
          value16 &= 0xFF;
          writeMem(address, value16);
          P.Z = value16 == 0;
          P.S = value16 > 0x7F;
        break;

        case 0x1F :  // ZPR BBR
          value8 = readMem(readMem(PC));
          PC++;
          address = readMem(PC);
          PC++;
          if (address & SIGN)
            address |= 0xFF00;  // jump backward
          if (!(value8 & 2))
            PC += address;
          ticks += 5;
        break;

        case 0x20 :  // ABS JSR
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          writeMem(0x100 + SP, (PC >> 8) & 0xFF);
          SP--;
          writeMem(0x100 + SP, PC & 0xFF);
          SP--;
          PC = address;
          ticks += 6;
        break;

        case 0x21 :  // IZX AND
          value8 = readMem(PC) + X;
          PC++;
          address = readMem(value8);
          value8++;
          address |= readMem(value8) << 8;
          A &= readMem(address);
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 6;
        break;

        case 0x22 :  // IMM NOP
          PC++;
          ticks += 2;
        break;

        case 0x23 :  // IMP NOP
          ticks++;
        break;

        case 0x24 :  // ZPG BIT
          address = readMem(PC);
          PC++;
          value8 = readMem(address);
          P.Z = (A & value8) == 0;
          P.byte = (P.byte & 0x3F) | (value8 & 0xC0);
          ticks += 3;
        break;

        case 0x25 :  // ZPG AND
          A &= readMem(readMem(PC));
          PC++;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 3;
        break;

        case 0x26 :  // ZPG ROL
          address = readMem(PC);
          PC++;
          value16 = (readMem(address) << 1) | P.C;
          P.C = (value16 & 0x100) != 0;
          value16 &= 0xFF;
          writeMem(address, value16);
          P.Z = value16 == 0;
          P.S = value16 > 0x7F;
          ticks += 5;
        break;

        case 0x27 :  // ZPG RMB
          address = readMem(PC);
          PC++;
          writeMem(address, readMem(address) & ~4);
          ticks += 5;
        break;

        case 0x28 :  // IMP PLP
          SP++;
          P.byte = readMem(0x100 + SP) | UNDEF;
          ticks += 4;
        break;

        case 0x29 :  // IMM AND
          A &= readMem(PC);
          PC++;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 2;
        break;

        case 0x2A :  // ACC ROL
          value16 = (A << 1) | P.C;
          P.C = (value16 & 0x100) != 0;
          A = value16 & 0xFF;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 2;
        break;

        case 0x2B :  // IMP NOP
          ticks++;
        break;

        case 0x2C :  // ABS BIT
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          PC++;
          value8 = readMem(address);
          P.Z = (A & value8) == 0;
          P.byte = (P.byte & 0x3F) | (value8 & 0xC0);
          ticks += 4;
        break;

        case 0x2D :  // ABS AND
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          PC++;
          A &= readMem(address);
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 4;
        break;

        case 0x2E :  // ABS ROL
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          PC++;
          value16 = (readMem(address) << 1) | P.C;
          P.C = (value16 & 0x100) != 0;
          value16 &= 0xFF;
          writeMem(address, value16);
          P.Z = value16 == 0;
          P.S = value16 > 0x7F;
          ticks += 6;
        break;

        case 0x2F :  // ZPR BBR
          value8 = readMem(readMem(PC));
          PC++;
          address = readMem(PC);
          PC++;
          if (address & SIGN)
            address |= 0xFF00;  // jump backward
          if (!(value8 & 4))
            PC += address;
          ticks += 5;
        break;

        case 0x30 :  // REL BMI
          address = readMem(PC);
          PC++;
          if (P.S) {  // branch taken
            ticks++;
            if (address & SIGN)
              address |= 0xFF00;  // jump backward
            if (((PC & 0xFF) + address) & 0xFF00)  // page crossing
              ticks++;
            PC += address;
          }
          ticks += 2;
        break;

        case 0x31 :  // IZY AND
          value8 = readMem(PC);
          PC++;
          address = readMem(value8);
          value8++;
          address |= readMem(value8) << 8;
          ticks += (((address & 0xFF) + Y) & 0xFF00) ? 6 : 5;  // page crossing
          address += Y;
          A &= readMem(address);
          P.Z = A == 0;
          P.S = A > 0x7F;
        break;

        case 0x32 :  // IZP AND
          value8 = readMem(PC);
          PC++;
          address = readMem(value8);
          value8++;
          address |= readMem(value8) << 8;
          A &= readMem(address);
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 5;
        break;

        case 0x33 :  // IMP NOP
          ticks++;
        break;

        case 0x34 :  // ZPX BIT
          address = (readMem(PC) + X) & 0xFF;
          PC++;
          value8 = readMem(address);
          P.Z = (A & value8) == 0;
          P.byte = (P.byte & 0x3F) | (value8 & 0xC0);
          ticks += 4;
        break;

        case 0x35 :  // ZPX AND
          address = (readMem(PC) + X) & 0xFF;
          PC++;
          A &= readMem(address);
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 4;
        break;

        case 0x36 :  // ZPX ROL
          address = (readMem(PC) + X) & 0xFF;
          PC++;
          value16 = (readMem(address) << 1) | P.C;
          P.C = value16 > 0xFF;
          value16 &= 0xFF;
          writeMem(address, value16);
          P.Z = value16 == 0;
          P.S = value16 > 0x7F;
          ticks += 6;
        break;

        case 0x37 :  // ZPG RMB
          address = readMem(PC);
          PC++;
          writeMem(address, readMem(address) & ~8);
          ticks += 5;
        break;

        case 0x38 :  // IMP SEC
          P.C = 1;
          ticks += 2;
        break;

        case 0x39 :  // ABY AND
          address = readMem(PC);
          PC++;
          ticks += ((address + Y) & 0xFF00) ? 5 : 4;  // page crossing
          address |= readMem(PC) << 8;
          PC++;
          address += Y;
          A &= readMem(address);
          P.Z = A == 0;
          P.S = A > 0x7F;
        break;

        case 0x3A :  // ACC DEC
          --A;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 2;
        break;

        case 0x3B :  // IMP NOP
          ticks++;
        break;

        case 0x3C :  // ABX BIT
          ticks += readMem(PC) + X > 0xFF ? 5 : 4;
          address = readMem(PC);
          PC++;
          address |= (readMem(PC) << 8) + X;
          PC++;
          value8 = readMem(address);
          P.Z = (A & value8) == 0;
          P.byte = (P.byte & 0x3F) | (value8 & 0xC0);
        break;

        case 0x3D :  // ABX AND
          address = readMem(PC);
          PC++;
          ticks += ((address + X) & 0xFF00) ? 5 : 4;  // page crossing
          address |= readMem(PC) << 8;
          PC++;
          address += X;
          A &= readMem(address);
          P.Z = A == 0;
          P.S = A > 0x7F;
        break;

        case 0x3E :  // ABX ROL
          address = readMem(PC);
          PC++;
          ticks += address + X > 0xFF ? 7 : 6;
          address |= readMem(PC) << 8;
          PC++;
          address += X;
          value16 = (readMem(address) << 1) | P.C;
          P.C = value16 > 0xFF;
          value16 &= 0xFF;
          writeMem(address, value16);
          P.Z = value16 == 0;
          P.S = value16 > 0x7F;
        break;

        case 0x3F :  // ZPR BBR
          value8 = readMem(readMem(PC));
          PC++;
          address = readMem(PC);
          PC++;
          if (address & SIGN)
            address |= 0xFF00;  // jump backward
          if (!(value8 & 8))
            PC += address;
          ticks += 5;
        break;

        case 0x40 :  // IMP RTI
          SP++;
          P.byte = readMem(0x100 + SP);
          SP++;
          PC = readMem(0x100 + SP);
          SP++;
          PC |= readMem(0x100 + SP) << 8;
          ticks += 6;
        break;

        case 0x41 :  // IZX EOR
          value8 = readMem(PC) + X;
          PC++;
          address = readMem(value8);
          value8++;
          address |= readMem(value8) << 8;
          A ^= readMem(address);
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 6;
        break;

        case 0x42 :  // IMM NOP
          PC++;
          ticks += 2;
        break;

        case 0x43 :  // IMP NOP
          ticks++;
        break;

        case 0x44 :  // ZPG NOP
          PC++;
          ticks += 3;
        break;

        case 0x45 :  // ZPG EOR
          address = readMem(PC);
          PC++;
          A ^= readMem(address);
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 3;
        break;

        case 0x46 :  // ZPG LSR
          address = readMem(PC);
          PC++;
          value8 = readMem(address);
          P.C = (value8 & 1) != 0;
          value8 = value8 >> 1;
          writeMem(address, value8);
          P.Z = value8 == 0;
          P.S = value8 > 0x7F;
          ticks += 5;
        break;

        case 0x47 :  // ZPG RMB
          address = readMem(PC);
          PC++;
          writeMem(address, readMem(address) & ~16);
          ticks += 5;
        break;

        case 0x48 :  // IMP PHA
          writeMem(0x100 + SP, A);
          SP--;
          ticks += 3;
        break;

        case 0x49 :  // IMM EOR
          A ^= readMem(PC);
          PC++;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 2;
        break;

        case 0x4A :  // ACC LSR
          P.C = (A & 1) != 0;
          A = A >> 1;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 2;
        break;

        case 0x4B :  // IMP NOP
          ticks++;
        break;

        case 0x4C :  // ABS JMP
          PC = readMem(PC) | (readMem(PC + 1) << 8);
          ticks += 3;
        break;

        case 0x4D :  // ABS EOR
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          PC++;
          A ^= readMem(address);
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 4;
        break;

        case 0x4E :  // ABS LSR
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          PC++;
          value8 = readMem(address);
          P.C = (value8 & 1) != 0;
          value8 = value8 >> 1;
          writeMem(address, value8);
          P.Z = value8 == 0;
          P.S = value8 > 0x7F;
          ticks += 6;
        break;

        case 0x4F :  // ZPR BBR
          value8 = readMem(readMem(PC));
          PC++;
          address = readMem(PC);
          PC++;
          if (address & SIGN)
            address |= 0xFF00;  // jump backward
          if (!(value8 & 16))
            PC += address;
          ticks += 5;
        break;

        case 0x50 :  // REL BVC
          address = readMem(PC);
          PC++;
          if (!P.V) {  // branch taken
            ticks++;
            if (address & SIGN)
              address |= 0xFF00;  // jump backward
            if (((PC & 0xFF) + address) & 0xFF00)  // page crossing
              ticks++;
            PC += address;
          }
          ticks += 2;
        break;

        case 0x51 :  // IZY EOR
          value8 = readMem(PC);
          PC++;
          address = readMem(value8);
          value8++;
          address |= readMem(value8) << 8;
          ticks += (((address & 0xFF) + Y) & 0xFF00) ? 6 : 5;  // page crossing
          A ^= readMem(address + Y);
          P.Z = A == 0;
          P.S = A > 0x7F;
        break;

        case 0x52 :  // IZP EOR
          value8 = readMem(PC);
          PC++;
          address = readMem(value8);
          value8++;
          address |= readMem(value8) << 8;
          A ^= readMem(address);
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 5;
        break;

        case 0x53 :  // IMP NOP
          ticks++;
        break;

        case 0x54 :  // ZPX NOP
          PC++;
          ticks += 4;
        break;

        case 0x55 :  // ZPX EOR
          address = (readMem(PC) + X) & 0xFF;
          PC++;
          A ^= readMem(address);
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 4;
        break;

        case 0x56 :  // ZPX LSR
          address = (readMem(PC) + X) & 0xFF;
          PC++;
          value8 = readMem(address);
          P.C = (value8 & 1) != 0;
          value8 = value8 >> 1;
          writeMem(address, value8);
          P.Z = value8 == 0;
          P.S = value8 > 0x7F;
          ticks += 6;
        break;

        case 0x57 :  // ZPG RMB
          address = readMem(PC);
          PC++;
          writeMem(address, readMem(address) & ~32);
          ticks += 5;
        break;

        case 0x58 :  // IMP CLI
          P.I = 0;
          ticks += 2;
        break;

        case 0x59 :  // ABY EOR
          address = readMem(PC);
          PC++;
          ticks += ((address + Y) & 0xFF00) ? 5 : 4;  // page crossing
          address |= readMem(PC) << 8;
          PC++;
          address += Y;
          A ^= readMem(address);
          P.Z = A == 0;
          P.S = A > 0x7F;
        break;

        case 0x5A :  // IMP PHY
          writeMem(0x100 + SP, Y);
          SP--;
          ticks += 3;
        break;

        case 0x5B :  // IMP NOP
          ticks++;
        break;

        case 0x5C :  // ABS NOP
          PC += 2;
          ticks += 8;
        break;

        case 0x5D :  // ABX EOR
          address = readMem(PC);
          PC++;
          ticks += ((address + X) & 0xFF00) ? 5 : 4;  // page crossing
          address |= readMem(PC) << 8;
          PC++;
          address += X;
          A ^= readMem(address);
          P.Z = A == 0;
          P.S = A > 0x7F;
        break;

        case 0x5E :  // ABX LSR
          address = readMem(PC);
          PC++;
          ticks += address + X > 0xFF ? 7 : 6;
          address |= readMem(PC) << 8;
          PC++;
          address += X;
          value8 = readMem(address);
          P.C = (value8 & 1) != 0;
          value8 = value8 >> 1;
          writeMem(address, value8);
          P.Z = value8 == 0;
          P.S = value8 > 0x7F;
        break;

        case 0x5F :  // ZPR BBR
          value8 = readMem(readMem(PC));
          PC++;
          address = readMem(PC);
          PC++;
          if (address & SIGN)
            address |= 0xFF00;  // jump backward
          if (!(value8 & 32))
            PC += address;
          ticks += 5;
        break;

        case 0x60 :  // IMP RTS
          SP++;
          PC = readMem(0x100 + SP);
          SP++;
          PC |= readMem(0x100 + SP) << 8;
          PC++;
          ticks += 6;
        break;

        case 0x61 :  // IZX ADC
          value8 = readMem(PC) + X;
          PC++;
          address = readMem(value8);
          value8++;
          address |= readMem(value8) << 8;
          value8 = readMem(address);
          value16 = A + value8 + P.C;
          P.V = ((value16 ^ A) & (value16 ^ value8) & 0x0080) != 0;
          if (P.D)
            value16 += ((((value16 + 0x66) ^ A ^ value8) >> 3) & 0x22) * 3;
          P.C = value16 > 0xFF;
          A = value16 & 0xFF;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 6;
        break;

        case 0x62 :  // IMM NOP
          PC++;
          ticks += 2;
        break;

        case 0x63 :  // IMP NOP
          ticks++;
        break;

        case 0x64 :  // ZPG STZ
          writeMem(readMem(PC), 0x00);
          PC++;
          ticks += 3;
        break;

        case 0x65 :  // ZPG ADC
          address = readMem(PC);
          PC++;
          value8 = readMem(address);
          value16 = A + value8 + P.C;
          P.V = ((value16 ^ A) & (value16 ^ value8) & 0x0080) != 0;
          if (P.D)
            value16 += ((((value16 + 0x66) ^ A ^ value8) >> 3) & 0x22) * 3;
          P.C = value16 > 0xFF;
          A = value16 & 0xFF;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 3;
        break;

        case 0x66 :  // ZPG ROR
          address = readMem(PC);
          PC++;
          value8 = readMem(address);
          value16 = (value8 >> 1) | (P.C << 7);
          P.C = (value8 & 0x1) != 0;
          value16 &= 0xFF;
          writeMem(address, value16);
          P.Z = value16 == 0;
          P.S = value16 > 0x7F;
          ticks += 5;
        break;

        case 0x67 :  // ZPG RMB
          address = readMem(PC);
          PC++;
          writeMem(address, readMem(address) & ~64);
          ticks += 5;
        break;

        case 0x68 :  // IMP PLA
          SP++;
          A = readMem(0x100 + SP);
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 4;
        break;

        case 0x69 :  // IMM ADC
          value8 = readMem(PC);
          PC++;
          value16 = A + value8 + P.C;
          P.V = ((value16 ^ A) & (value16 ^ value8) & 0x0080) != 0;
          if (P.D)
            value16 += ((((value16 + 0x66) ^ A ^ value8) >> 3) & 0x22) * 3;
          P.C = value16 > 0xFF;
          A = value16 & 0xFF;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 2;
        break;

        case 0x6A :  // ACC ROR
          value16 = (A >> 1) | (P.C << 7);
          P.C = (A & 0x1) != 0;
          A = value16 & 0xFF;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 2;
        break;

        case 0x6B :  // IMP NOP
          ticks++;
        break;

        case 0x6C :  // IND JMP
          address = readMem(PC) | readMem(PC + 1) << 8;
          PC = readMem(address) | (readMem(address + 1) << 8);
          ticks += 5;
        break;

        case 0x6D :  // ABS ADC
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          PC++;
          value8 = readMem(address);
          value16 = A + value8 + P.C;
          P.V = ((value16 ^ A) & (value16 ^ value8) & 0x0080) != 0;
          if (P.D)
            value16 += ((((value16 + 0x66) ^ A ^ value8) >> 3) & 0x22) * 3;
          P.C = value16 > 0xFF;
          A = value16 & 0xFF;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 4;
        break;

        case 0x6E :  // ABS ROR
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          PC++;
          value8 = readMem(address);
          value16 = (value8 >> 1) | (P.C << 7);
          P.C = (value8 & 0x1) != 0;
          value16 = value16 & 0xFF;
          writeMem(address, value16);
          P.Z = value16 == 0;
          P.S = value16 > 0x7F;
          ticks += 6;
        break;

        case 0x6F :  // ZPR BBR
          value8 = readMem(readMem(PC));
          PC++;
          address = readMem(PC);
          PC++;
          if (address & SIGN)
            address |= 0xFF00;  // jump backward
          if (!(value8 & 64))
            PC += address;
          ticks += 5;
        break;

        case 0x70 :  // REL BVS
          address = readMem(PC);
          PC++;
          if (P.V) {  // branch taken
            ticks++;
            if (((PC & 0xFF) + address) & 0xFF00)  // page crossing
              ticks++;
            if (address & SIGN)
              address |= 0xFF00;  // jump backward
            PC += address;
          }
          ticks += 2;
        break;

        case 0x71 :  // IZY ADC
          value8 = readMem(PC);
          PC++;
          address = readMem(value8);
          if ((address + Y) & 0xFF00)  // page crossing
            ticks++;
          value8++;
          address |= readMem(value8) << 8;
          address += Y;
          value8 = readMem(address);
          value16 = A + value8 + P.C;
          P.V = ((value16 ^ A) & (value16 ^ value8) & 0x0080) != 0;
          if (P.D)
            value16 += ((((value16 + 0x66) ^ A ^ value8) >> 3) & 0x22) * 3;
          P.C = value16 > 0xFF;
          A = value16 & 0xFF;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 5;
        break;

        case 0x72 :  // IZP ADC
          value8 = readMem(PC);
          PC++;
          address = readMem(value8);
          value8++;
          address |= readMem(value8) << 8;
          value8 = readMem(address);
          value16 = A + value8 + P.C;
          P.V = ((value16 ^ A) & (value16 ^ value8) & 0x0080) != 0;
          if (P.D)
            value16 += ((((value16 + 0x66) ^ A ^ value8) >> 3) & 0x22) * 3;
          P.C = value16 > 0xFF;
          A = value16 & 0xFF;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 5;
        break;

        case 0x73 :  // IMP NOP
          ticks++;
        break;

        case 0x74 :  // ZPX STZ
          value8 = readMem(PC) + X;  // 8bit -> zp wrap around
          PC++;
          writeMem(value8, 0x00);
          ticks += 4;
        break;

        case 0x75 :  // ZPX ADC
          address = (readMem(PC) + X) & 0xFF;
          PC++;
          value8 = readMem(address);
          value16 = A + value8 + P.C;
          P.V = ((value16 ^ A) & (value16 ^ value8) & 0x0080) != 0;
          if (P.D)
            value16 += ((((value16 + 0x66) ^ A ^ value8) >> 3) & 0x22) * 3;
          P.C = value16 > 0xFF;
          A = value16 & 0xFF;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 4;
        break;

        case 0x76 :  // ZPX ROR
          address = (readMem(PC) + X) & 0xFF;
          PC++;
          value8 = readMem(address);
          value16 = (value8 >> 1) | (P.C << 7);
          P.C = (value8 & 0x1) != 0;
          value16 = value16 & 0xFF;
          writeMem(address, value16);
          P.Z = value16 == 0;
          P.S = value16 > 0x7F;
          ticks += 6;
        break;

        case 0x77 :  // ZPG RMB
          address = readMem(PC);
          PC++;
          writeMem(address, readMem(address) & ~128);
          ticks += 5;
        break;

        case 0x78 :  // IMP SEI
          P.I = 1;
          ticks += 2;
        break;

        case 0x79 :  // ABY ADC
          if ((readMem(PC) + Y) & 0xFF00)
            ticks++;
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          PC++;
          address += Y;
          value8 = readMem(address);
          value16 = A + value8 + P.C;
          P.V = ((value16 ^ A) & (value16 ^ value8) & 0x0080) != 0;
          if (P.D)
            value16 += ((((value16 + 0x66) ^ A ^ value8) >> 3) & 0x22) * 3;
          P.C = value16 > 0xFF;
          A = value16 & 0xFF;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 4;
        break;

        case 0x7A :  // IMP PLY
          SP++;
          Y = readMem(0x100 + SP);
          P.Z = Y == 0;
          P.S = Y > 0x7F;
          ticks += 4;
        break;

        case 0x7B :  // IMP NOP
          ticks++;
        break;

        case 0x7C :  // IAX JMP
          ticks += ((PC & 0xFF) + X) > 0xFF ? 7 : 6;
          address = (readMem((PC + 1) & 0xFFFF) << 8) + readMem(PC) + X;
          PC = (readMem(address) | (readMem((address + 1) & 0xFFFF) << 8));
        break;

        case 0x7D :  // ABX ADC
          if ((readMem(PC) + X) & 0xFF00)
            ticks++;
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          PC++;
          address += X;
          value8 = readMem(address);
          value16 = A + value8 + P.C;
          P.V = ((value16 ^ A) & (value16 ^ value8) & 0x0080) != 0;
          if (P.D)
            value16 += ((((value16 + 0x66) ^ A ^ value8) >> 3) & 0x22) * 3;
          P.C = value16 > 0xFF;
          A = value16 & 0xFF;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 4;
        break;

        case 0x7E :  // ABX ROR
          address = readMem(PC);
          PC++;
          ticks += address + X > 0xFF ? 7 : 6;
          address |= readMem(PC) << 8;
          PC++;
          address += X;
          value8 = readMem(address);
          value16 = (value8 >> 1) | (P.C << 7);
          P.C = (value8 & 0x1) != 0;
          value16 = value16 & 0xFF;
          writeMem(address, value16);
          P.Z = value16 == 0;
          P.S = value16 > 0x7F;
        break;

        case 0x7F :  // ZPR BBR
          value8 = readMem(readMem(PC));
          PC++;
          address = readMem(PC);
          PC++;
          if (address & SIGN)
            address |= 0xFF00;  // jump backward
          if (!(value8 & 128))
            PC += address;
          ticks += 5;
        break;

        case 0x80 :  // REL BRA
          address = readMem(PC);
          PC++;
          if (address & SIGN)
            address |= 0xFF00;  // jump backward
          ticks += ((PC & 0xFF) + address) & 0xFF00 ? 4 : 3;
          PC += address;
        break;

        case 0x81 :  // IZX STA
          value8 = readMem(PC) + X;
          PC++;
          address = readMem(value8);
          value8++;
          address |= readMem(value8) << 8;
          writeMem(address, A);
          ticks += 6;
        break;

        case 0x82 :  // IMM NOP
          PC++;
          ticks += 2;
        break;

        case 0x83 :  // IMP NOP
          ticks++;
        break;

        case 0x84 :  // ZPG STY
          writeMem(readMem(PC), Y);
          PC++;
          ticks += 3;
        break;

        case 0x85 :  // ZPG STA
          writeMem(readMem(PC), A);
          PC++;
          ticks += 3;
        break;

        case 0x86 :  // ZPG STX
          writeMem(readMem(PC), X);
          PC++;
          ticks += 3;
        break;

        case 0x87 :  // ZPG SMB
          address = readMem(PC);
          PC++;
          writeMem(address, readMem(address) | 1);
          ticks += 5;
        break;

        case 0x88 :  // IMP DEY
          Y--;
          P.Z = (Y & 0xFF) == 0;
          P.S = (Y & SIGN) != 0;
          ticks += 2;
        break;

        case 0x89 :  // IMM BIT
          P.Z = (A & readMem(PC)) == 0;
          PC++;
          ticks += 2;
        break;

        case 0x8A :  // IMP TXA
          A = X;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 2;
        break;

        case 0x8B :  // IMP NOP
          ticks++;
        break;

        case 0x8C :  // ABS STY
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          PC++;
          writeMem(address, Y);
          ticks += 4;
        break;

        case 0x8D :  // ABS STA
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          PC++;
          writeMem(address, A);
          ticks += 4;
        break;

        case 0x8E :  // ABS STX
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          PC++;
          writeMem(address, X);
          ticks += 4;
        break;

        case 0x8F :  // ZPR BBS
          value8 = readMem(readMem(PC));
          PC++;
          address = readMem(PC);
          PC++;
          if (address & SIGN)
            address |= 0xFF00;  // jump backward
          if (value8 & 1)
            PC += address;
          ticks += 5;
        break;

        case 0x90 :  // REL BCC
          address = readMem(PC);
          PC++;
          if (!P.C) {  // branch taken
            ticks++;
            if (((PC & 0xFF) + address) & 0xFF00)  // page crossing
              ticks++;
            if (address & SIGN)
              address |= 0xFF00;  // jump backward
            PC += address;
          }
          ticks += 2;
        break;

        case 0x91 :  // IZY STA
          value8 = readMem(PC);
          PC++;
          address = readMem(value8);
          value8++;
          address |= readMem(value8) << 8;
          address += Y;
          writeMem(address, A);
          ticks += 6;
        break;

        case 0x92 :  // IZP STA
          value8 = readMem(PC);
          PC++;
          address = readMem(value8);
          value8++;
          address |= readMem(value8) << 8;
          writeMem(address, A);
          ticks += 5;
        break;

        case 0x93 :  // IMP NOP
          ticks++;
        break;

        case 0x94 :  // ZPX STY
          address = (readMem(PC) + X) & 0xFF;
          PC++;
          writeMem(address, Y);
          ticks += 4;
        break;

        case 0x95 :  // ZPX STA
          writeMem((readMem(PC) + X) & 0xFF, A);
          PC++;
          ticks += 4;
        break;

        case 0x96 :  // ZPY STX
          writeMem((readMem(PC) + Y) & 0xFF, X);
          PC++;
          ticks += 4;
        break;

        case 0x97 :  // ZPG SMB
          address = readMem(PC);
          PC++;
          writeMem(address, readMem(address) | 2);
          ticks += 5;
        break;

        case 0x98 :  // IMP TYA
          A = Y;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 2;
        break;

        case 0x99 :  // ABY STA
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          PC++;
          address += Y;
          writeMem(address, A);
          ticks += 5;
        break;

        case 0x9A :  // IMP TXS
          SP = X;
          ticks += 2;
        break;

        case 0x9B :  // IMP NOP
          ticks++;
        break;

        case 0x9C :  // ABS STZ
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          PC++;
          writeMem(address, 0x00);
          ticks += 4;
        break;

        case 0x9D :  // ABX STA
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          PC++;
          address += X;
          writeMem(address, A);
          ticks += 5;
        break;

        case 0x9E :  // ABX STZ
          ticks +=  readMem(PC) + X > 0xFF ? 6 : 5;
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          PC++;
          address += X;
          writeMem(address, 0x00);
        break;

        case 0x9F :  // ZPR BBS
          value8 = readMem(readMem(PC));
          PC++;
          address = readMem(PC);
          PC++;
          if (address & SIGN)
            address |= 0xFF00;  // jump backward
          if (value8 & 2)
            PC += address;
          ticks += 5;
        break;

        case 0xA0 :  // IMM LDY
          Y = readMem(PC);
          PC++;
          P.Z = Y == 0;
          P.S = Y > 0x7F;
          ticks += 2;
        break;

        case 0xA1 :  // IZX LDA
          value8 = readMem(PC) + X;
          PC++;
          address = readMem(value8);
          value8++;
          address |= readMem(value8) << 8;
          A = readMem(address);
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 6;
        break;

        case 0xA2 :  // IMM LDX
          address = PC;
          PC++;
          X = readMem(address);
          P.Z = X == 0;
          P.S = X > 0x7F;
          ticks += 2;
        break;

        case 0xA4 :  // ZPG LDY
          Y = readMem(readMem(PC));
          PC++;
          P.Z = Y == 0;
          P.S = Y > 0x7F;
          ticks += 3;
        break;

        case 0xA5 :  // ZPG LDA
          A = readMem(readMem(PC));
          PC++;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 3;
        break;

        case 0xA6 :  // ZPG LDX
          X = readMem(readMem(PC));
          PC++;
          P.Z = X == 0;
          P.S = X > 0x7F;
          ticks += 3;
        break;

        case 0xA7 :  // ZPG SMB
          address = readMem(PC);
          PC++;
          writeMem(address, readMem(address) | 4);
          ticks += 5;
        break;

        case 0xA8 :  // IMP TAY
          Y = A;
          P.Z = Y == 0;
          P.S = Y > 0x7F;
          ticks += 2;
        break;

        case 0xA9 :  // IMM LDA
          A = readMem(PC);
          PC++;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 2;
        break;

        case 0xAA :  // IMP TAX
          X = A;
          P.Z = X == 0;
          P.S = X > 0x7F;
          ticks += 2;
        break;

        case 0xAB :  // IMP NOP
          ticks++;
        break;

        case 0xAC :  // ABS LDY
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          PC++;
          Y = readMem(address);
          P.Z = Y == 0;
          P.S = Y > 0x7F;
          ticks += 4;
        break;

        case 0xAD :  // ABS LDA
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          PC++;
          A = readMem(address);
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 4;
        break;

        case 0xAE :  // ABS LDX
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          PC++;
          X = readMem(address);
          P.Z = X == 0;
          P.S = X > 0x7F;
          ticks += 4;
        break;

        case 0xAF :  // ZPR BBS
          value8 = readMem(readMem(PC));
          PC++;
          address = readMem(PC);
          PC++;
          if (address & SIGN)
            address |= 0xFF00;  // jump backward
          if (value8 & 4)
            PC += address;
          ticks += 5;
        break;

        case 0xB0 :  // REL BCS
          address = readMem(PC);
          PC++;
          if (P.C) {  // branch taken
            ticks++;
            if (address & SIGN)
              address |= 0xFF00;  // jump backward
            if (((PC & 0xFF) + address) & 0xFF00)  // page crossing
              ticks++;
            PC += address;
          }
          ticks += 2;
        break;

        case 0xB1 :  // IZY LDA
          value8 = readMem(PC);
          PC++;
          address = readMem(value8);
          value8++;
          address |= readMem(value8) << 8;
          A = readMem(address + Y);
          ticks += (((address & 0xFF) + Y) & 0xFF00) ? 6 : 5;  // page crossing
          P.Z = A == 0;
          P.S = A > 0x7F;
        break;

        case 0xB2 :  // IZP LDA
          value8 = readMem(PC);
          PC++;
          address = readMem(value8);
          value8++;
          address |= readMem(value8) << 8;
          A = readMem(address);
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 5;
        break;

        case 0xB3 :  // IMP NOP
          ticks++;
        break;

        case 0xB4 :  // ZPX LDY
          address = (readMem(PC) + X) & 0xFF;
          PC++;
          Y = readMem(address);
          P.Z = Y == 0;
          P.S = Y > 0x7F;
          ticks += 4;
        break;

        case 0xB5 :  // ZPX LDA
          address = (readMem(PC) + X) & 0xFF;
          PC++;
          A = readMem(address);
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 4;
        break;

        case 0xB6 :  // ZPY LDX
          address = (readMem(PC) + Y) & 0xFF;
          PC++;
          X = readMem(address);
          P.Z = X == 0;
          P.S = X > 0x7F;
          ticks += 4;
        break;

        case 0xB7 :  // ZPG SMB
          address = readMem(PC);
          PC++;
          writeMem(address, readMem(address) | 8);
          ticks += 5;
        break;

        case 0xB8 :  // IMP CLV
          P.V = 0;
          ticks += 2;
        break;

        case 0xB9 :  // ABY LDA
          address = readMem(PC);
          PC++;
          ticks += ((address + Y) & 0xFF00) ? 5 : 4;  // page crossing
          address |= readMem(PC) << 8;
          PC++;
          address += Y;
          A = readMem(address);
          P.Z = A == 0;
          P.S = A > 0x7F;
        break;

        case 0xBA :  // IMP TSX
          X = SP;
          P.Z = X == 0;
          P.S = X > 0x7F;
          ticks += 2;
        break;

        case 0xBB :  // IMP NOP
          ticks++;
        break;

        case 0xBC :  // ABX LDY
          address = readMem(PC);
          PC++;
          ticks += ((address + X) & 0xFF00) ? 5 : 4;  // page crossing
          address |= readMem(PC) << 8;
          PC++;
          address += X;
          Y = readMem(address);
          P.Z = Y == 0;
          P.S = Y > 0x7F;
        break;

        case 0xBD :  // ABX LDA
          address = readMem(PC);
          PC++;
          ticks += ((address + X) & 0xFF00) ? 5 : 4;  // page crossing
          address |= readMem(PC) << 8;
          PC++;
          address += X;
          A = readMem(address);
          P.Z = A == 0;
          P.S = A > 0x7F;
        break;

        case 0xBE :  // ABY LDX
          address = readMem(PC);
          PC++;
          ticks += ((address + Y) & 0xFF00) ? 5 : 4;  // page crossing
          address |= readMem(PC) << 8;
          PC++;
          address += Y;
          X = readMem(address);
          P.Z = X == 0;
          P.S = X > 0x7F;
        break;

        case 0xBF :  // ZPR BBS
          value8 = readMem(readMem(PC));
          PC++;
          address = readMem(PC);
          PC++;
          if (address & SIGN)
            address |= 0xFF00;  // jump backward
          if (value8 & 8)
            PC += address;
          ticks += 5;
        break;

        case 0xC0 :  // IMM CPY
          value8 = readMem(PC);
          PC++;
          P.Z = ((Y - value8) & 0xFF) == 0;
          P.S = ((Y - value8) & SIGN) != 0;
          P.C = (Y >= value8) != 0;
          ticks += 2;
        break;

        case 0xC1 :  // IZX CMP
          value8 = readMem(PC) + X;
          PC++;
          address = readMem(value8);
          value8++;
          address |= readMem(value8) << 8;
          value8 = readMem(address);
          P.Z = ((A - value8) & 0xFF) == 0;
          P.S = ((A - value8) & SIGN) != 0;
          P.C = (A >= value8) != 0;
          ticks += 6;
        break;

        case 0xC2 :  // IMM NOP
          PC++;
          ticks += 2;
        break;

        case 0xC3 :  // IMP NOP
          ticks++;
        break;

        case 0xC4 :  // ZPG CPY
          value8 = readMem(readMem(PC));
          PC++;
          P.Z = ((Y - value8) & 0xFF) == 0;
          P.S = ((Y - value8) & SIGN) != 0;
          P.C = (Y >= value8) != 0;
          ticks += 3;
        break;

        case 0xC5 :  // ZPG CMP
          value8 = readMem(readMem(PC));
          PC++;
          P.Z = ((A - value8) & 0xFF) == 0;
          P.S = ((A - value8) & SIGN) != 0;
          P.C = (A >= value8) != 0;
          ticks += 3;
        break;

        case 0xC6 :  // ZPG DEC
          address = readMem(PC);
          PC++;
          value8 = readMem(address);
          --value8;
          writeMem(address, value8);
          P.Z = value8 == 0;
          P.S = value8 > 0x7F;
          ticks += 5;
        break;

        case 0xC7 :  // ZPG SMB
          address = readMem(PC);
          PC++;
          writeMem(address, readMem(address) | 16);
          ticks += 5;
        break;

        case 0xC8 :  // IMP INY
          Y++;
          P.Z = Y  == 0;
          P.S = Y > 0x7F;
          ticks += 2;
        break;

        case 0xC9 :  // IMM CMP
          value8 = readMem(PC);
          PC++;
          P.Z = ((A - value8) & 0xFF) == 0;
          P.S = ((A - value8) & SIGN) != 0;
          P.C = (A >= value8) != 0;
          ticks += 2;
        break;

        case 0xCA :  // IMP DEX
          X--;
          P.Z = (X & 0xFF) == 0;
          P.S = X > 0x7F;
          ticks += 2;
        break;

        case 0xCB :  // IMP WAI
          state = wait;
          ticks += 3;
        break;

        case 0xCC :  // ABS CPY
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          PC++;
          value8 = readMem(address);
          P.Z = ((Y - value8) & 0xFF) == 0;
          P.S = ((Y - value8) & SIGN) != 0;
          P.C = (Y >= value8) != 0;
          ticks += 4;
        break;

        case 0xCD :  // ABS CMP
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          PC++;
          value8 = readMem(address);
          P.Z = ((A - value8) & 0xFF) == 0;
          P.S = ((A - value8) & SIGN) != 0;
          P.C = (A >= value8) != 0;
          ticks += 4;
        break;

        case 0xCE :  // ABS DEC
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          PC++;
          value8 = readMem(address);
          value8--;
          writeMem(address, value8);
          P.Z = value8 == 0;
          P.S = value8 > 0x7F;
          ticks += 3;
        break;

        case 0xCF :  // ZPR BBS
          value8 = readMem(readMem(PC));
          PC++;
          address = readMem(PC);
          PC++;
          if (address & SIGN)
            address |= 0xFF00;  // jump backward
          if (value8 & 16)
            PC += address;
          ticks += 5;
        break;

        case 0xD0 :  // REL BNE
          address = readMem(PC);
          PC++;
          if (!P.Z) {  // branch taken
            ticks++;
            if (address & SIGN)
              address |= 0xFF00;  // jump backward
            if (((PC & 0xFF) + address) & 0xFF00)  // page crossing
              ticks++;
            PC += address;
          }
          ticks += 2;
        break;

        case 0xD1 :  // IZY CMP
          value8 = readMem(PC);
          PC++;
          address = readMem(value8);
          ticks += ((address + Y) & 0xFF00) ? 6 : 5;  // page crossing
          value8++;
          address |= readMem(value8) << 8;
          address += Y;
          value8 = readMem(address);
          P.Z = ((A - value8) & 0xFF) == 0;
          P.S = ((A - value8) & SIGN) != 0;
          P.C = (A >= value8) != 0;
        break;

        case 0xD2 :  // IZP CMP
          value8 = readMem(PC);
          PC++;
          address = readMem(value8);
          value8++;
          address |= readMem(value8) << 8;
          value8 = readMem(address);
          P.Z = ((A - value8) & 0xFF) == 0;
          P.S = ((A - value8) & SIGN) != 0;
          P.C = (A >= value8) != 0;
          ticks += 5;
        break;

        case 0xD3 :  // IMP NOP
          ticks++;
        break;

        case 0xD4 :  // ZPX NOP
          PC++;
          ticks += 4;
        break;

        case 0xD5 :  // ZPX CMP
          address = (readMem(PC) + X) & 0xFF;
          PC++;
          value8 = readMem(address);
          P.Z = ((A - value8) & 0xFF) == 0;
          P.S = ((A - value8) & SIGN) != 0;
          P.C = (A >= value8) != 0;
          ticks += 4;
        break;

        case 0xD6 :  // ZPX DEC
          address = (readMem(PC) + X) & 0xFF;
          PC++;
          value8 = readMem(address);
          value8--;
          writeMem(address, value8);
          P.Z = value8 == 0;
          P.S = value8 > 0x7F;
          ticks += 6;
        break;

        case 0xD7 :  // ZPG SMB
          address = readMem(PC);
          PC++;
          writeMem(address, readMem(address) | 32);
          ticks += 5;
        break;

        case 0xD8 :  // IMP CLD
          P.D = 0;
          ticks += 2;
        break;

        case 0xD9 :  // ABY CMP
          address = readMem(PC);
          PC++;
          ticks += ((address + Y) & 0xFF00) ? 5 : 4;  // page crossing
          address |= readMem(PC) << 8;
          PC++;
          address += Y;
          value8 = readMem(address);
          P.Z = ((A - value8) & 0xFF) == 0;
          P.S = ((A - value8) & SIGN) != 0;
          P.C = (A >= value8) != 0;
        break;

        case 0xDA :  // IMP PHX
          writeMem(0x100 + SP, X);
          SP--;
          ticks += 3;
        break;

        case 0xDB :  // IMP STP
          state = stop;
          ticks += 3;
        break;

        case 0xDC :  // ABS NOP
          PC += 2;
          ticks += 4;
        break;

        case 0xDD :  // ABX CMP
          address = readMem(PC);
          PC++;
          ticks += ((address + X) & 0xFF00) ? 5 : 4;  // page crossing
          address |= readMem(PC) << 8;
          PC++;
          address += X;
          value8 = readMem(address);
          P.Z = ((A - value8) & 0xFF) == 0;
          P.S = ((A - value8) & SIGN) != 0;
          P.C = (A >= value8) != 0;
        break;

        case 0xDE :  // ABX DEC
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          PC++;
          address += X;
          value8 = readMem(address);
          value8--;
          writeMem(address, value8);
          P.Z = value8 == 0;
          P.S = (value8 & SIGN) != 0;
          ticks += 7;
        break;

        case 0xDF :  // ZPR BBS
          value8 = readMem(readMem(PC));
          PC++;
          address = readMem(PC);
          PC++;
          if (address & SIGN)
            address |= 0xFF00;  // jump backward
          if (value8 & 32)
            PC += address;
          ticks += 5;
        break;

        case 0xE0 :  // IMM CPX
          value8 = readMem(PC);
          PC++;
          P.Z = ((X - value8) & 0xFF) == 0;
          P.S = ((X - value8) & SIGN) != 0;
          P.C = (X >= value8) != 0;
          ticks += 2;
        break;

        case 0xE1 :  // IZX SBC
          value8 = readMem(PC) + X;
          PC++;
          address = readMem(value8);
          value8++;
          address |= readMem(value8) << 8;
          value8 = readMem(address);
          value8 ^= 0xFF;
          if (P.D)
            value8 -= 0x0066;
          value16 = A + value8 + P.C;
          P.V = ((value16 ^ A) & (value16 ^ value8) & 0x0080) != 0;
          if (P.D)
            value16 += ((((value16 + 0x66) ^ A ^ value8) >> 3) & 0x22) * 3;
          P.C = value16 > 0xFF;
          A = value16 & 0xFF;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 6;
        break;

        case 0xE2 :  // IMM NOP
          PC++;
          ticks += 2;
        break;

        case 0xE3 :  // IMP NOP
          ticks++;
        break;

        case 0xE4 :  // ZPG CPX
          value8 = readMem(readMem(PC));
          PC++;
          P.Z = ((X - value8) & 0xFF) == 0;
          P.S = ((X - value8) & SIGN) != 0;
          P.C = (X >= value8) != 0;
          ticks += 3;
        break;

        case 0xE5 :  // ZPG SBC
          value8 = readMem(readMem(PC));
          PC++;
          value8 ^= 0xFF;
          if (P.D)
            value8 -= 0x0066;
          value16 = A + value8 + P.C;
          P.V = ((value16 ^ A) & (value16 ^ value8) & 0x0080) != 0;
          if (P.D)
            value16 += ((((value16 + 0x66) ^ A ^ value8) >> 3) & 0x22) * 3;
          P.C = value16 > 0xFF;
          A = value16 & 0xFF;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 3;
        break;

        case 0xE6 :  // ZPG INC
          address = readMem(PC);
          PC++;
          value8 = readMem(address);
          value8++;
          writeMem(address, value8);
          P.Z = value8 == 0;
          P.S = value8 > 0x7F;
          ticks += 5;
        break;

        case 0xE7 :  // ZPG SMB
          address = readMem(PC);
          PC++;
          writeMem(address, readMem(address) | 64);
          ticks += 5;
        break;

        case 0xE8 :  // IMP INX
          X++;
          P.Z = X == 0;
          P.S = X > 0x7F;
          ticks += 2;
        break;

        case 0xE9 :  // IMM SBC
          value8 = readMem(PC);
          PC++;
          value8 ^= 0xFF;
          if (P.D)
            value8 -= 0x0066;
          value16 = A + value8 + (P.C);
          P.V = ((value16 ^ A) & (value16 ^ value8) & 0x0080) != 0;
          if (P.D)
            value16 += ((((value16 + 0x66) ^ A ^ value8) >> 3) & 0x22) * 3;
          P.C = value16 > 0xFF;
          A = value16 & 0xFF;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 2;
        break;

        case 0xEA:  // IMP NOP
          ticks += 2;
        break;

        case 0xEB :  // IMP NOP
          ticks++;
        break;

        case 0xEC :  // ABS CPX
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          PC++;
          value8 = readMem(address);
          P.Z = ((X - value8) & 0xFF) == 0;
          P.S = ((X - value8) & SIGN) != 0;
          P.C = (X >= value8) != 0;
          ticks += 4;
        break;

        case 0xED :  // ABS SBC
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          PC++;
          value8 = readMem(address);
          value8 ^= 0xFF;
          if (P.D)
            value8 -= 0x0066;
          value16 = A + value8 + P.C;
          P.V = ((value16 ^ A) & (value16 ^ value8) & 0x0080) != 0;
          if (P.D)
            value16 += ((((value16 + 0x66) ^ A ^ value8) >> 3) & 0x22) * 3;
          P.C = value16 > 0xFF;
          A = value16 & 0xFF;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 4;
        break;

        case 0xEE :  // ABS INC
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          PC++;
          value8 = readMem(address);
          value8++;
          writeMem(address, value8);
          P.Z = value8 == 0;
          P.S = value8 > 0x7F;
          ticks += 6;
        break;

        case 0xEF :  // ZPR BBS
          value8 = readMem(readMem(PC));
          PC++;
          address = readMem(PC);
          PC++;
          if (address & SIGN)
            address |= 0xFF00;  // jump backward
          if (value8 & 64)
            PC += address;
          ticks += 5;
        break;

        case 0xF0 :  // REL BEQ
          address = readMem(PC);
          PC++;
          if (P.Z) {  // branch taken
            ticks++;
            if (address & SIGN)
              address |= 0xFF00;  // jump backward
            if (((PC & 0xFF) + address) & 0xFF00)  // page crossing
              ticks++;
            PC += address;
          }
          ticks += 2;
        break;

        case 0xF1 :  // IZY SBC
          value8 = readMem(PC);
          PC++;
          address = readMem(value8);
          if ((address + Y) & 0xFF00)  // page crossing
            ticks++;
          value8++;
          address |= readMem(value8) << 8;
          address += Y;
          value8 = readMem(address);
          value8 ^= 0xFF;
          if (P.D)
            value8 -= 0x0066;
          value16 = A + value8 + P.C;
          P.V = ((value16 ^ A) & (value16 ^ value8) & 0x0080) != 0;
          if (P.D)
            value16 += ((((value16 + 0x66) ^ A ^ value8) >> 3) & 0x22) * 3;
          P.C = value16 > 0xFF;
          A = value16 & 0xFF;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 5;
        break;

        case 0xF2 :  // IZP SBC
          value8 = readMem(PC);
          PC++;
          address = readMem(value8);
          value8++;
          address |= readMem(value8) << 8;
          value8 = readMem(address);
          value8 ^= 0xFF;
          if (P.D)
            value8 -= 0x0066;
          value16 = A + value8 + P.C;
          P.V = ((value16 ^ A) & (value16 ^ value8) & 0x0080) != 0;
          if (P.D)
            value16 += ((((value16 + 0x66) ^ A ^ value8) >> 3) & 0x22) * 3;
          P.C = value16 > 0xFF;
          A = value16 & 0xFF;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 5;
        break;

        case 0xF3 :  // IMP NOP
          ticks++;
        break;

        case 0xF4 :  // ZPX NOP
          PC++;
          ticks += 4;
        break;

        case 0xF5 :  // ZPX SBC
          address = (readMem(PC) + X) & 0xFF;
          PC++;
          value8 = readMem(address);
          value8 ^= 0xFF;
          if (P.D)
            value8 -= 0x0066;
          value16 = A + value8 + P.C;
          P.V = ((value16 ^ A) & (value16 ^ value8) & 0x0080) != 0;
          if (P.D)
            value16 += ((((value16 + 0x66) ^ A ^ value8) >> 3) & 0x22) * 3;
          P.C = value16 > 0xFF;
          A = value16 & 0xFF;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 4;
        break;

        case 0xF6 :  // ZPX INC
          address = (readMem(PC) + X) & 0xFF;
          PC++;
          value8 = readMem(address);
          value8++;
          writeMem(address, value8);
          P.Z = value8 == 0;
          P.S = value8 > 0x7F;
          ticks += 6;
        break;

        case 0xF7 :  // ZPG SMB
          address = readMem(PC);
          PC++;
          writeMem(address, readMem(address) | 128);
          ticks += 5;
        break;

        case 0xF8 :  // IMP SED
          P.D = 1;
          ticks += 2;
        break;

        case 0xF9 :  // ABY SBC
          address = readMem(PC);
          PC++;
          if ((address + Y) & 0xFF00)  // page crossing
            ticks++;
          address |= readMem(PC) << 8;
          PC++;
          address += Y;
          value8 = readMem(address);
          value8 ^= 0xFF;
          if (P.D)
            value8 -= 0x0066;
          value16 = A + value8 + P.C;
          P.V = ((value16 ^ A) & (value16 ^ value8) & 0x0080) != 0;
          if (P.D)
            value16 += ((((value16 + 0x66) ^ A ^ value8) >> 3) & 0x22) * 3;
          P.C = value16 > 0xFF;
          A = value16 & 0xFF;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 4;
        break;

        case 0xFA :  // IMP PLX
          SP++;
          X = readMem(0x100 + SP);
          P.Z = X == 0;
          P.S = X > 0x7F;
          ticks += 4;
        break;

        case 0xFB :  // IMP NOP
          ticks++;
        break;

        case 0xFC :  // ABS NOP
          PC += 2;
          ticks += 4;
        break;

        case 0xFD :  // ABX SBC
          address = readMem(PC);
          PC++;
          if ((address + X) & 0xFF00)  // page crossing
            ticks++;
          address |= readMem(PC) << 8;
          PC++;
          address += X;
          value8 = readMem(address);
          value8 ^= 0xFF;
          if (P.D)
            value8 -= 0x0066;
          value16 = A + value8 + P.C;
          P.V = ((value16 ^ A) & (value16 ^ value8) & 0x0080) != 0;
          if (P.D)
            value16 += ((((value16 + 0x66) ^ A ^ value8) >> 3) & 0x22) * 3;
          P.C =  (value16 & 0xFF00) != 0;
          A = value16 & 0xFF;
          P.Z = A == 0;
          P.S = A > 0x7F;
          ticks += 4;
        break;

        case 0xFE :  // ABX INC
          address = readMem(PC);
          PC++;
          address |= readMem(PC) << 8;
          PC++;
          address += X;
          value8 = readMem(address);
          value8++;
          writeMem(address, value8);
          P.Z = value8 == 0;
          P.S = value8 > 0x7F;
          ticks += 7;
        break;

        case 0xFF :  // ZPR BBS
          value8 = readMem(readMem(PC));
          PC++;
          address = readMem(PC);
          PC++;
          if (address & SIGN)
            address |= 0xFF00;  // jump backward
          if (value8 & 128)
            PC += address;
          ticks += 5;
        break;

      } // end of switch
    }  // end of if
  }  // end of while
  return PC;
}



//==============================================================================
// Everything down below are optionnal utilities used during devellopment
// mostly using Klaus Dormann's functonnal tests published at
// https://github.com/Klaus2m5/6502_65C02_functional_tests
// but also BigEd's timing tests code published at
// https://github.com/BigEd/6502timing
// and http://forum.6502.org/viewtopic.php?f=8&t=3340
//
// for functionnal tests :
//    g++ -Wall -O3 -D __TESTS__ puce65c02.cpp -o puce65c02 && ./puce65c02
// or, for detailled info
//     g++ -Wall -O3 -D __TESTS__ -D VERBOSE puce65c02.cpp -o puce65c02 && ./puce65c02
// or, if you want to check the cycle accuracy :
//    g++ -Wall -O3 -D __TESTS__ -D TIMING puce65c02.cpp -o puce65c02 && ./puce65c02
// and see bottom of file for more info


#if __TESTS__
    #include<cstdio>

    static const char* mn[256] = {
      "BRK","ORA","NOP","NOP","TSB","ORA","ASL","RMB","PHP","ORA","ASL","NOP","TSB","ORA","ASL","BBR",
      "BPL","ORA","ORA","NOP","TRB","ORA","ASL","RMB","CLC","ORA","INC","NOP","TRB","ORA","ASL","BBR",
      "JSR","AND","NOP","NOP","BIT","AND","ROL","RMB","PLP","AND","ROL","NOP","BIT","AND","ROL","BBR",
      "BMI","AND","AND","NOP","BIT","AND","ROL","RMB","SEC","AND","DEC","NOP","BIT","AND","ROL","BBR",
      "RTI","EOR","NOP","NOP","NOP","EOR","LSR","RMB","PHA","EOR","LSR","NOP","JMP","EOR","LSR","BBR",
      "BVC","EOR","EOR","NOP","NOP","EOR","LSR","RMB","CLI","EOR","PHY","NOP","NOP","EOR","LSR","BBR",
      "RTS","ADC","NOP","NOP","STZ","ADC","ROR","RMB","PLA","ADC","ROR","NOP","JMP","ADC","ROR","BBR",
      "BVS","ADC","ADC","NOP","STZ","ADC","ROR","RMB","SEI","ADC","PLY","NOP","JMP","ADC","ROR","BBR",
      "BRA","STA","NOP","NOP","STY","STA","STX","SMB","DEY","BIT","TXA","NOP","STY","STA","STX","BBS",
      "BCC","STA","STA","NOP","STY","STA","STX","SMB","TYA","STA","TXS","NOP","STZ","STA","STZ","BBS",
      "LDY","LDA","LDX","NOP","LDY","LDA","LDX","SMB","TAY","LDA","TAX","NOP","LDY","LDA","LDX","BBS",
      "BCS","LDA","LDA","NOP","LDY","LDA","LDX","SMB","CLV","LDA","TSX","NOP","LDY","LDA","LDX","BBS",
      "CPY","CMP","NOP","NOP","CPY","CMP","DEC","SMB","INY","CMP","DEX","WAI","CPY","CMP","DEC","BBS",
      "BNE","CMP","CMP","NOP","NOP","CMP","DEC","SMB","CLD","CMP","PHX","STP","NOP","CMP","DEC","BBS",
      "CPX","SBC","NOP","NOP","CPX","SBC","INC","SMB","INX","SBC","NOP","NOP","CPX","SBC","INC","BBS",
      "BEQ","SBC","SBC","NOP","NOP","SBC","INC","SMB","SED","SBC","PLX","NOP","NOP","SBC","INC","BBS"
    };

    static const int am[256] = {
       0x0 , 0xC , 0x2 , 0x0 , 0x3 , 0x3 , 0x3 , 0x3 , 0x0 , 0x2 , 0x1 , 0x0 , 0x7 , 0x7 , 0x7 , 0xE ,
       0x6 , 0xD , 0xB , 0x0 , 0x3 , 0x4 , 0x4 , 0x3 , 0x0 , 0x9 , 0x1 , 0x0 , 0x7 , 0x8 , 0x8 , 0xE ,
       0x7 , 0xC , 0x2 , 0x0 , 0x3 , 0x3 , 0x3 , 0x3 , 0x0 , 0x2 , 0x1 , 0x0 , 0x7 , 0x7 , 0x7 , 0xE ,
       0x6 , 0xD , 0xB , 0x0 , 0x4 , 0x4 , 0x4 , 0x3 , 0x0 , 0x9 , 0x1 , 0x0 , 0x8 , 0x8 , 0x8 , 0xE ,
       0x0 , 0xC , 0x2 , 0x0 , 0x3 , 0x3 , 0x3 , 0x3 , 0x0 , 0x2 , 0x1 , 0x0 , 0x7 , 0x7 , 0x7 , 0xE ,
       0x6 , 0xD , 0xB , 0x0 , 0x4 , 0x4 , 0x4 , 0x3 , 0x0 , 0x9 , 0x0 , 0x0 , 0x7 , 0x8 , 0x8 , 0xE ,
       0x0 , 0xC , 0x2 , 0x0 , 0x3 , 0x3 , 0x3 , 0x3 , 0x0 , 0x2 , 0x1 , 0x0 , 0xA , 0x7 , 0x7 , 0xE ,
       0x6 , 0xD , 0xB , 0x0 , 0x4 , 0x4 , 0x4 , 0x3 , 0x0 , 0x9 , 0x0 , 0x0 , 0xF , 0x8 , 0x8 , 0xE ,
       0x6 , 0xC , 0x2 , 0x0 , 0x3 , 0x3 , 0x3 , 0x3 , 0x0 , 0x2 , 0x0 , 0x0 , 0x7 , 0x7 , 0x7 , 0xE ,
       0x6 , 0xD , 0xB , 0x0 , 0x4 , 0x4 , 0x5 , 0x3 , 0x0 , 0x9 , 0x0 , 0x0 , 0x7 , 0x8 , 0x8 , 0xE ,
       0x2 , 0xC , 0x2 , 0x0 , 0x3 , 0x3 , 0x3 , 0x3 , 0x0 , 0x2 , 0x0 , 0x0 , 0x7 , 0x7 , 0x7 , 0xE ,
       0x6 , 0xD , 0xB , 0x0 , 0x4 , 0x4 , 0x5 , 0x3 , 0x0 , 0x9 , 0x0 , 0x0 , 0x8 , 0x8 , 0x9 , 0xE ,
       0x2 , 0xC , 0x2 , 0x0 , 0x3 , 0x3 , 0x3 , 0x3 , 0x0 , 0x2 , 0x0 , 0x0 , 0x7 , 0x7 , 0x7 , 0xE ,
       0x6 , 0xD , 0xB , 0x0 , 0x4 , 0x4 , 0x4 , 0x3 , 0x0 , 0x9 , 0x0 , 0x0 , 0x7 , 0x8 , 0x8 , 0xE ,
       0x2 , 0xC , 0x2 , 0x0 , 0x3 , 0x3 , 0x3 , 0x3 , 0x0 , 0x2 , 0x0 , 0x0 , 0x7 , 0x7 , 0x7 , 0xE ,
       0x6 , 0xD , 0xB , 0x0 , 0x4 , 0x4 , 0x4 , 0x3 , 0x0 , 0x9 , 0x0 , 0x0 , 0x7 , 0x8 , 0x8 , 0xE
     };

    void puce65c02::dasm(uint16_t address) {  // TODO : give the bit# for BBR, BSS, RMB and SMB
      uint8_t op = readMem(address);
      uint8_t b1 = readMem((address + 1) & 0xFFFF);
      uint8_t b2 = readMem((address + 2) & 0xFFFF);
      printf("%04X %02X ", address, op);
      switch(am[op]) {
        case 0x0: printf("       %s          ",              mn[op]      ); break;  // implied
        case 0x1: printf("       %s A        ",              mn[op]      ); break;  // accumulator
        case 0x2: printf("%02X     %s #$%02X     ",    b1,   mn[op],b1   ); break;  // immediate
        case 0x3: printf("%02X     %s $%02X      ",    b1,   mn[op],b1   ); break;  // zero page
        case 0x4: printf("%02X     %s $%02X,X    ",    b1,   mn[op],b1   ); break;  // zero page, X indexed
        case 0x5: printf("%02X     %s $%02X,Y    ",    b1,   mn[op],b1   ); break;  // zero page, Y indexed
        case 0x6: printf("%02X     %s $%02X      ",    b1,   mn[op],b1   ); break;  // relative
        case 0xB: printf("%02X     %s ($%02X)    ",    b1,   mn[op],b1   ); break;  // izp ($00)
        case 0xC: printf("%02X     %s ($%02X,X)  ",    b1,   mn[op],b1   ); break;  // X indexed, indirect
        case 0xD: printf("%02X     %s ($%02X),Y  ",    b1,   mn[op],b1   ); break;  // indirect, Y indexed
        case 0x7: printf("%02X%02X   %s $%02X%02X    ",b1,b2,mn[op],b2,b1); break;  // absolute
        case 0x8: printf("%02X%02X   %s $%02X%02X,X  ",b1,b2,mn[op],b2,b1); break;  // absolute, X indexed
        case 0x9: printf("%02X%02X   %s $%02X%02X,Y  ",b1,b2,mn[op],b2,b1); break;  // absolute, Y indexed
        case 0xA: printf("%02X%02X   %s ($%02X%02X)  ",b1,b2,mn[op],b2,b1); break;  // indirect
        case 0xF: printf("%02X%02X   %s ($%02X%02X,X)",b1,b2,mn[op],b2,b1); break;  // iax ($0000,X)
        case 0xE: printf("%02X%02X   %s $%02X,$%02X  ",b1,b2,mn[op],b2,b1); break;  // zpr $00,$00
      }
      return;
    }


    int puce65c02::getCode(uint16_t address, char* buffer, int size, int numLines) {
      int consumed = 0;
      int remaining = size;

      for (int i=0; i<numLines; i++) {
        uint8_t op = readMem(address);
        uint8_t b1 = readMem((address + 1) & 0xFFFF);
        uint8_t b2 = readMem((address + 2) & 0xFFFF);
        consumed += snprintf(buffer + consumed, remaining - consumed, "%04X %02X ", address, op);
        switch(am[op]) {
          case 0x0: consumed += snprintf(buffer + consumed, remaining - consumed, "       %s          ",              mn[op]      ); address+=1; break;  // implied
          case 0x1: consumed += snprintf(buffer + consumed, remaining - consumed, "       %s A        ",              mn[op]      ); address+=1; break;  // accumulator
          case 0x2: consumed += snprintf(buffer + consumed, remaining - consumed, "%02X     %s #$%02X     ",    b1,   mn[op],b1   ); address+=2; break;  // immediate
          case 0x3: consumed += snprintf(buffer + consumed, remaining - consumed, "%02X     %s $%02X      ",    b1,   mn[op],b1   ); address+=2; break;  // zero page
          case 0x4: consumed += snprintf(buffer + consumed, remaining - consumed, "%02X     %s $%02X,X    ",    b1,   mn[op],b1   ); address+=2; break;  // zero page, X indexed
          case 0x5: consumed += snprintf(buffer + consumed, remaining - consumed, "%02X     %s $%02X,Y    ",    b1,   mn[op],b1   ); address+=2; break;  // zero page, Y indexed
          case 0x6: consumed += snprintf(buffer + consumed, remaining - consumed, "%02X     %s $%02X      ",    b1,   mn[op],b1   ); address+=2; break;  // relative
          case 0xB: consumed += snprintf(buffer + consumed, remaining - consumed, "%02X     %s ($%02X)    ",    b1,   mn[op],b1   ); address+=2; break;  // izp ($00)
          case 0xC: consumed += snprintf(buffer + consumed, remaining - consumed, "%02X     %s ($%02X,X)  ",    b1,   mn[op],b1   ); address+=2; break;  // X indexed, indirect
          case 0xD: consumed += snprintf(buffer + consumed, remaining - consumed, "%02X     %s ($%02X),Y  ",    b1,   mn[op],b1   ); address+=2; break;  // indirect, Y indexed
          case 0x7: consumed += snprintf(buffer + consumed, remaining - consumed, "%02X%02X   %s $%02X%02X    ",b1,b2,mn[op],b2,b1); address+=3; break;  // absolute
          case 0x8: consumed += snprintf(buffer + consumed, remaining - consumed, "%02X%02X   %s $%02X%02X,X  ",b1,b2,mn[op],b2,b1); address+=3; break;  // absolute, X indexed
          case 0x9: consumed += snprintf(buffer + consumed, remaining - consumed, "%02X%02X   %s $%02X%02X,Y  ",b1,b2,mn[op],b2,b1); address+=3; break;  // absolute, Y indexed
          case 0xA: consumed += snprintf(buffer + consumed, remaining - consumed, "%02X%02X   %s ($%02X%02X)  ",b1,b2,mn[op],b2,b1); address+=3; break;  // indirect
          case 0xF: consumed += snprintf(buffer + consumed, remaining - consumed, "%02X%02X   %s ($%02X%02X,X)",b1,b2,mn[op],b2,b1); address+=3; break;  // iax ($0000,X)
          case 0xE: consumed += snprintf(buffer + consumed, remaining - consumed, "%02X%02X   %s $%02X,$%02X  ",b1,b2,mn[op],b2,b1); address+=3; break;  // zpr $00,$00
        }
        consumed += snprintf(buffer + consumed, remaining - consumed, "\n");
      }
      return consumed;
    }


    void puce65c02::printRegs() {
      printf("A=%02X  X=%02X  Y=%02X  S=%02X  *S=%02X  %c%c%c%c%c%c%c%c", \
      A, X, Y, SP, readMem(0x100 + SP), \
      P.S?'N':'-', P.V?'V':'-', P.U?'U':'.', P.B?'B':'-', \
      P.D?'D':'-', P.I?'I':'-', P.Z?'Z':'-', P.C?'C':'-');
    }


    int puce65c02::getRegs(char* buffer) {
      return (snprintf(buffer, 100, "A=%02X  X=%02X  Y=%02X  S=%02X  *S=%02X\nPC=%04X  P=%c%c%c%c%c%c%c%c", \
      A, X, Y, SP, readMem(0x100 + SP), PC, \
      P.S?'N':'-', P.V?'V':'-', P.U?'U':'.', P.B?'B':'-', \
      P.D?'D':'-', P.I?'I':'-', P.Z?'Z':'-', P.C?'C':'-'));
    }





    int main(int argc, char* argv[]) {

#if TIMING

      const char *filename = "timingtest-1.bin";
      FILE *f = fopen(filename, "rb");
      if (!f || fread(RAM + 0x1000, 1, 620, f) != 620) {
        printf("ERROR : can't load %s\n", filename);
        return(0);
      }
      fclose(f);

      puce65c02 *cpu = new puce65c02(0x1000);  // instantiate, reset the cpu and set PC to 0x1000
      cpu->ticks = 0;  // we won't measure the 7 ticks the RESET took above
      unsigned long long int oldticks = cpu->ticks;

      do {
        cpu->dasm(cpu->PC);
        printf("   ");
        cpu->exec(1);
        cpu->printRegs();
        printf("   Cycles: %llu Total: %llu\n", cpu->ticks - oldticks, cpu->ticks);

        oldticks = cpu->ticks;
      }  while(cpu->ticks < 1143);

#else
      // Running the two functionnal tests :
      const char *filename = "6502_functional_test.bin";
      FILE *f = fopen(filename, "rb");
      if (!f || fread(RAM, 1, 65536, f) != 65536) {
        printf("ERROR : can't load Klaus Dormann's functonnal tests binary file %s\n", filename);
        return(0);
      }
      fclose(f);

      puce65c02 *cpu = new puce65c02(0x400);  // instantiate, reset the cpu and set PC to 0x400

#if VERBOSE
      unsigned long long int oldticks = cpu->ticks;
#endif

      uint16_t oldPC = 0x400, newPC = 0x400;  // to detect loops (BNE $FE) when error occurs

      while(newPC != 0x3469) {

#if VERBOSE
        cpu->dasm(newPC);
        printf("  ");
#endif

        newPC = cpu->exec(1);

#if VERBOSE
        cpu->printRegs();
        printf("   Cycles: %llu Total: %llu\n", cpu->ticks - oldticks, cpu->ticks);
        oldticks = cpu->ticks;
#endif

        if (newPC == oldPC ) {
          printf("\n\nLoop detected @ %04X - Press ENTER to proceed with next test or CTRL<C> to abort\n\n", newPC);
          getchar();
          cpu->setPC(newPC + 2);  // step out the loop
        }
        oldPC = newPC;
      }
      printf("\nSUCCESS : Reached end of 6502_functional_test @ 0x3469 after %llu clock cycles\n", cpu->ticks);


      const char *filename2 = "65C02_extended_opcodes_test.bin";
      f = fopen(filename2, "rb");
      if (!f || fread(RAM, 1, 65536, f) != 65536) {
        printf("ERROR : can't load 65c02 Klaus Dormann's functonnal tests binary file %s\n", filename2);
        return(0);
      }
      fclose(f);

      cpu->RST();
      cpu->setPC(0x400);
      oldPC = newPC = 0x400;
#if VERBOSE
      oldticks = cpu->ticks;
#endif

      while(newPC != 0x24f1) {
#if VERBOSE
        cpu->dasm(newPC);
        printf("  ");
#endif
        newPC = cpu->exec(1);
#if VERBOSE
        cpu->printRegs();
        printf("   Cycles: %llu Total: %llu\n", cpu->ticks - oldticks, cpu->ticks);
        oldticks = cpu->ticks;
#endif
        if (newPC == oldPC ) {
          printf("\n\nLoop detected @ %04X - Press ENTER to proceed with next test or CTRL<C> to abort\n\n", newPC);
          getchar();
          cpu->setPC(newPC + 2);  // step out the loop
        }
        oldPC = newPC;
      }
      printf("\nSUCCESS : Reached end of 65C02_extended_opcodes_test @ 0x24f1 after %llu clock cycles\n", cpu->ticks);

#endif
      return(0);
    }
#endif
