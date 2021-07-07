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
#include <iostream>

// global variables - TODO create a config file and make changes persistant
bool  muted   = false;
int   volume  = 10;
bool  running = true;
bool  paused  = false;
float speed   = 1.023f;

// instanciate all objects, calling their respective constructors
Mmu*       mmu     = new Mmu();
puce65c02* cpu     = new puce65c02();
Video*     video   = new Video();
Disk*      disk    = new Disk();
Speaker*   speaker = new Speaker();
Paddles*   paddles = new Paddles();
Gui*       gui     = new Gui();


int main(int argc, char *argv[]) {

  cpu->RST();
  if (argc > 1) disk->load(argv[1], 0);                                         // load .nib in parameter into drive 0
  uint8_t tries = 0;                                                            // for disk ][ speed-up

  // main loop
  while (running) {

    gui->getInputs();
    paddles->update();
    gui->newFrame();

    if (!paused) {
      // dirty hack - fix soon TODO - a better VERTBLANK ... - won't work when stepping through instructions
      mmu->VERTBLANK = true;
      cpu->exec((unsigned long long int)(1000000.0 * speed / gui->fps*0.1f));   // the apple II is clocked at 1023000.0 Hhz
      mmu->VERTBLANK = false;
      cpu->exec((unsigned long long int)(1000000.0 * speed / gui->fps*0.9f));   // the apple II is clocked at 1023000.0 Hhz

      while (disk->unit[disk->curDrv].motorOn && ++tries)                       // until motor is off or i reaches 255+1=0
        cpu->exec(5000);                                                        // speed up drive access artificially
      video->update();                                                          // won't update the video if paused
    }
    else if (cpu->state == step) {                                              // paused and user pressed debugNumber
      cpu->exec(1);
      cpu->state = run;                                                         // still paused
      video->update();                                                          // update the video after each instruction ...
    }

    gui->update();
    gui->render();

  }  // while (running)

  return 0;
  // at this point all destructors were called, properly closing open files and releasing other ressources
}
