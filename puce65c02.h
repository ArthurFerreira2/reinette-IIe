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


#ifndef _PUCE65C02_H
#define _PUCE65C02_H

#include <cstdint>

typedef enum {run, step, stop, wait} status;

#define CARRY 0x01
#define ZERO  0x02
#define INTR  0x04
#define DECIM 0x08
#define BREAK 0x10
#define UNDEF 0x20
#define OFLOW 0x40
#define SIGN  0x80


class puce65c02 {
public:

  unsigned long long int ticks;
// private:
  status state;
  uint16_t PC;         //  Program Counter
  uint8_t A, X, Y, SP; // Accumulator, X and y indexes and Stack Pointer
  union {
    uint8_t byte;
    struct {
      uint8_t C : 1;          // Carry
      uint8_t Z : 1;          // Zero
      uint8_t I : 1;          // Interupt disabled
      uint8_t D : 1;          // Decimal
      uint8_t B : 1;          // Break
      uint8_t U : 1;          // Undefined
      uint8_t V : 1;          // Overflow
      uint8_t S : 1;          // Sign
    };
  } P;                        // Processor Status

public:
  puce65c02()  { ticks = 0; RST(); }
  puce65c02(uint16_t address) { ticks = 0; RST(); PC = address; }
  ~puce65c02();

  uint16_t getPC() { return PC; };
  void setPC(uint16_t address) { PC = address; };

  void RST();
  void IRQ();
  void NMI();
  uint16_t exec(unsigned long long int cycleCount);

#if __TESTS__
  void dasm(uint16_t address);
  void printRegs();
  int getRegs(char* buffer);
  int getCode(uint16_t address, char* buffer, int size, int numLines);
#endif

};

#endif
