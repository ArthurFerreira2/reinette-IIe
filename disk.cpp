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

#include <stdio.h>
#include "reinette.h"

Disk::Disk() {
  curDrv = 0;                                                                   // Current Drive - only one can be enabled at a time
  // TODO : initialize the unit[x] structs
}


int Disk::load( char *path, int drive) {
  FILE *f = fopen(path, "rb");                                                  // open file in read binary mode

  if (!f || fread(unit[drive].data, 1, 232960, f) != 232960)                    // load it into memory and check size
    return 0;
  fclose(f);

  sprintf(unit[drive].pathName, "%s", path);                                    // update floppy image pathName record

  // get filename
  int i =0, a = 0;
  while (unit[drive].pathName[i] != 0) {                                        // find start of filename for disk0
    if (unit[drive].pathName[i] == '/' || unit[drive].pathName[i] == '\\')
       a = i + 1;
    i++;
  }
  sprintf(unit[drive].fileName, "%s", unit[drive].pathName + a);

  // is the file read only ?
  f = fopen(path, "ab");                                                        // try to open the file in append binary mode
  if (f) {                                                                      // success, file is writable
    unit[drive].readOnly = false;                                               // update the readOnly flag
    fclose(f);                                                                  // and close it untouched
  } else {
    unit[drive].readOnly = true;                                                // f is NULL, no writable, no need to close it
  }
  return 1;
}


int Disk::save(int drive) {
  if (!unit[drive].pathName[0]) return 0;                                       // no file loaded into drive
  if (unit[drive].readOnly) return 0;                                           // file is read only write no aptempted

  FILE *f = fopen(unit[drive].pathName, "wb");

  if (!f) return 0;                                                             // could not open the file in write overide binary
  if (fwrite(unit[drive].data, 1, 232960, f) != 232960) {                       // failed to write the full file (hdd full ?)
    fclose(f);                                                                  // release the ressource
    return 0;
  }
  fclose(f);                                                                    // success, release the ressource
  return 1;
}


int Disk::eject(int drive)   {
  unit[drive].fileName[0] = 0;
  unit[drive].pathName[0] = 0;
  unit[drive].readOnly = false;

  for (int i=0; i<232960; i++)  // erase data
    unit[drive].data[i]=0;

  return 1;
}


void Disk::stepMotor(uint16_t address) {
  static bool phases[2][4] = { 0 };                                             // phases states (for both drives)
  static bool phasesB[2][4] = { 0 };                                            // phases states Before
  static bool phasesBB[2][4] = { 0 };                                           // phases states Before Before
  static int pIdx[2] = { 0 };                                                   // phase index (for both drives)
  static int pIdxB[2] = { 0 };                                                  // phase index Before
  static int halfTrackPos[2] = { 0 };

  address &= 7;
  int phase = address >> 1;

  phasesBB[curDrv][pIdxB[curDrv]] = phasesB[curDrv][pIdxB[curDrv]];
  phasesB[curDrv][pIdx[curDrv]]   = phases[curDrv][pIdx[curDrv]];
  pIdxB[curDrv] = pIdx[curDrv];
  pIdx[curDrv]  = phase;

  if (!(address & 1)) {                                                         // head not moving (PHASE x OFF)
    phases[curDrv][phase] = false;
    return;
  }

  if ((phasesBB[curDrv][(phase + 1) & 3]) && (--halfTrackPos[curDrv] < 0))      // head is moving in
    halfTrackPos[curDrv] = 0;

  if ((phasesBB[curDrv][(phase - 1) & 3]) && (++halfTrackPos[curDrv] > 140))    // head is moving out
    halfTrackPos[curDrv] = 140;

  phases[curDrv][phase] = true;                                                 // update track#
  unit[curDrv].track = (halfTrackPos[curDrv] + 1) / 2;
}


void Disk::setDrv(int drv) {
  unit[drv].motorOn = unit[!drv].motorOn || unit[drv].motorOn;                  // if any of the motors were ON
  unit[!drv].motorOn = false;                                                   // motor of the other drive is set to OFF
  curDrv = drv;                                                                 // set the current drive
}
