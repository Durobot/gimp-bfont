This is an example of using .bfont bitmapped font files (just plain C arrays in self-documented text files really):

![bfont_user_01](img/bfont_user_01.png)

Dependencies: [SDL2](https://www.libsdl.org/download-2.0.php)

To compile on Linux, either

* run ``make`` or
* run ``build.sh``

The actual build command is ``gcc bfont_user.c bfont_user_fonts.c -lSDL2 -o bfont_user`` in both cases.

Building on Windows should not be difficult either, since SDL is cross-platform.
