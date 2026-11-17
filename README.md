
# WORK IN PROGRESS


# reinette IIe
### reinette gets enhanced !
![screenshots](animated.gif)

uses [SDL2](https://www.libsdl.org/index.php), [imgui](https://github.com/ocornut/imgui), [imgui Memory editor](https://github.com/ocornut/imgui_club), [ImGui Color Text Edit](https://github.com/BalazsJako/ImGuiColorTextEdit) and [imgui-filebrowser](https://github.com/AirGuanZ/imgui-filebrowser).

## Disk image formats

| format | size | load | save |
|--------|------|------|------|
| `.nib` (raw GCR nibble) | 232960 | raw | raw dump |
| `.dsk` (DOS 3.3 order)  | 143360 | nibblelized on load | denibblelized on save |

`.dsk` images are GCR 6-and-2 encoded into the internal nibble buffer on load (same
layout as the classic `dsk2nib` utility), and decoded back on save. Saving a `.dsk`
is strict: every sector of every track must decode with a valid address and data
checksum, otherwise the save is refused and the file is left untouched - a damaged
or non-standard track is never silently written back.

Windows binaries : https://github.com/ArthurFerreira2/reinetteIIe/releases/tag/0.8.1

\
\
\
*simplicity is the ultimate sophistication*
