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

#ifndef _GUI_H
#define _GUI_H

#include <cstdio>
#include <cstdlib>

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl2.h"
#include "imfilebrowser.h"
#include "imgui_memory_editor.h"
#include "TextEditor.h"

#include <SDL2/SDL.h>
#include <SDL_opengl.h>
#include <fstream>

class Gui {
public:
  int fps;
  unsigned int  frameNumber;

private:
  bool show_about_window;
  bool show_help_window;
  bool CRT_is_focused;
  bool show_crt_window;
  bool show_control_window;
  bool show_disks_window;
  bool show_info_window;
  bool show_cpu_window;
  bool show_code_window;
  bool show_pageZero_window;
  bool show_stack_window;
  bool show_ram_window;
  bool show_aux_window;
  bool show_rom_window;
  bool show_editor_window;
  bool show_ramHeatmap_window;
  bool show_auxHeatmap_window;

  ImVec4 clear_color;
  uint32_t screenTexture;
  uint32_t ramHeatmapTexture;
  uint32_t auxHeatmapTexture;

  SDL_Window* wdo;
  float screenScale;
  SDL_GLContext gl_context;

  TextEditor editor;
  std::string fileToEdit;
  ImGui::FileBrowser fileDialog1;
  ImGui::FileBrowser fileDialog2;

public:
  Gui();
  ~Gui();
  void getInputs();
  int update();
  int newFrame();
  int render();
  int release();
};

#endif
