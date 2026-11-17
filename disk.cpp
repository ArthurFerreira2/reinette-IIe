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
#include <sys/stat.h>
#include <unistd.h>
#include "reinette.h"
#include "gcr62.h"


// writes data to path via a temp file + rename, so that a failure at any point
// (disk full, process killed, power loss) never leaves a truncated image :
// the original file is only replaced by the rename, which is atomic
static int atomicWrite(const char *path, const void *data, size_t size) {
  char tmp[420];
  if ((size_t)snprintf(tmp, sizeof(tmp), "%s.reinette-tmp", path) >= sizeof(tmp))
    return 0;                                                                    // path too long

  struct stat st;
  bool hadStat = (stat(path, &st) == 0);                                         // to keep the file mode

  FILE *f = fopen(tmp, "wb");
  if (!f) return 0;                                                              // could not create the temp file
  size_t written = fwrite(data, 1, size, f);
  int flushed = (fflush(f) == 0) && (fsync(fileno(f)) == 0);                     // get the data on the medium
  if (written != size || !flushed || fclose(f) != 0) {
    remove(tmp);                                                                 // release the ressource,
    return 0;                                                                    // leave the original untouched
  }

  if (hadStat) chmod(tmp, st.st_mode & 07777);                                   // keep the original file mode
  if (rename(tmp, path) != 0) {                                                  // the atomic replacement
    remove(tmp);
    return 0;
  }
  return 1;
}


Disk::Disk() {
  curDrv = 0;                                                                   // Current Drive - only one can be enabled at a time
  // TODO : initialize the unit[x] structs
}


int Disk::load( char *path, int drive) {
  FILE *f = fopen(path, "rb");                                                  // open file in read binary mode

  if (!f) return 0;

  fseek(f, 0, SEEK_END);                                                        // dispatch on the image size
  long size = ftell(f);
  fseek(f, 0, SEEK_SET);

  if (size == gcr62::NIB_FILE_BYTES) {                                          // 232960 : raw nibble image
    if (fread(unit[drive].data, 1, gcr62::NIB_FILE_BYTES, f) != gcr62::NIB_FILE_BYTES) {
      fclose(f);
      return 0;
    }
    unit[drive].dskFormat = false;
  }

  else if (size == gcr62::DSK_FILE_BYTES) {                                     // 143360 : DOS 3.3 .dsk image, nibblelize it
    static uint8_t dsk[gcr62::DSK_FILE_BYTES];
    if (fread(dsk, 1, gcr62::DSK_FILE_BYTES, f) != gcr62::DSK_FILE_BYTES) {
      fclose(f);
      return 0;
    }
    for (int track = 0; track < gcr62::TRACKS; track++)
      gcr62::encodeTrack(dsk + track * gcr62::DSK_TRACK_BYTES,
                         unit[drive].data + track * gcr62::NIB_TRACK_BYTES, track);
    unit[drive].dskFormat = true;
  }

  else {                                                                        // not a supported image
    fclose(f);
    return 0;
  }

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

  if (unit[drive].dskFormat) {                                                  // .dsk image : denibblelize each track
    static uint8_t dsk[gcr62::DSK_FILE_BYTES];
    for (int track = 0; track < gcr62::TRACKS; track++)
      if (!gcr62::decodeTrack(unit[drive].data + track * gcr62::NIB_TRACK_BYTES,
                              dsk + track * gcr62::DSK_TRACK_BYTES, track))
        return 0;                                                               // damaged or non-standard track : never write a partial image

    return atomicWrite(unit[drive].pathName, dsk, gcr62::DSK_FILE_BYTES);
  }

  return atomicWrite(unit[drive].pathName, unit[drive].data, 232960);
}


int Disk::eject(int drive)   {
  unit[drive].fileName[0] = 0;
  unit[drive].pathName[0] = 0;
  unit[drive].readOnly = false;
  unit[drive].dskFormat = false;

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
