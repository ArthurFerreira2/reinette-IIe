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

#include "reinette.h"

Paddles::Paddles() {
  PB0 = 0;                                                                      // $C061 Push Button 0 (bit 7) / Open Apple
  PB1 = 0;                                                                      // $C062 Push Button 1 (bit 7) / Solid Apple
  PB2 = 0;                                                                      // $C063 Push Button 2 (bit 7) / shift mod !!!
  GCActionSpeed = 8;                                                            // Game Controller speed at which it goes to the edges
  GCReleaseSpeed = 8;                                                           // Game Controller speed at which it returns to center
  GCCrigger = 0;                                                                // $C070 the tick at which the GCs were reseted
}


void Paddles::reset() {
  GCC[0] = GCP[0] * GCP[0];                                                     // initialize the countdown for both paddles
  GCC[1] = GCP[1] * GCP[1];                                                     // to the square of their actuall values (positions)
  GCC[2] = GCP[2] * GCP[2];                                                     // to the square of their actuall values (positions)
  GCC[3] = GCP[3] * GCP[3];                                                     // to the square of their actuall values (positions)
  GCCrigger = cpu->ticks;                                                       // records the time this was done
}


uint8_t Paddles::read(int pdl) {
  const float GCFreq = 6.6;                                                     // the speed at which the GC values decrease

  GCC[pdl] -= (cpu->ticks - GCCrigger) / GCFreq;                                // decreases the countdown
  if (GCC[pdl] <= 0)                                                            // timeout
    return GCC[pdl] = 0;                                                        // returns 0
  return 0x80;                                                                  // not timeout, return something with the MSB set
}


void Paddles::update() {
  for (int pdl = 0; pdl < 4; pdl++) {                                           // update the two paddles positions
    if (GCA[pdl]) {                                                             // actively pushing the stick
      GCP[pdl] += GCD[pdl] * GCActionSpeed;
      if (GCP[pdl] > 255) GCP[pdl] = 255;
      if (GCP[pdl] < 0)   GCP[pdl] = 0;
    } else {                                                                    // the stick returns back to center
      GCP[pdl] += GCD[pdl] * GCReleaseSpeed;
      if (GCD[pdl] == 1  && GCP[pdl] > 127) GCP[pdl] = 127;
      if (GCD[pdl] == -1 && GCP[pdl] < 127) GCP[pdl] = 127;
    }
  }
}
