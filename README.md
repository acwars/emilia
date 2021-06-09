# About
Simple terminal emulator for Wayland and X11 with OpenGL rendering and minimal dependencies.

**This is roughly alpha quality, expect bugs!**

# Features
* Unicode support
* Text reflow
* 24-bit colors
* Dynamic colors
* All text properties (squiggly underline, blinking, overline etc.)
* Resizable font
* Subpixel antialiasing
* Mouse reporting
* Scrollback
* Mouse text selection
* Clipboard
* Configurable keybindings
* Clickable links, OSC 8 links
* Command history and marks
* kitty image protocol (experimental)

# To-Do
* Ligatures
* Sixel graphics
* All xterm and vte control sequences

# Building
```shell
make
make install
```

#### Dependencies:
* OpenGL >= 2.1
* freetype >= 2.10
* fontconfig
* xkbcommon [wayland]
* utf8proc [optional]
* notify-send [optional]

To build without X11 or Wayland support set ```window_protocol=wayland``` or ```window_protocol=x11``` respectively. With both backends enabled emilia will default to wayland. You can force X11 mode with the ```xorg-only``` option.

To build with debuging symbols set ```mode=debugoptimized```.

# Usage

#### Configuration file
All option can be set in a configuration file or passed as command line arguments. To see all supported options run ```emilia --help```.\
\
Emilia will look for: ```$XDG_CONFIG_HOME/emilia/config``` or ```$HOME/.config/emilia/config```.
For an example configuration file see ```example/config```.

#### Default Keybindings
Keys|Action|
 --- | ---
```Ctrl```+```Shift```+```c```            | Copy to clipboard
```Ctrl```+```Shift```+```x```            | Copy output of last command to clipboard
```Ctrl```+```Shift```+```v```            | Paste from clipboard
```Ctrl```+```Shift```+```=```            | Increase font size
```Ctrl```+```Shift```+```-```            | Decrease font size
```Ctrl```+```Shift```+```Up/Down```      | Scroll
```Ctrl```+```Shift```+```Page Up/Down``` | Scroll by page
```Ctrl```+```Shift```+```Left/Right```   | Jump to previous/next command output or mark
```Ctrl```+```Shift```+```u```            | Enter unicode character by hex code
```Ctrl```+```Shift```+```k```            | Enter vi-like keyboard select mode
```Ctrl```+```Shift```+```d```            | Start new instance in active work directory (set by OSC 7)
```Ctrl```+```Shift```+```F12```          | HTML screen dump
```Ctrl```+```Shift```+```\```            | Pipe to external program
```LMB```                                 | Select text
```RMB```                                 | Change selected region
```Shift```+```LMB```                     | Select text in mouse reporting mode
```Ctrl``` + ```LMB```                    | Open link/Box select

# License
MIT
