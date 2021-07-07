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

#ifndef __REINETTE_H__
#define __REINETTE_H__

#define VERSION "0.8.1"

#include <cstdint>

#include "puce65c02.h"
#include "mmu.h"
#include "disk.h"
#include "video.h"
#include "speaker.h"
#include "paddles.h"
#include "gui.h"


extern bool muted;
extern int  volume;
extern bool running;  // the entire application
extern bool paused;   // the virtual machine
extern float speed;

extern puce65c02* cpu;
extern Mmu*       mmu;
extern Disk*      disk;
extern Video*     video;
extern Speaker*   speaker;
extern Paddles*   paddles;
extern Gui*       gui;

#endif
