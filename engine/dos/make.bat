@echo off
SET PATH=C:\watcom\BINNT;C:\watcom\BINW;%PATH%
SET WATCOM=C:\watcom
SET EDPATH=C:\watcom\EDDAT
SET INCLUDE=C:\watcom\H;C:\watcom\H\NT;C:\Projects\OpenBoR\;C:\Projects\OpenBoR\DOS;C:\Projects\OpenBoR\source;C:\Projects\OpenBoR\source\ADPCMLIB;C:\Projects\OpenBoR\source\GAMELIB;;C:\Projects\OpenBoR\source\MEMLIB;C:\Projects\OpenBoR\source\PCXLIB;C:\Projects\OpenBoR\source\SCRIPTLIB;C:\Projects\OpenBoR\source\TRACELIB;

echo clean.bat

wasm -3pr -fp3 -mf -bt=dos32a ASMCOPY.ASM
wasm -3pr -fp3 -mf -bt=dos32a VGA.ASM
wasm -3pr -fp3 -mf -bt=dos32a TIMER.ASM
wasm -3pr -fp3 -mf -bt=dos32a RAND32.ASM
wasm -3pr -fp3 -mf -bt=dos32a KEYBOARD.ASM


wcc386 -DDOS -fp3 -3r -bt=dos32a -mf ..\SOURCE\ADPCMLIB\ADPCM.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf ..\SOURCE\GAMELIB\ANIGIF.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf ..\SOURCE\GAMELIB\BITMAP.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf ..\SOURCE\GAMELIB\DRAW.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf ..\SOURCE\GAMELIB\FONT.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf ..\SOURCE\GAMELIB\LOADIMG.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf ..\SOURCE\GAMELIB\PACKFILE.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf ..\SOURCE\GAMELIB\PALETTE.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf ..\SOURCE\GAMELIB\SCREEN.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf ..\SOURCE\GAMELIB\SPRITE.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf ..\SOURCE\GAMELIB\SPRITEQ.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf ..\SOURCE\GAMELIB\SSPRITE.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf ..\SOURCE\GAMELIB\TEXTURE.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf ..\SOURCE\MEMLIB\MEMAGE.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf ..\SOURCE\PCXLIB\SAVEPCX.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf ..\SOURCE\SCRIPTLIB\INSTRUCTION.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf ..\SOURCE\SCRIPTLIB\INTERPRETER.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf ..\SOURCE\SCRIPTLIB\LEXER.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf ..\SOURCE\SCRIPTLIB\LIST.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf ..\SOURCE\SCRIPTLIB\PARSER.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf ..\SOURCE\SCRIPTLIB\PARSERSET.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf ..\SOURCE\SCRIPTLIB\SCRIPTVARIANT.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf ..\SOURCE\SCRIPTLIB\STACK.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf ..\SOURCE\SCRIPTLIB\STACKEDSYMBOLTABLE.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf ..\SOURCE\SCRIPTLIB\SYMBOLTABLE.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf ..\SOURCE\TRACELIB\TRACEMALLOC.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf ..\SOURCE\UTILS.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf CONTROL.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf DOSPORT.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf JOY.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf SBLASTER.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf SOUNDMIX.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf SYSTEM.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf VESA.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf VIDEO.C
wcc386 -DDOS -fp3 -3r -bt=dos32a -mf ..\OPENBORSCRIPT.C

CD ..

wcc386 -DDOS -i=%WATCOM%\h -fp3 -3r -bt=dos32a -mf openbor.c

pause

wlink system dos32a file openbor,.\dos\*.obj

move openbor.exe .\dos\OpenBOR.exe

pause

