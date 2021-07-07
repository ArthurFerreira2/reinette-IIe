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

Gui::Gui() {
  frameNumber = 0;
  screenScale = 2.0f;
  fps = 60;

  show_about_window    = false;
  show_help_window     = false;
  CRT_is_focused       = false;
  show_crt_window      = true;
  show_control_window  = true;
  show_disks_window    = true;
  show_info_window     = true;
  show_cpu_window      = true;
  show_code_window     = true;
  show_pageZero_window = true;
  show_stack_window    = true;
  show_ram_window      = true;
  show_aux_window      = true;
  show_rom_window      = true;
  show_editor_window   = true;
  show_ramHeatmap_window = true;
  show_auxHeatmap_window = true;

  // Setup SDL
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
      printf("Error: %s\n", SDL_GetError());
      exit(EXIT_FAILURE);
  }

  // Setup window
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
  SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  wdo = SDL_CreateWindow("Reinette", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1700, 900, window_flags);
  gl_context = SDL_GL_CreateContext(wdo);
  SDL_GL_MakeCurrent(wdo, gl_context);
  SDL_GL_SetSwapInterval(1);                                                    // Enable vsync

  SDL_EventState(SDL_DROPFILE, SDL_ENABLE);                                     // ask SDL2 to read dropfile events

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO& io = ImGui::GetIO(); (void)io;
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;                         // Enable Keyboard Controls
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;                          // Enable Gamepad Controls

  #include "imguiTheme.h"

  // Setup Platform/Renderer backends
  ImGui_ImplSDL2_InitForOpenGL(wdo, gl_context);
  ImGui_ImplOpenGL2_Init();

  clear_color = ImVec4((float)51/256, (float)58/256, (float)64/256, 1.00f);     // fond de la fenetre principale
  glGenTextures(1, &screenTexture);
  glGenTextures(1, &ramHeatmapTexture);
  glGenTextures(1, &auxHeatmapTexture);

  // editor init
  auto lang = TextEditor::LanguageDefinition::AppleSoft();
  editor.SetLanguageDefinition(lang);
  editor.SetShowWhitespaces(false);
  editor.SetPalette(TextEditor::GetLightPalette());
  fileToEdit = "program.bas";
  std::ifstream t(fileToEdit);
  if (t.good()) {
    std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
    editor.SetText(str);
  }
}



int Gui::newFrame() {
  // Start the Dear ImGui frame
  ImGui_ImplOpenGL2_NewFrame();
  ImGui_ImplSDL2_NewFrame(wdo);
  ImGui::NewFrame();
  return 0;
}



void Gui::getInputs() {

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL2_ProcessEvent(&event);

    SDL_bool ctrl, shift, alt;

    alt = SDL_GetModState() & KMOD_ALT   ? SDL_TRUE : SDL_FALSE;
    ctrl = SDL_GetModState() & KMOD_CTRL  ? SDL_TRUE : SDL_FALSE;
    shift = SDL_GetModState() & KMOD_SHIFT ? SDL_TRUE : SDL_FALSE;
    paddles->PB0 = alt   ? 0xFF : 0x00;                                         // update push button 0, Open Apple
    paddles->PB1 = ctrl  ? 0xFF : 0x00;                                         // update push button 1, Solid Apple or option
    paddles->PB2 = shift ? 0xFF : 0x00;                                         // update push button 2, single-wire Shift-key mod


    if (event.type == SDL_QUIT) running = false;                                // WM sent TERM signal

    if (event.type == SDL_DROPFILE) {                                           // user dropped a file
      char *filename = event.drop.file;                                         // get full pathname
      if (!disk->load(filename, alt))                                           // if ALT : drv 1 else drv 0
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Load", "Not a valid nib file", NULL);
      SDL_free(filename);                                                       // free filename memory
      paused = false;                                                           // might already be the case
      if (!(alt || ctrl)) {                                                     // unless ALT or CTRL were
        mmu->init();
        mmu->writeMem(0x3F4,0);                                                 // unset the Power-UP byte
        cpu->RST();                                                             // do a cold reset
      }
    }

    if (CRT_is_focused) {      // if (!io.WantCaptureKeyboard) {
      if (event.type == SDL_KEYDOWN) {                                          // a key has been pressed
        switch (event.key.keysym.sym) {

        // EMULATOR CONTROLS :

        case SDLK_F3:                                                           // PASTE text from clipboard
          if (SDL_HasClipboardText()) {
            char *clipboardText = SDL_GetClipboardText();
            int c = 0;
            while (clipboardText[c]) {                                          // all chars until ascii NUL
              mmu->KBD = clipboardText[c++] | 0x80;                             // set bit7
              if (mmu->KBD == 0x8A) mmu->KBD = 0x8D;                            // translate Line Feed to Carriage Ret
              cpu->exec(400000);                                                // give cpu (and applesoft) some cycles to process each char
            }
            SDL_free(clipboardText);                                            // release the ressource
          }
        break;

        case SDLK_F4:                                                           // VOLUME
          if (shift) speaker->setVolume(volume+10);                             // increase volume
          if (ctrl)  speaker->setVolume(volume-10);                             // decrease volume
          if (!ctrl && !shift) speaker->toggleMute();                           // toggle mute / unmute
        break;

        case SDLK_F5:                                                           // JOYSTICK Release Speed
          if (shift && (paddles->GCReleaseSpeed < 127)) paddles->GCReleaseSpeed += 2;  // increase Release Speed
          if (ctrl && (paddles->GCReleaseSpeed > 1)) paddles->GCReleaseSpeed -= 2;     // decrease Release Speed
          if (!ctrl && !shift) paddles->GCReleaseSpeed = 8;                     // reset Release Speed to 8
          break;

        case SDLK_F6:                                                           // JOYSTICK Action Speed
          if (shift && (paddles->GCActionSpeed < 127)) paddles->GCActionSpeed += 2;  // increase Action Speed
          if (ctrl && (paddles->GCActionSpeed > 1)) paddles->GCActionSpeed -= 2;     // decrease Action Speed
          if (!ctrl && !shift) paddles->GCActionSpeed = 8;                      // reset Action Speed to 8
          break;

        case SDLK_F7:                                                           // ZOOM
          if (shift && (screenScale <= 3.9f)) screenScale+=.1f;                 // zoom in
          if (ctrl && (screenScale > 1.0f)) screenScale-=.1f;                   // zoom out
          if (!ctrl && !shift) screenScale = 2.0f;                              // reset zoom to 2
        break;

        case SDLK_F10: paused = !paused; break;                                 // toggle pause

        case SDLK_F11:  {
          paused = true;
          cpu->state = step;
        } break;

        case SDLK_F12: cpu->RST(); break;                                       // reset

        // EMULATED KEYS :

        case SDLK_a:            mmu->KBD = ctrl ? 0x81: 0xC1;   break;          // a
        case SDLK_b:            mmu->KBD = ctrl ? 0x82: 0xC2;   break;          // b STX
        case SDLK_c:            mmu->KBD = ctrl ? 0x83: 0xC3;   break;          // c ETX
        case SDLK_d:            mmu->KBD = ctrl ? 0x84: 0xC4;   break;          // d EOT
        case SDLK_e:            mmu->KBD = ctrl ? 0x85: 0xC5;   break;          // e
        case SDLK_f:            mmu->KBD = ctrl ? 0x86: 0xC6;   break;          // f ACK
        case SDLK_g:            mmu->KBD = ctrl ? 0x87: 0xC7;   break;          // g BELL
        case SDLK_h:            mmu->KBD = ctrl ? 0x88: 0xC8;   break;          // h BS
        case SDLK_i:            mmu->KBD = ctrl ? 0x89: 0xC9;   break;          // i HTAB
        case SDLK_j:            mmu->KBD = ctrl ? 0x8A: 0xCA;   break;          // j LF
        case SDLK_k:            mmu->KBD = ctrl ? 0x8B: 0xCB;   break;          // k VTAB
        case SDLK_l:            mmu->KBD = ctrl ? 0x8C: 0xCC;   break;          // l FF
        case SDLK_m:            mmu->KBD = ctrl ? shift ? 0x9D:0x8D:0xCD; break;// m CR ]
        case SDLK_n:            mmu->KBD = ctrl ? shift ? 0x9E:0x8E:0xCE; break;// n ^
        case SDLK_o:            mmu->KBD = ctrl ? 0x8F: 0xCF;   break;          // o
        case SDLK_p:            mmu->KBD = ctrl ? shift ? 0x80:0x90:0xD0; break;     // p @
        case SDLK_q:            mmu->KBD = ctrl ? 0x91: 0xD1;   break;          // q
        case SDLK_r:            mmu->KBD = ctrl ? 0x92: 0xD2;   break;          // r
        case SDLK_s:            mmu->KBD = ctrl ? 0x93: 0xD3;   break;          // s ESC
        case SDLK_t:            mmu->KBD = ctrl ? 0x94: 0xD4;   break;          // t
        case SDLK_u:            mmu->KBD = ctrl ? 0x95: 0xD5;   break;          // u NAK
        case SDLK_v:            mmu->KBD = ctrl ? 0x96: 0xD6;   break;          // v
        case SDLK_w:            mmu->KBD = ctrl ? 0x97: 0xD7;   break;          // w
        case SDLK_x:            mmu->KBD = ctrl ? 0x98: 0xD8;   break;          // x CANCEL
        case SDLK_y:            mmu->KBD = ctrl ? 0x99: 0xD9;   break;          // y
        case SDLK_z:            mmu->KBD = ctrl ? 0x9A: 0xDA;   break;          // z
        case SDLK_LEFTBRACKET:  mmu->KBD = ctrl ? 0x9B: 0xDB;   break;          // [ {
        case SDLK_BACKSLASH:    mmu->KBD = ctrl ? 0x9C: 0xDC;   break;          // \ |
        case SDLK_RIGHTBRACKET: mmu->KBD = ctrl ? 0x9D: 0xDD;   break;          // ] }
        case SDLK_BACKSPACE:    mmu->KBD = ctrl ? 0xDF: 0x88;   break;          // BS
        case SDLK_0:            mmu->KBD = shift? 0xA9: 0xB0;   break;          // 0 )
        case SDLK_1:            mmu->KBD = shift? 0xA1: 0xB1;   break;          // 1 !
        case SDLK_2:            mmu->KBD = shift? 0xC0: 0xB2;   break;          // 2
        case SDLK_3:            mmu->KBD = shift? 0xA3: 0xB3;   break;          // 3 #
        case SDLK_4:            mmu->KBD = shift? 0xA4: 0xB4;   break;          // 4 $
        case SDLK_5:            mmu->KBD = shift? 0xA5: 0xB5;   break;          // 5 %
        case SDLK_6:            mmu->KBD = shift? 0xDE: 0xB6;   break;          // 6 ^
        case SDLK_7:            mmu->KBD = shift? 0xA6: 0xB7;   break;          // 7 &
        case SDLK_8:            mmu->KBD = shift? 0xAA: 0xB8;   break;          // 8 *
        case SDLK_9:            mmu->KBD = shift? 0xA8: 0xB9;   break;          // 9 (
        case SDLK_QUOTE:        mmu->KBD = shift? 0xA2: 0xA7;   break;          // ' "
        case SDLK_EQUALS:       mmu->KBD = shift? 0xAB: 0xBD;   break;          // = +
        case SDLK_SEMICOLON:    mmu->KBD = shift? 0xBA: 0xBB;   break;          // ; :
        case SDLK_COMMA:        mmu->KBD = shift? 0xBC: 0xAC;   break;          // , <
        case SDLK_PERIOD:       mmu->KBD = shift? 0xBE: 0xAE;   break;          // . >
        case SDLK_SLASH:        mmu->KBD = shift? 0xBF: 0xAF;   break;          // / ?
        case SDLK_MINUS:        mmu->KBD = shift? 0xDF: 0xAD;   break;          // - _
        case SDLK_BACKQUOTE:    mmu->KBD = shift? 0xFE: 0xE0;   break;          // ` ~
        case SDLK_LEFT:         mmu->KBD = 0x88;                break;          // BS
        case SDLK_RIGHT:        mmu->KBD = 0x95;                break;          // NAK
        case SDLK_DOWN:         mmu->KBD = 0x8A;                break;          // LF
        case SDLK_UP:           mmu->KBD = 0x8B;                break;          // VTAB
        case SDLK_SPACE:        mmu->KBD = 0xA0;                break;
        case SDLK_ESCAPE:       mmu->KBD = 0x9B;                break;          // ESC
        case SDLK_RETURN:       mmu->KBD = 0x8D;                break;          // CR
        case SDLK_TAB:          mmu->KBD = 0x89;                break;          // HTAB

        // EMULATED JOYSTICK :

        case SDLK_KP_1: paddles->GCD[0] = -1; paddles->GCA[0] = 1; break;       // pdl0 <-
        case SDLK_KP_3: paddles->GCD[0] = 1;  paddles->GCA[0] = 1; break;       // pdl0 ->
        case SDLK_KP_5: paddles->GCD[1] = -1; paddles->GCA[1] = 1; break;       // pdl1 <-
        case SDLK_KP_2: paddles->GCD[1] = 1;  paddles->GCA[1] = 1; break;       // pdl1 ->
        }
      }

      if (event.type == SDL_KEYUP) {
        switch (event.key.keysym.sym) {
        case SDLK_KP_1: paddles->GCD[0] = 1;  paddles->GCA[0] = 0; break;       // pdl0 ->
        case SDLK_KP_3: paddles->GCD[0] = -1; paddles->GCA[0] = 0; break;       // pdl0 <-
        case SDLK_KP_5: paddles->GCD[1] = 1;  paddles->GCA[1] = 0; break;       // pdl1 ->
        case SDLK_KP_2: paddles->GCD[1] = -1; paddles->GCA[1] = 0; break;       // pdl1 <-
        }
      }
    }
  }
}




int Gui::update() {

  if (ImGui::BeginMainMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("New", "Ctrl+N")) {
        // void editor.SetText(const std::string& aText);
      }

      if (ImGui::MenuItem("Open", "Ctrl+O")) {
        // void SetText(const std::string& aText);
      }

      if (ImGui::MenuItem("Save", "Ctrl+S")) {
        // std::string textToSave = editor.GetText();
        // std::ifstream t(fileToEdit);
        // if (t.good()) {
        //   std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
        //   editor.SetText(str);
        // }
      }

      if (ImGui::MenuItem("Save As..")) {
        // TBD
      }

      if (ImGui::MenuItem("Quit", "CTRL+Q")) {
        running = false;
      }
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Edit")) {
      bool ro = editor.IsReadOnly();
      if (ImGui::MenuItem("Read-only mode", nullptr, &ro))
        editor.SetReadOnly(ro);

      ImGui::Separator();
      if (ImGui::MenuItem("Undo", "CTRL+Z", nullptr, !ro && editor.CanUndo()))
        editor.Undo();
      if (ImGui::MenuItem("Redo", "CTRL+Y", nullptr, !ro && editor.CanRedo()))
        editor.Redo();

      ImGui::Separator();
      if (ImGui::MenuItem("Copy", "Ctrl-C", nullptr, editor.HasSelection()))
        editor.Copy();
      if (ImGui::MenuItem("Cut",  "Ctrl-X", nullptr, !ro && editor.HasSelection()))
        editor.Cut();
      if (ImGui::MenuItem("Delete", "Del",  nullptr, !ro && editor.HasSelection()))
        editor.Delete();
      if (ImGui::MenuItem("Paste", "Ctrl-V", nullptr, !ro && ImGui::GetClipboardText() != nullptr))
        editor.Paste();

      ImGui::Separator();
      if (ImGui::MenuItem("Select all", nullptr, nullptr))
        editor.SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(editor.GetTotalLines(), 0));

      ImGui::Separator();
      if (ImGui::MenuItem("Dark palette"))
        editor.SetPalette(TextEditor::GetDarkPalette());
      if (ImGui::MenuItem("Light palette"))
        editor.SetPalette(TextEditor::GetLightPalette());
      if (ImGui::MenuItem("Retro blue palette"))
        editor.SetPalette(TextEditor::GetRetroBluePalette());

      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("View")) {
      ImGui::MenuItem("CRT", NULL, &show_crt_window);
      ImGui::Separator();
      ImGui::MenuItem("Page Zero", NULL, &show_pageZero_window);
      ImGui::MenuItem("Stack", NULL, &show_stack_window);
      ImGui::MenuItem("Main RAM", NULL, &show_ram_window);
      ImGui::MenuItem("Aux RAM", NULL, &show_aux_window);
      ImGui::MenuItem("ROM", NULL, &show_rom_window);
      ImGui::MenuItem("RAM Heatmap", NULL, &show_ramHeatmap_window);
      ImGui::MenuItem("AUX Heatmap", NULL, &show_auxHeatmap_window);
      ImGui::Separator();
      ImGui::MenuItem("Controls", NULL, &show_control_window);
      ImGui::MenuItem("CPU", NULL, &show_cpu_window);
      ImGui::MenuItem("Code", NULL, &show_code_window);
      ImGui::MenuItem("Disk", NULL, &show_disks_window);
      ImGui::MenuItem("Info", NULL, &show_info_window);
      ImGui::Separator();
      ImGui::MenuItem("Editor", NULL, &show_editor_window);
      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Help")) {
      ImGui::MenuItem("Help", NULL, &show_help_window);
      ImGui::MenuItem("About", NULL, &show_about_window);
      ImGui::EndMenu();
    }
    ImGui::EndMainMenuBar();
  }

  static MemoryEditor mem_edit_stack;
  if (show_stack_window) {
    mem_edit_stack.DrawWindow("STACK", mmu->ram+256, 256);
  }

  static MemoryEditor mem_edit_pageZero;
  if (show_pageZero_window) {
    mem_edit_pageZero.DrawWindow("PAGE ZERO", mmu->ram, 256);
  }

  static MemoryEditor mem_edit_ram;
  if (show_ram_window) {
    mem_edit_ram.DrawWindow("RAM", mmu->ram, RAMSIZE);
  }

  static MemoryEditor mem_edit_rom;
  if (show_rom_window) {
    mem_edit_rom.DrawWindow("ROM", mmu->rom, ROMSIZE);
  }

  static MemoryEditor mem_edit_aux;
  if (show_aux_window) {
    mem_edit_aux.DrawWindow("AUX", mmu->aux, AUXSIZE);
  }

  if (show_about_window) {
    if (ImGui::Begin("ABOUT", &show_about_window, ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("Reinette 2021 Arthur Ferreira " );
      ImGui::Separator();
      ImGui::Text("version : %s", VERSION);
      ImGui::Text("https://github.com/ArthurFerreira2");
      ImGui::Text("\nReinette is licensed under the MIT License\nsee LICENSE for more information.\n");
    ImGui::End();
    }
  }

  if (show_help_window) {
    if (ImGui::Begin("HELP", &show_help_window, ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("Reinette v%s", VERSION);
      ImGui::Separator();
      ImGui::Text(  "\nctrl F1      writes the changes of the floppy in drive 0"
                    "\nalt F1       writes the changes of the floppy in drive 1"
                    "\n"
                    "\nF3           paste text from clipboard"
                    "\n"
                    "\nF4           mute / un-mute sound"
                    "\nshift F4     increase volume"
                    "\nctrl F4      decrease volume"
                    "\n"
                    "\nF5           reset joystick release speed"
                    "\nshift F5     increase joystick release speed"
                    "\ncrtl F5      decrease joystick release speed"
                    "\n"
                    "\nF6           reset joystick action speed"
                    "\nshift F6     increase joystick action speed"
                    "\ncrtl F6      decrease joystick action speed"
                    "\n"
                    "\nF7           reset the zoom to 2:1"
                    "\nshift F7     increase zoom up to 4:1 max"
                    "\nctrl F7      decrease zoom down to 1:1 pixels"
                    "\n"
                    "\nF8           monochrome / color display (only in HGR)"
                    "\nF9           pause / un-pause the emulator"
                    "\nF10          Not implemented"
                    "\nF11          reset");
    ImGui::End();
    }
  }

  if (show_ramHeatmap_window) {
    ImGui::Begin("RAM HEATMAP", &show_ramHeatmap_window);
      // Adjust the image to the window
      auto window_size = ImGui::GetWindowSize();
      auto image_size = ImVec2(window_size.x - 15, window_size.y - 50);
      // Draw image
      ImGui::Image((void*)((intptr_t)ramHeatmapTexture), image_size, ImVec2(0,0), ImVec2(1,1), ImColor(255,255,255,255), ImColor(0,0,0,0));
    ImGui::End();
  }

  if (show_auxHeatmap_window) {
    ImGui::Begin("AUX HEATMAP", &show_auxHeatmap_window);
      // Adjust the image to the window
      auto window_size = ImGui::GetWindowSize();
      auto image_size = ImVec2(window_size.x - 15, window_size.y - 50);
      // Draw image
      ImGui::Image((void*)((intptr_t)auxHeatmapTexture), image_size, ImVec2(0,0), ImVec2(1,1), ImColor(255,255,255,255), ImColor(0,0,0,0));
    ImGui::End();
  }

  if (show_control_window) {
    ImGui::Begin("CONTROLS", &show_control_window);
      ImGui::SliderInt("GC ACTION", &paddles->GCActionSpeed, 0, 128);                    // JOYSTICK Action Speed
      ImGui::SliderInt("GC RELEASE", &paddles->GCReleaseSpeed, 0, 128);                  // JOYSTICK Release Speed
      ImGui::Separator();
      ImGui::SliderFloat("SCALE", &screenScale, 1, 4, "%.1f");
      if (ImGui::SliderInt("VOLUME", &volume, 0, 127)) speaker->setVolume(volume);
      ImGui::SameLine();
      if (ImGui::Checkbox("MUTE", &muted)) speaker->toggleMute();
      ImGui::Separator();
      if (ImGui::SliderFloat("SPEED", &speed, .0f, 200, "%.4f MHz", ImGuiSliderFlags_Logarithmic));
      ImGui::SameLine();
      if (ImGui::Button("NORMAL")) speed = 1.023f;
      ImGui::Checkbox("PAUSE", &paused);
      ImGui::SameLine();
      if (ImGui::Button("STEP")) {
        paused = true;
        cpu->state = step;
      }
      ImGui::Separator();
      if (ImGui::Button("RESET"))
        cpu->RST();
      ImGui::SameLine();
      if (ImGui::Button("POWER CYCLE")) {
        cpu->RST();                                                             // do a cold reset
        mmu->init();
        mmu->ram[0x3F4] = 0;                                                    // unset the Power-UP byte
      }
    ImGui::End();
  }

  static std::string floppy1="", floppy2="";

  fileDialog1.Display();
  if (fileDialog1.HasSelected()) {
    floppy1 = fileDialog1.GetSelected().filename().string();
    disk->load((char*)fileDialog1.GetSelected().string().c_str(), 0);
    fileDialog1.ClearSelected();
  }

  fileDialog2.Display();
  if (fileDialog2.HasSelected()) {
    floppy2 = fileDialog2.GetSelected().filename().string();
    disk->load((char*)fileDialog2.GetSelected().string().c_str(), 1);
    fileDialog2.ClearSelected();
  }

  if (show_disks_window) {
    ImGui::Begin("DISK ][", &show_disks_window);

      if (ImGui::Button("LOAD FLOPPY #1")) {
        fileDialog1.SetTitle("Insert floppy into drive #1");
        fileDialog1.SetTypeFilters({ ".nib", ".dsk", ".woz" });
        fileDialog1.Open();
      }
      ImGui::SameLine();
      if (ImGui::Button("SAVE FLOPPY #1")) {
        disk->save(0);
      }
      ImGui::SameLine();
      if (ImGui::Button("EJECT FLOPPY #1")) {
        disk->eject(0);
      }
      ImGui::Text("Floppy #1 : %s", disk->unit[0].fileName);
      ImGui::Text("Read %s", disk->unit[0].readOnly ? "Only" : "and Write");
      ImGui::Text("Status : ");
      ImGui::SameLine();
      if (disk->unit[0].motorOn) {
        if (disk->unit[0].writeMode)
          ImGui::Text("writting");
        else
          ImGui::Text("reading");
      }
      else
        ImGui::Text("idle");

      ImGui::Separator();

      if (ImGui::Button("LOAD FLOPPY #2")) {
        fileDialog2.SetTitle("Insert floppy into drive #2");
        fileDialog2.SetTypeFilters({ ".nib", ".dsk", ".woz" });
        fileDialog2.Open();
      }
      ImGui::SameLine();
      if (ImGui::Button("SAVE FLOPPY #2")) {
        disk->save(1);
      }
      ImGui::SameLine();
      if (ImGui::Button("EJECT FLOPPY #2")) {
        disk->eject(1);
      }
      ImGui::Text("Floppy #2 : %s", disk->unit[1].fileName);
      ImGui::Text("Read %s", disk->unit[1].readOnly ? "Only" : "and Write");
      ImGui::Text("Status : ");
      ImGui::SameLine();
      if (disk->unit[1].motorOn) {
        if (disk->unit[1].writeMode)
          ImGui::Text("writting");
        else
          ImGui::Text("reading");
      }
      else
        ImGui::Text("idle");

    ImGui::End();
  }

  if (show_info_window) {
    ImGui::Begin("INFO", &show_info_window);
      // Display FPS
      ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      ImGui::Separator();
      ImGui::Text("KEY   : %02X", mmu->KBD);
      ImGui::Separator();
      ImGui::Checkbox("TEXT", &mmu->TEXT);
      ImGui::Checkbox("MIXED", &mmu->MIXED);
      ImGui::Checkbox("PAGE2", &mmu->PAGE2);
      ImGui::Checkbox("HIRES", &mmu->HIRES);
      ImGui::Checkbox("DHIRES", &mmu->DHIRES);
      ImGui::Checkbox("COL80", &mmu->COL80);
      ImGui::Separator();
      ImGui::Text("Language Card readable : %s", mmu->LCRD  ? "True" : "False");
      ImGui::Text("Language Card writable : %s", mmu->LCWR  ? "True" : "False");
      ImGui::Text("Language Card bank 2   : %s", mmu->LCBK2 ? "Enabled" : "Disabled");
      ImGui::Text("Language Card prewrite : %s", mmu->LCWFF ? "On" : "Off");
      ImGui::Separator();
      ImGui::Text("RAMRD      : %s", mmu->RAMRD      ? "On" : "Off");
      ImGui::Text("RAMWRT     : %s", mmu->RAMWRT     ? "On" : "Off");
      ImGui::Text("ALTZP      : %s", mmu->ALTZP      ? "On" : "Off");
      ImGui::Text("STORE80    : %s", mmu->STORE80    ? "On" : "Off");
      ImGui::Text("ALTCHARSET : %s", mmu->ALTCHARSET ? "On" : "Off");
      ImGui::Text("INTCXROM   : %s", mmu->INTCXROM   ? "On" : "Off");
      ImGui::Text("SLOTC3ROM  : %s", mmu->SLOTC3ROM  ? "On" : "Off");
      ImGui::Separator();
      ImGui::Text("VERTBLANK  : %s", mmu->VERTBLANK  ? "On" : "Off");
    ImGui::End();
  }

  if (show_cpu_window) {
    char buffer[1000];
    cpu->getRegs(buffer);
    ImGui::Begin("REGISTERS", &show_cpu_window);
      ImGui::Text("%s", buffer);
      // ImGui::Text("TICK : %lld", ticks);
    ImGui::End();
  }

  static char buffer[2000];
  static char addressString[5];
  static unsigned long int address = 0xFF00;
  static bool followPC = true;

  if (show_code_window) {
    ImGui::Begin("CODE", &show_code_window);
      if( ImGui::InputTextWithHint("", "address in hex", addressString, IM_ARRAYSIZE(addressString))) {
        address = strtoul(addressString, NULL, 16);
        cpu->getCode(address, buffer, 2000, 20);
      }
      ImGui::SameLine();
      ImGui::Checkbox("PC", &followPC);
      if (followPC)
        cpu->getCode(cpu->getPC(), buffer, 2000, 20);
      ImGui::Text("%s", buffer);
    ImGui::End();
  }

  if (show_editor_window) {
    ImGui::Begin("EDITOR", &show_editor_window, ImGuiWindowFlags_HorizontalScrollbar);
      ImGui::SetWindowSize(ImVec2(100, 200), ImGuiCond_FirstUseEver);
      ImGui::Separator();
      if (ImGui::Button("NEW & RUN SELECTION")) {
        if (editor.HasSelection()) {
          std::string program = "NEW\n";
          program += editor.GetSelectedText();
          program += "\nRUN\n";
          int c = 0;
          while (program[c]) {                                                  // all chars until ascii NUL
            mmu->KBD = program[c++] | 0x80;                                          // set bit7
            if (mmu->KBD == 0x8A) mmu->KBD = 0x8D;                                        // translate Line Feed to Carriage Ret
            cpu->exec(500000);                                                   // give cpu (and applesoft) some cycles to process each char
          }
        }
      }
      ImGui::SameLine();
      if (ImGui::Button("PASTE SELECTION")) {
        if (editor.HasSelection()) {
          std::string program = "\n";
          program += editor.GetSelectedText();
          program += "\n";
          int c = 0;
          while (program[c]) {                                                  // all chars until ascii NUL
            mmu->KBD = program[c++] | 0x80;                                          // set bit7
            if (mmu->KBD == 0x8A) mmu->KBD = 0x8D;                                        // translate Line Feed to Carriage Ret
            cpu->exec(500000);                                                   // give cpu (and applesoft) some cycles to process each char
          }
        }
      }
      ImGui::SameLine();
      if (ImGui::Button("SEND RESET"))
        cpu->RST();
      ImGui::Separator();

      auto cpos = editor.GetCursorPosition();
      ImGui::Text("%4d/%-4d %4d lines | %s | %s | %s | %s",
        cpos.mLine + 1,
        cpos.mColumn + 1,
        editor.GetTotalLines(),
        editor.IsOverwrite() ? "Ovr" : "Ins",
        editor.CanUndo() ? "*" : " ",
        editor.GetLanguageDefinition().mName.c_str(),
        fileToEdit.c_str());

      ImGui::Separator();
      editor.Render("TextEditor");
    ImGui::End();
  }

  if (show_crt_window) {
    ImGui::Begin("CRT", &show_crt_window);
      // Adjust the window to the image
      auto image_size = ImVec2(280 * screenScale, 192 * screenScale);
      ImGui::SetWindowSize(ImVec2(280 * screenScale + 16, 192 * screenScale + 50));
      // Draw image
      ImGui::Image((void*)((intptr_t)screenTexture), image_size, ImVec2(0,0), ImVec2(1,1), ImColor(255,255,255,255), ImColor(0,0,0,0));
      // get inputs
      CRT_is_focused = ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows);
    ImGui::End();
  }

  return 1;
}


int Gui::render() {
  // crt
  if (video->gfxmode == 80) {
    glBindTexture(GL_TEXTURE_2D, screenTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 560, 384, 0, GL_RGBA, GL_UNSIGNED_BYTE, video->screenPixels80);
  }
  else {
    glBindTexture(GL_TEXTURE_2D, screenTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 280, 192, 0, GL_RGBA, GL_UNSIGNED_BYTE, video->screenPixels);
  }
  // RAM heatmap
  glBindTexture(GL_TEXTURE_2D, ramHeatmapTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, video->ramHeatmap);

  // AUX heatmap
  glBindTexture(GL_TEXTURE_2D, auxHeatmapTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, video->auxHeatmap);

  // Rendering
  ImGui::Render();

  glViewport(0, 0, (int)ImGui::GetIO().DisplaySize.x, (int)ImGui::GetIO().DisplaySize.y);

  glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
  glClear(GL_COLOR_BUFFER_BIT);
  ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
  SDL_GL_SwapWindow(wdo);

  if (++frameNumber >= 60) frameNumber = 0;                                     // reset to zero every second
  return 1;
}


Gui::~Gui() {
  ImGui_ImplOpenGL2_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
  SDL_GL_DeleteContext(gl_context);
  SDL_DestroyWindow(wdo);
  SDL_AudioQuit();
  SDL_Quit();
}
