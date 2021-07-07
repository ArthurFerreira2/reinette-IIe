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

#ifndef __PADDLES_H__
#define __PADDLES_H__

class Paddles {
public:
  uint8_t PB0;                                                                  // $C061 Push Button 0 (bit 7) / Open Apple
  uint8_t PB1;                                                                  // $C062 Push Button 1 (bit 7) / Solid Apple
  uint8_t PB2;                                                                  // $C063 Push Button 2 (bit 7) / shift mod !!!

  float GCP[4] = { 127.0f, 127.0f, 127.0f, 127.0f };                            // GC Position ranging from 0 (left) to 255 right
  float GCC[4] = { 0.0f };                                                      // $C064 (GC0) and $C065 (GC1) Countdowns

  int GCD[4] = { 0 };                                                           // GC0 and GC1 Directions (left/down or right/up)
  int GCA[4] = { 0 };                                                           // GC0 and GC1 Action (push or release)

  int GCActionSpeed;                                                            // Game Controller speed at which it goes to the edges
  int GCReleaseSpeed;                                                           // Game Controller speed at which it returns to center
  long long int GCCrigger;                                                      // $C070 the tick at which the GCs were reseted

  Paddles();
  ~Paddles();

  void reset();
  void update();
  uint8_t read(int pdl);
};

#endif
