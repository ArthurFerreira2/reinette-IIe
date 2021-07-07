/*
 * reinette, a french Apple II emulator, using SDL2
 * and powered by puce65c02 - a WDS 65c02 cpu emulator by the same author
 * Last modified 1st of July 2021
 * Copyright (c) 2021 Arthur Ferreira (arthur.ferreira2@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _MEMORY_H
#define _MEMORY_H

// memory layout
#define RAMSIZE  0xC000  // 48K
#define AUXSIZE  0xC000  // 48K

// Apple II and II+
// #define ROMSTART 0xD000
// #define ROMSIZE  0x3000  // 12K

// Apple IIe and IIee
#define ROMSTART 0xC000
#define ROMSIZE  0x4000  // 16K

// language card
#define LGCSTART 0xD000
#define LGCSIZE  0x3000
#define BK2START 0xD000
#define BK2SIZE  0x1000

// slot 1,
#define SL1START 0xC100
#define SL1SIZE  0x0100
// slot 2
#define SL2START 0xC200
#define SL2SIZE  0x0100
// slot 3
#define SL3START 0xC300
#define SL3SIZE  0x0100
// slot 4
#define SL4START 0xC400
#define SL4SIZE  0x0100
// slot 5
#define SL5START 0xC500
#define SL5SIZE  0x0100
// slot 6, diskII
#define SL6START 0xC600
#define SL6SIZE  0x0100
// slot 7
#define SL7START 0xC700
#define SL7SIZE  0x0100



#define SLROMSTART 0xC800        // peripheral-card expansion ROMs -
#define SLROMSIZE 0xC800


class Mmu {
public:
  uint8_t ram[RAMSIZE];          // 48K of MAIN in $000-$BFFF
  uint8_t ramlgc[LGCSIZE];       // MAIN Language Card 12K in $D000-$FFFF
  uint8_t rambk2[BK2SIZE];       // MAIN bank 2 of Language Card 4K in $D000-$DFFF

  uint8_t aux[AUXSIZE];          // 48K of AUX memory
  uint8_t auxlgc[LGCSIZE];       // AUX Language Card 12K in $D000-$FFFF
  uint8_t auxbk2[BK2SIZE];       // AUX bank 2 of Language Card 4K in $D000-$DFFF

  uint8_t rom[ROMSIZE];          // 16K of rom in $C000-$FFFF

  uint8_t sl1[SL1SIZE];
  uint8_t sl2[SL2SIZE];
  uint8_t sl3[SL3SIZE];
  uint8_t sl5[SL5SIZE];
  uint8_t sl4[SL4SIZE];
  uint8_t sl6[SL6SIZE];          // P5A disk ][ PROM in slot 6
  uint8_t sl7[SL7SIZE];
  uint8_t slrom[8][SLROMSIZE];   // 7x peripheral-card expansion ROMs ( index 0 is not used)

  uint8_t KBD;                   // $C000, $C010 ascii value of keyboard input

  bool PAGE2;                    // $C054 PAGE1    / $C055 PAGE2
  bool TEXT;                     // $C050 CLRTEXT  / $C051 SETTEXT
  bool MIXED;                    // $C052 CLRMIXED / $C053 SETMIXED
  bool HIRES;                    // $C056 GR       / $C057 HGR
  bool DHIRES;                   // 0xC05E / 0xC05F DOUBLE HIRES
  bool COL80;                    // 80 COLUMNS
  bool ALTCHARSET;

  bool LCWR ;                    // Language Card writable
  bool LCRD ;                    // Language Card readable
  bool LCBK2;                    // Language Card bank 2 enabled
  bool LCWFF;                    // Language Card pre-write flip flop

  bool AN0;
  bool AN1;
  bool AN2;
  bool AN3;

  bool RAMRD;
  bool RAMWRT;
  bool ALTZP;
  bool STORE80;
  bool INTCXROM;
  bool SLOTC3ROM;
  bool IOUDIS;
  bool VERTBLANK;

  Mmu();
  ~Mmu();
  void init();
  uint8_t readMem(uint16_t address);
  void writeMem(uint16_t address, uint8_t value);

private:
  uint8_t softSwitches(uint16_t address, uint8_t value, bool WRT);
};

#endif
