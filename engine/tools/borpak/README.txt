borpak v0.3 by Plombo

Original Authors: Luigi Auriemma (borpak.c v0.1), SX (borpak.c v0.2), and 
Joerg-R. Hill (scandir.c). Stristr.c is by multiple authors but it is in the 
public domain.

This version fixes 2 bugs in versions 0.1 and 0.2 that prevented packfile 
building from working on Windows and Linux, respectively. Unlike the other 
versions of borpak, the files created during extraction have lowercase filenames
instead of uppercase; this simulates the behavior of paxplode and creates more 
readable filenames.
