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

// code is not iso compliant : abuse of range expressions in switch statements

#include <stdio.h>
#include <stdlib.h>
#include "reinette.h"

//========================================== MEMORY MAPPED SOFT SWITCHES HANDLER

uint8_t Mmu::softSwitches(uint16_t address, uint8_t value, bool WRT) {
  static uint8_t dLatch = 0;                                                    // disk ][ I/O register

  switch (address) {

    // MEMORY MANAGEMENT (and KEYBOARD)
    case 0xC000: if (WRT) STORE80    = false; else return KBD; break;           // cause PAGE2 on to select AUX -- KEYBOARD return key code - if hi-bit is set the key code (7 lo-bits) is valid
    case 0xC001: if (WRT) STORE80    = true;  break;                            // allow PAGE2 to switch MAIN / AUX
    case 0xC002: if (WRT) RAMRD      = false; break;                            // read from MAIN
    case 0xC003: if (WRT) RAMRD      = true;  break;                            // read from AUX
    case 0xC004: if (WRT) RAMWRT     = false; break;                            // write to MAIN
    case 0xC005: if (WRT) RAMWRT     = true;  break;                            // write to AUX
    case 0xC006: if (WRT) INTCXROM   = false; break;                            // set peripheral roms for peripherals ($C100-$CFFF)
    case 0xC007: if (WRT) INTCXROM   = true;  break;                            // set internal rom for peripherals ($C100-$CFFF)
    case 0xC008: if (WRT) ALTZP      = false; break;                            // MAIN stack & rero page
    case 0xC009: if (WRT) ALTZP      = true;  break;                            // AUX stack & rero page
    case 0xC00A: if (WRT) SLOTC3ROM  = false; break;                            // ROM in Slot 3
    case 0xC00B: if (WRT) SLOTC3ROM  = true;  break;                            // ROM in AUX Slot
    case 0xC00C: if (WRT) COL80      = false; break;                            // 80 COL OFF -> 40 COL
    case 0xC00D: if (WRT) COL80      = true;  break;                            // 80 COL ON
    case 0xC00E: if (WRT) ALTCHARSET = false; break;                            // primary character set
    case 0xC00F: if (WRT) ALTCHARSET = true;  break;                            // alternate character set
    case 0xC010: KBD &= 0x7F;  return KBD;                                       // KBDSTROBE, clear hi-bit and return key code

    // SOFT SWITCH STATUS FLAGS
    case 0xC011: return (0x80 * LCBK2);
    case 0xC012: return (0x80 * LCRD);
    case 0xC013: return (0x80 * RAMRD);                                         // 0x80 if reads from AUX
    case 0xC014: return (0x80 * RAMWRT);                                        // 0x80 if writes from AUX
    case 0xC015: return (0x80 * INTCXROM);
    case 0xC016: return (0x80 * ALTZP);                                         // 0x80 if using stack and zero page from AUX
    case 0xC017: return (0x80 * SLOTC3ROM);
    case 0xC018: return (0x80 * STORE80);                                       // do we store 80 col page 2 on MAIN or AUX
    case 0xC019: return (0x80 * VERTBLANK);

    case 0xC01A: return (0x80 * TEXT);                                          // read text switch
    case 0xC01B: return (0x80 * MIXED);                                         // read mixed switch
    case 0xC01C: return (0x80 * PAGE2);                                         // read page 2 switch
    case 0xC01D: return (0x80 * HIRES);                                         // read HiRes switch
    case 0xC01E: return (0x80 * ALTCHARSET);                                    // alternate character set ?
    case 0xC01F: return (0x80 * COL80);                                         // 80 columns on ?

    // SOUND
    case 0xC020:                                                                // TAPEOUT (shall we listen it ? - try SAVE from applesoft)
    case 0xC030:                                                                // SPEAKER
    case 0xC033: speaker->play(); break;                                        // apple invader uses $C033 to output sound !

    // GAME I/O STROBE OUT
    case 0xC040: break;

    // VIDEO MODES
    case 0xC050: TEXT = false;  video->clearCache(); break;                     // set Graphics
    case 0xC051: TEXT = true;   video->clearCache(); break;                     // set Text
    case 0xC052: MIXED = false; video->clearCache(); break;                     // set Mixed to off
    case 0xC053: MIXED = true;  video->clearCache(); break;                     // set Mixed to on
    case 0xC054: PAGE2 = false; video->clearCache(); break;                     // select page 1
    case 0xC055: PAGE2 = true;  video->clearCache(); break;                     // select page 2
    case 0xC056: HIRES = false; video->clearCache(); break;                     // set HiRes to off
    case 0xC057: HIRES = true;  video->clearCache(); break;                     // set HiRes to on

    // ANNUNCIATORS
    case 0xC058: if (!IOUDIS) AN0 = false; break;                               // If IOUDIS off: Annunciator 0 Off
    case 0xC059: if (!IOUDIS) AN0 = true;  break;                               // If IOUDIS off: Annunciator 0 On
    case 0xC05A: if (!IOUDIS) AN1 = false; break;                               // If IOUDIS off: Annunciator 1 Off
    case 0xC05B: if (!IOUDIS) AN1 = true;  break;                               // If IOUDIS off: Annunciator 1 On
    case 0xC05C: if (!IOUDIS) AN2 = false; break;                               // If IOUDIS off: Annunciator 2 Off
    case 0xC05D: if (!IOUDIS) AN2 = true;  break;                               // If IOUDIS off: Annunciator 2 On
    case 0xC05E: if (!IOUDIS) AN3 = false; DHIRES = true;  break;               // If IOUDIS off: Annunciator 3 Off
    case 0xC05F: if (!IOUDIS) AN3 = true;  DHIRES = false; break;               // If IOUDIS off: Annunciator 2 On

    // TAPE
    case 0xC060: break;                                                         // TAPE IN

    // PADDLES
    case 0xC061: return paddles->PB0;                                           // Push Button 0 / Open Apple
    case 0xC062: return paddles->PB1;                                           // Push Button 1 / Solid Apple
    case 0xC063: return paddles->PB2;                                           // Push Button 2 / Shift
    case 0xC064: return paddles->read(0);                                       // Paddle 0
    case 0xC065: return paddles->read(1);                                       // Paddle 1
    case 0xC066: return paddles->read(2);                                       // Paddle 2
    case 0xC067: return paddles->read(3);                                       // Paddle 3
    case 0xC070: paddles->reset(); break;                                       // paddles timer reset

    // IOUDIS
    case 0xC07E: if (WRT) IOUDIS = false; else return (0x80 * IOUDIS); break;
    case 0xC07F: if (WRT) IOUDIS = true;  else return (0x80 * DHIRES); break;

    // LANGUAGE CARD (used with MAIN and AUX)
    case 0xC080:
    case 0xC084: LCBK2 = 1; LCRD = 1; LCWR = 0;      LCWFF = 0;    break;       // LC2RD
    case 0xC081:
    case 0xC085: LCBK2 = 1; LCRD = 0; LCWR |= LCWFF; LCWFF = !WRT; break;       // LC2WR
    case 0xC082:
    case 0xC086: LCBK2 = 1; LCRD = 0; LCWR = 0;      LCWFF = 0;    break;       // ROMONLY2
    case 0xC083:
    case 0xC087: LCBK2 = 1; LCRD = 1; LCWR |= LCWFF; LCWFF = !WRT; break;       // LC2RW
    case 0xC088:
    case 0xC08C: LCBK2 = 0; LCRD = 1; LCWR = 0;      LCWFF = 0;    break;       // LC1RD
    case 0xC089:
    case 0xC08D: LCBK2 = 0; LCRD = 0; LCWR |= LCWFF; LCWFF = !WRT; break;       // LC1WR
    case 0xC08A:
    case 0xC08E: LCBK2 = 0; LCRD = 0; LCWR = 0;      LCWFF = 0;    break;       // ROMONLY1
    case 0xC08B:
    case 0xC08F: LCBK2 = 0; LCRD = 1; LCWR |= LCWFF; LCWFF = !WRT; break;       // LC1RW

    // SLOT 1
    case 0xC090 ... 0xC09F: break;

    // SLOT 2
    case 0xC0A0 ... 0xC0AF: break;

    // SLOT 3
    case 0xC0B0 ... 0xC0BF: break;

    // SLOT 4
    case 0xC0C0 ... 0xC0CF: break;

    // SLOT 5
    case 0xC0D0 ... 0xC0DF: break;

    // SLOT 6 DISK ][
    case 0xC0E0 ... 0xC0E7: disk->stepMotor(address); break;                    // MOVE DRIVE HEAD
    case 0xC0E8: disk->unit[disk->curDrv].motorOn = false; break;               // MOTOROFF
    case 0xC0E9: disk->unit[disk->curDrv].motorOn = true; break;                // MOTORON
    case 0xC0EA: disk->setDrv(0); break;                                        // DRIVE0EN
    case 0xC0EB: disk->setDrv(1); break;                                        // DRIVE1EN
    case 0xC0EC:                                                                // Shift Data Latch
      if (disk->unit[disk->curDrv].writeMode)                                   // writting
        disk->unit[disk->curDrv].data[disk->unit[disk->curDrv].track * 0x1A00 + disk->unit[disk->curDrv].nibble] = dLatch;  // good luck gcc
      else                                                                      // reading
        dLatch = disk->unit[disk->curDrv].data[disk->unit[disk->curDrv].track * 0x1A00 + disk->unit[disk->curDrv].nibble];  // easy peasy
      disk->unit[disk->curDrv].nibble = (disk->unit[disk->curDrv].nibble + 1) % 0x1A00;  // turn floppy of 1 nibble
      return dLatch;
    case 0xC0ED: dLatch = value; break;                                         // Load Data Latch
    case 0xC0EE:                                                                // latch for READ
      disk->unit[disk->curDrv].writeMode = false;
      return disk->unit[disk->curDrv].readOnly ? 0x80 : 0;                      // check protection
    case 0xC0EF: disk->unit[disk->curDrv].writeMode = true; break;              // latch for WRITE

    // SLOT 7
    case 0xC0F0 ... 0xC0FF: break;

  }
  return cpu->ticks%256;                                                        // catch all, gives a floating value
}

//================================================================== MEMORY READ

uint8_t Mmu::readMem(uint16_t address) {

  switch (address) {
    case 0x0000 ... 0x01FF:                                                     // STACK and Zero Page in AUX or MAIN
      if (ALTZP) {
        video->auxHeatmap[address] |= 0xFF00FF00;
        return aux[address];
      }
      video->ramHeatmap[address] |= 0xFF00FF00;
      return ram[address];
    break;

    case 0x0200 ... 0x03FF:                                                     // MAIN or AUX
      if (RAMRD) {
        video->auxHeatmap[address] |= 0xFF00FF00;
        return aux[address];
      }
      video->ramHeatmap[address] |= 0xFF00FF00;
      return ram[address];
    break;

    case 0x0400 ... 0x07FF:                                                     // TEXT PAGE 1 in AUX or MAIN
      if (STORE80) {
         if (PAGE2) {
          video->auxHeatmap[address] |= 0xFF00FF00;
          return aux[address];
        }
        video->ramHeatmap[address] |= 0xFF00FF00;
        return ram[address];
      }
      if (RAMRD) {
        video->auxHeatmap[address] |= 0xFF00FF00;
        return aux[address];
      }
      video->ramHeatmap[address] |= 0xFF00FF00;
      return ram[address];
    break;

    case 0x0800 ... 0x0BFF:                                                     // TEXT PAGE 2 in AUX or MAIN
      if (RAMRD) {
        video->auxHeatmap[address] |= 0xFF00FF00;
        return aux[address];
      }
      video->ramHeatmap[address] |= 0xFF00FF00;
      return ram[address];
    break;

    case 0x0C00 ... 0x1FFF:                                                     // AUX or MAIN
      if (RAMRD) {
        video->auxHeatmap[address] |= 0xFF00FF00;
        return aux[address];
      }
      video->ramHeatmap[address] |= 0xFF00FF00;
      return ram[address];
    break;

    case 0x2000 ... 0x3FFF:                                                     // HIRES PAGE 1
      if (STORE80) {
        if (PAGE2 && HIRES) {
          video->auxHeatmap[address] |= 0xFF00FF00;
          return aux[address];
        }
        video->ramHeatmap[address] |= 0xFF00FF00;
        return ram[address];
      }
      if (RAMRD) {
        video->auxHeatmap[address] |= 0xFF00FF00;
        return aux[address];
      }
      video->ramHeatmap[address] |= 0xFF00FF00;
      return ram[address];
    break;

    case 0x4000 ... 0x5FFF:                                                     // HIRES PAGE 2 in AUX or MAIN
      if (RAMRD) {
        video->auxHeatmap[address] |= 0xFF00FF00;
        return aux[address];
      }
      video->ramHeatmap[address] |= 0xFF00FF00;
      return ram[address];
    break;

    case 0x6000 ... 0xBFFF:                                                     // AUX or MAIN
      if (RAMRD) {
        video->auxHeatmap[address] |= 0xFF00FF00;
        return aux[address];
      }
      video->ramHeatmap[address] |= 0xFF00FF00;
      return ram[address];
    break;

    case 0xC000 ... 0xC0FF:                                                     // SOFT SWITCHES
      video->ramHeatmap[address] |= 0xFF00FF00;
      return softSwitches(address, 0, false);
    break;

    case 0xC100 ... 0xC1FF:                                                     // SLOT 1 ROM or ROM
      video->ramHeatmap[address] |= 0xFF00FF00;
      if (INTCXROM) {
        return rom[address - ROMSTART];
      }
      return sl1[address - SL1START];
    break;

    case 0xC200 ... 0xC2FF:                                                     // SLOT 2 ROM or ROM
      video->ramHeatmap[address] |= 0xFF00FF00;
      if (INTCXROM) {
        return rom[address - ROMSTART];
      }
      return sl2[address - SL2START];
    break;

    case 0xC300 ... 0xC3FF:                                                     // SLOT 3 ROM  or ROM : video
      video->ramHeatmap[address] |= 0xFF00FF00;
      if (INTCXROM || !SLOTC3ROM) {
        return rom[address - ROMSTART];
      }
      return sl3[address - SL3START];
    break;

    case 0xC400 ... 0xC4FF:                                                     // SLOT 4 ROM or ROM
      video->ramHeatmap[address] |= 0xFF00FF00;
      if (INTCXROM) {
        return rom[address - ROMSTART];
      }
      return sl4[address - SL4START];
    break;

    case 0xC500 ... 0xC5FF:                                                     // SLOT 5 ROM or ROM
      video->ramHeatmap[address] |= 0xFF00FF00;
      if (INTCXROM) {
        return rom[address - ROMSTART];
      }
      return sl5[address - SL5START];
    break;

    case 0xC600 ... 0xC6FF:                                                     // SLOT 6 ROM : DISK ][ or ROM
      video->ramHeatmap[address] |= 0xFF00FF00;
      if (INTCXROM) {
        return rom[address - ROMSTART];
      }
      return sl6[address - SL6START];
    break;

    case 0xC700 ... 0xC7FF:                                                     // SLOT 7 ROM or ROM
      video->ramHeatmap[address] |= 0xFF00FF00;
      if (INTCXROM) {
        return rom[address - ROMSTART];
      }
      return sl7[address - SL7START];
    break;

    case 0xC800 ... 0xCFFE:                                                     // SHARED EXANSION SLOTS ROM AREA or ROM
      video->ramHeatmap[address] |= 0xFF00FF00;
      if (INTCXROM || !SLOTC3ROM) {
        return rom[address - ROMSTART];
      }
      return slrom[(address & 0x0F00) >> 2 & 0xF][address - SLROMSTART];
    break;

    case 0xCFFF:  // turn off all slots expansion ROMs - TODO : NEEDS REWORK
      disk->unit[disk->curDrv].motorOn = false;
      return 0;
    break;

    case 0xD000 ... 0xDFFF:                                                     // ROM, MAIN-BK1, RAM-BK2, AUX-BK1 or AUX-BK2
      if (LCRD) {
        if (LCBK2) {
          if (ALTZP) {
            video->auxHeatmap[address] |= 0xFF00FF00;
            return auxbk2[address - BK2START];                                  // AUX Bank 2
          }
          video->ramHeatmap[address] |= 0xFF00FF00;
          return rambk2[address - BK2START];                                    // MAIN Bank 2
        }
        if (ALTZP) {
          video->auxHeatmap[address] |= 0xFF00FF00;
          return auxlgc[address - LGCSTART];                                    // AUX Bank 1
        }
        video->ramHeatmap[address] |= 0xFF00FF00;
        return ramlgc[address - LGCSTART];                                      // MAIN Bank 1
      }
      video->ramHeatmap[address] |= 0xFF00FF00;
      return rom[address - ROMSTART];                                           // ROM
    break;

    case 0xE000 ... 0xFFFF:                                                     // ROM, MAIN-BK1  or AUX-BK1
      if (LCRD) {
        if (ALTZP) {
          video->auxHeatmap[address] |= 0xFF00FF00;
          return auxlgc[address - LGCSTART];                                    // AUX Bank 1
        }
        video->ramHeatmap[address] |= 0xFF00FF00;
        return ramlgc[address - LGCSTART];                                      // MAIN Bank 1
      }
      video->ramHeatmap[address] |= 0xFF00FF00;
      return rom[address - ROMSTART];                                           // ROM
    break;
  }
  return cpu->ticks%256;                                                        // returns a floating value
}


//================================================================= MEMORY WRITE

void Mmu::writeMem(uint16_t address, uint8_t value) {

  switch (address) {
    case 0x0000 ... 0x01FF:                                                     // STACK and Zero Page in AUX or MAIN
      if (ALTZP) {
        video->auxHeatmap[address] |= 0xFF0000FF;
        aux[address] = value;
        return;
      }
      video->ramHeatmap[address] |= 0xFF0000FF;
      ram[address] = value;
      return;
    break;

    case 0x0200 ... 0x03FF:                                                     // MAIN or AUX
      if (RAMWRT) {
        video->auxHeatmap[address] |= 0xFF0000FF;
        aux[address] = value;
        return;
      }
      video->ramHeatmap[address] |= 0xFF0000FF;
      ram[address] = value;
      return;
    break;

    case 0x0400 ... 0x07FF:                                                     // TEXT PAGE 1 in AUX or MAIN
      if (STORE80) {
        if (PAGE2) {
          video->auxHeatmap[address] |= 0xFF0000FF;
          aux[address] = value;
          return;
        }
        video->ramHeatmap[address] |= 0xFF0000FF;
        ram[address] = value;
        return;
      }
      if (RAMWRT) {
        video->auxHeatmap[address] |= 0xFF0000FF;
        aux[address] = value;
        return;
      }
      video->ramHeatmap[address] |= 0xFF0000FF;
      ram[address] = value;
      return;
    break;

    case 0x0800 ... 0x0BFF:                                                     // TEXT PAGE 2 in AUX or MAIN
      if (RAMWRT) {
        video->auxHeatmap[address] |= 0xFF0000FF;
        aux[address] = value;
        return;
      }
      video->ramHeatmap[address] |= 0xFF0000FF;
      ram[address] = value;
      return;
    break;

    case 0x0C00 ... 0x1FFF:                                                     // MAIN or AUX
      if (RAMWRT) {
        video->auxHeatmap[address] |= 0xFF0000FF;
        aux[address] = value;
        return;
      }
      video->ramHeatmap[address] |= 0xFF0000FF;
      ram[address] = value;
      return;
    break;

    case 0x2000 ... 0x3FFF:                                                     // HIRES PAGE 1
      if (STORE80) {
        if (PAGE2 && HIRES) {
          video->auxHeatmap[address] |= 0xFF0000FF;
          aux[address] = value;
          return;
        }
        video->ramHeatmap[address] |= 0xFF0000FF;
        ram[address] = value;
        return;
      }
      if (RAMWRT) {
        video->auxHeatmap[address] |= 0xFF0000FF;
        aux[address] = value;
        return;
      }
      video->ramHeatmap[address] |= 0xFF0000FF;
      ram[address] = value;
      return;
    break;

    case 0x4000 ... 0x5FFF:                                                     // HIRES PAGE 2 in AUX or MAIN
      if (RAMWRT) {
        video->auxHeatmap[address] |= 0xFF0000FF;
        aux[address] = value;
        return;
      }
      video->ramHeatmap[address] |= 0xFF0000FF;
      ram[address] = value;
      return;
    break;

    case 0x6000 ... 0xBFFF:                                                     // AUX or MAIN
      if (RAMWRT) {
        video->auxHeatmap[address] |= 0xFF0000FF;
        aux[address] = value;
        return;
      }
      video->ramHeatmap[address] |= 0xFF0000FF;
      ram[address] = value;
      return;
    break;

    case 0xC000 ... 0xC0FF:                                                     // softSwitches
      video->ramHeatmap[address] |= 0xFF0000FF;
      softSwitches(address, 0, true);
      return;
    break;

    case 0xC100 ... 0xCFFE:                                                     // readonly area
      return;
    break;

    case 0xCFFF:  // turn off all slots expansion ROMs - NEEDS REWORK soft switch ?
      disk->unit[disk->curDrv].motorOn = false;
      return;
    break;

    case 0xD000 ... 0xDFFF:                                                     // ROM, MAIN-BK1, RAM-BK2, AUX-BK1 or AUX-BK2
      if (LCWR) {
        if (LCBK2) {
          if (ALTZP) {
            video->auxHeatmap[address] |= 0xFF0000FF;
            auxbk2[address - BK2START] = value;                                 // AUX Bank 2
            return;
          }
          video->ramHeatmap[address] |= 0xFF0000FF;
          rambk2[address - BK2START] = value;                                   // MAIN Bank 2
          return;
        }
        if (ALTZP) {
          video->auxHeatmap[address] |= 0xFF0000FF;
          auxlgc[address - LGCSTART] = value;                                   // AUX Bank 1
          return;
        }
        video->ramHeatmap[address] |= 0xFF0000FF;
        ramlgc[address - LGCSTART] = value;                                     // MAIN Bank 1
        return;
      }
      video->ramHeatmap[address] |= 0xFF0000FF;
      return;
    break;

    case 0xE000 ... 0xFFFF:                                                     // ROM, MAIN-BK1  or AUX-BK1
      if (LCWR) {
        if (ALTZP) {
          video->auxHeatmap[address] |= 0xFF0000FF;
          auxlgc[address - LGCSTART] = value;                                   // AUX Bank 1
          return;
        }
        video->ramHeatmap[address] |= 0xFF0000FF;
        ramlgc[address - LGCSTART] = value;                                     // MAIN Bank 1
        return;
      }
      video->ramHeatmap[address] |= 0xFF0000FF;
      return;
    break;
  }
}


Mmu::Mmu() {
  // TODO : give user the choice of the rom version
  
  // pick one ROM file
  // FILE* f = fopen("rom/appleII.rom", "rb");                                  // load the Apple II ROM
  // FILE* f = fopen("rom/appleII+.rom", "rb");                                 // load the Apple II+ ROM
  // FILE* f = fopen("rom/appleIIe.rom", "rb");                                 // load the Apple IIe ROM
  FILE* f = fopen("rom/appleIIee.rom", "rb");                                   // load the Apple IIee ROM
  fread(rom, 1, ROMSIZE, f);
  fclose(f);

  // load DISK][ PROM
  f = fopen("rom/diskII.rom", "rb");                                            // load the P5A disk ][ PROM
  fread(sl6, 1, 256, f);
  fclose(f);

  init();
}


Mmu::~Mmu() {
  // release the memory arrays ?
}


void Mmu::init() {
  KBD = 0;                                                                      // $C000, $C010 ascii value of keyboard input
  PAGE2  = false;                                                               // $C054 PAGE1    / $C055 PAGE2
  TEXT  = true;                                                                 // $C050 CLRTEXT  / $C051 SETTEXT
  MIXED = false;                                                                // $C052 CLRMIXED / $C053 SETMIXED
  HIRES = false;                                                                // $C056 GR       / $C057 HGR
  DHIRES = false;                                                               // 0xC05E / 0xC05F DOUBLE HIRES
  COL80 = false;                                                                // 80 COLUMNS
  ALTCHARSET = false;
  LCWR  = true;                                                                 // Language Card writable
  LCRD  = false;                                                                // Language Card readable
  LCBK2 = true;                                                                 // Language Card bank 2 enabled
  LCWFF = false;                                                                // Language Card pre-write flip flop
  AN0 = false;
  AN1 = false;
  AN2 = true;
  AN3 = true;

  RAMRD = false;
  RAMWRT = false;
  ALTZP = false;
  STORE80 = false;

  INTCXROM = false;                                                             // use slots roms
  SLOTC3ROM = false;                                                            // use AUX Slot rom

  IOUDIS = false;
  VERTBLANK = true;

  memset(ram,    0, sizeof(ram));                                               // 48K of MAIN in $000-$BFFF
  memset(aux,    0, sizeof(aux));                                               // 48K of AUX memory
  memset(ramlgc, 0, sizeof(ramlgc));                                            // MAIN Language Card 12K in $D000-$FFFF
  memset(rambk2, 0, sizeof(rambk2));                                            // MAIN bank 2 of Language Card 4K in $D000-$DFFF
  memset(auxlgc, 0, sizeof(auxlgc));                                            // AUX Language Card 12K in $D000-$FFFF
  memset(auxbk2, 0, sizeof(auxbk2));                                            // AUX bank 2 of Language Card 4K in $D000-$DFFF

  // dirty hacks - fix when I know why
  ram[0x4D] = 0xAA;                                                             // Joust won't work if this memory location equals zero
  ram[0xD0] = 0xAA;                                                             // Planetoids won't work if this memory location equals zero
}
