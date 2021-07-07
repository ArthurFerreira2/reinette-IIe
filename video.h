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

#ifndef _VIDEO_H
#define _VIDEO_H

class Video {
public:
  int gfxmode;
  uint8_t glyph;                                                                // a TEXT character, or 2 blocks in GR
  enum characterAttribute { A_NORMAL, A_INVERSE, A_FLASH } glyphAttr;           // character attribute in TEXT
  uint8_t previousBit[192][40] = {0};                                           // the last bit value of the byte before.

  int previousDots[192][40] = {0};                                              // check which Hi-Res 7 dots needs redraw
  int previousBlocks[24][40] = {0};                                             // check which Lo-Res blocks or text chars needs redraw
  int previousChars[24][40] = {0};                                              // check which Lo-Res blocks or text chars needs redraw
  int previousChars80[24][40] = {0};                                            // check which Lo-Res blocks or text chars needs redraw

  uint32_t fontNormal[128][8][7];                                               // normal font
  uint32_t fontInverse[128][8][7];                                              // reversed font

  uint32_t screenPixels[280*192] = {0xFF000000};
  uint32_t screenPixels80[560*384] = {0xFF000000};
  uint32_t ramHeatmap[256*256] = {0xFF000000};
  uint32_t auxHeatmap[256*256] = {0xFF000000};

  Video();
  ~Video();

  void update();
  void clearCache();
};

#endif
