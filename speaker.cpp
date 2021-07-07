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

Speaker::Speaker() {
  if (SDL_Init(SDL_INIT_AUDIO) != 0) {
      printf("Error: %s\n", SDL_GetError());
      exit(EXIT_FAILURE);
  }
  SDL_AudioSpec desired = { 96000, AUDIO_S8, 1, 0, 512, 0, 0, NULL, NULL };
  audioDevice = SDL_OpenAudioDevice(NULL, 0, &desired, NULL, SDL_FALSE);        // get the audio device ID
  setVolume(volume);
  SDL_PauseAudioDevice(audioDevice, muted);                                     // unmute it (muted is false)
}


void Speaker::play() {
  static unsigned long long int lastTick = 0LL;
  static bool SPKR = false;                                                     // $C030 Speaker toggle

  SPKR = !SPKR;                                                                 // toggle speaker state
  Uint32 length = (int)((double)(cpu->ticks - lastTick) / 10.42f / speed);      // 1000000Hz / 96000Hz = 10.4166
  lastTick = cpu->ticks;
  if (!muted) {
    if (length > audioBufferSize)
      SDL_QueueAudio(audioDevice, audioBuffer[2], audioBufferSize);             // silence
    else
      SDL_QueueAudio(audioDevice, audioBuffer[SPKR], length | 1);               // | 1 TO HEAR HIGH FREQ SOUNDS
  }
}


void Speaker::toggleMute() {
  SDL_PauseAudioDevice(audioDevice, muted);
  SDL_ClearQueuedAudio(audioDevice);
}


void Speaker::setVolume(int newVolume) {
  if (muted) {
    muted = false;
    toggleMute();
  }

  if (newVolume > 127)
    volume = 127;
  else if (newVolume < 0)
    volume = 0;
  else volume = newVolume;

  for (int i = 0; i < audioBufferSize; i++) {                                   // two audio buffers,
    audioBuffer[true][i]  = (int8_t)volume;                                     // one used when SPKR is true
    audioBuffer[false][i] = (int8_t)-volume;                                    // the other when SPKR is false
    audioBuffer[2][i]     = 0;                                                  // silence
  }
}
