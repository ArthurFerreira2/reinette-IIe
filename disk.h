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

#ifndef __DISK_H__
#define __DISK_H__

typedef struct Drive_t {
  char     fileName[400];                                                       // the floppy image file name
  char     pathName[400];                                                       // the full floppy image path name
  bool     readOnly;                                                            // based on the image file attributes
  uint8_t  data[232960];                                                        // nibblelized floppy image
  bool     motorOn;                                                             // motor status
  bool     writeMode;                                                           // writes to file are not implemented
  uint8_t  track;                                                               // current track position
  uint16_t nibble;                                                              // ptr to nibble under head position
} Drive;


class Disk {
public:
  int curDrv = 0;                                                               // Current Drive - only one can be enabled at a time
  Drive unit[2] = {0};                                                          // two disk ][ drive units

  Disk();
  ~Disk();

  int load(char *filename, int drv);
  int save(int drive);
  int eject(int drive);

  void setDrv(int drv);
  void stepMotor(uint16_t address);
};

#endif
