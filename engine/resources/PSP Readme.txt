Version 1.2h By SamuraiX

New Feature:
-----> New High Score file auto save and load.
-----> OpenBOR engine has been updated to v2.0047.

Updates:
-----> video.txt update now supports one more resolution mode.
-----> video.txt support for controllable levelorder
-----> Logger now prints out memory usuage.
-----> More details added for logging on LevelLoader.
-----> Updated timing within engine to match DOS speed. (gameplay is faster now)

Bug fixes:
-----> High Score data curroption between games.
-----> Fixed video mode bug introduced from version 1.2c
-----> user controlled (x)icons, (x)life & timeloc[x] renabled and fixed to support mods. 
-----> Fixed fadeout from speeding up when killing bosses.
-----> Fixed Stage Complete from not being centered on screen.


Usage:
- Images Directory and filemenu.png are necessary for BOR.  Place them in the same directory as eboot.
- Beats Of Rage Sites I hang out:  [url]http://senileteam.com/[/url] [url]http://openbor.net/[/url] [url]http://www.borgeneration.com/[/url] [url]http://borrevolution.vg-network.com/[/url]
- Download and save in 'Paks'Directory.
- Startup Game and select the *.pak file you want to play and Press 'X'.

------------------------------------------------------------------------
Version 1.2 By SamuraiX

Bug fixes & New Features.  Will Only release OpenBOR from now on as the performance is identical to my last release of Beats Of Rage.


Updates:
-----> Runs at full 60 fps @ 333 and 48~58 fps @ 222!
-----> Bug Fix when attacking rider crashed PSP.
-----> Control bug Fix I introduced during Pause Menu (This will overwrite settings.sav) 
-----> Bug fix Hall Of Fame.
-----> Use the new filemenu.png as it will be using more inputs.
-----> All Necessary Directories will now be created automatically.


New Features:
-----> Screen Shots will now save under PSP/Photo directory with Name of Mod.
-----> New Frames Per Second Counter under PSP Options.
-----> New video.txt support for Mod Creaters (Allows Full WideScreen Resolution (480x272))
-----> Modders instructions for full screen support: http://www.borgeneration.net/forum/viewtopic.php?t=7


Future Releases:
- Wireless 2 Player support (Currently In progress)
- Update OpenBOR to latest version (2.0046)


Usage:
- Images Directory and filemenu.png are necessary for BOR.  Place them in the same directory as eboot.
- Beats Of Rage Sites I hang out:  [url]http://senileteam.com/[/url] [url]http://openbor.net/[/url] [url]http://www.borgeneration.com/[/url] [url]http://borrevolution.vg-network.com/[/url]
- Download and save in 'Paks'Directory.
- Startup Game and select the *.pak file you want to play and Press 'X'.

------------------------------------------------------------------------
Version 1.1 By SamuraiX

Mostly Bug fixes in this release, code optimizations and 1 small new feature.  Lastly opening 
up the source for anyone who would like to add onto BOR/OpenBOR. While I am still working on the 
Wifi 2 Player, Feel free to implement GPU support as I am very new at video manipulation.  Lastly, 
The update to the Game Save will currupt the old game saves.  Delete them or finish your game then 
upgrade.


Updates:
-----> Images will now save as PNG instead of PCX.
-----> Update to Game Save.  It will now save score as well.
-----> Code Optimization removed alot of unnecessary code that the PSP would not use.
-----> Music Pauses in Pause Menu.
-----> Use the new filemenu.png since I removed Huge font code I was using to generate psp symbols.


Issues:
-Sleep Mode is not supported Yet.


Future Releases:
-Wireless 2 Player support
-Video GPU Support


Usage:
- Images and Saves directories are necessary for BOR.  Place them in the same directory as eboot.
- Recommended all mods go in 'Paks' dir but shoudn't matter... 

------------------------------------------------------------------------
Version 1.0 By SamuraiX

With all the significant changes I've made since I started working on BOR.  I believe
that the BOR deserves a whole number.  So here it is 1.0


Updates:
-NEW save/load Levels option.
-----> BOR will auto-save the begining of every level.
-----> Load level thru menu and continue from begining of level.

-Final changes to Memory Thresholds.  
-----> Default clears memory after every level with large paks introduces longer loadtimes.
-----> Enhanced clears memory only if 5 MBytes of memory is left.

-Preparing code for PSP 2 player mode.


Issues:
-Sleep Mode is not supported Yet.
-OpenBOR Hall of Fame glitch in main menu.


Future Releases:
-Wireless 2 Player support
-Video GPU Support


Usage:
- Images and Saves directories are necessary for BOR.  Place them in the same directory as eboot.
- Recommended all mods go in 'Paks' dir but shoudn't matter... 
...Only tested save/load feature against 'Paks' dir but should still work with any choosen dir.

------------------------------------------------------------------------
Version 0.09 By SamuraiX

I will now include two versions of BOR. The original Beats of Rage V1.0029
and the NEW OpenBor V2.0045.  I suggest to use original BOR for mods that 
don't require enhanced features that OpenBOR provides because 333 is needed
for smooth video playback.  If mod is not compatible with BOR then use OpenBOR.  
I do plan on upgrading both versions to use GPU in the future, so we can default
to 222Mhz on OpenBOR.

Between the both of them you should be able to play any mod.

Updates:
-Compatibility with 2.0 TIFF
-New File Logger(BeatsOfRageLog.txt gets created when ever an error occurs)
-Lots of Memory Fixes/stabilizing here and there.
-Keep version identical to OpenBOR.
-OpenBOR defaults to 333 MHz (Keep Video Smooth) since its much more complex than bor.

Issues:
-Sleep Mode is not supported Yet.
-OpenBOR Hall of Fame glitch in main menu.


Usage:
-Place filemenu.png in the same directory as eboot.

------------------------------------------------------------------------
Version 0.08 By SamuraiX

Updates:
-New pak file selector!
-Updated Memory Thresholds.
-BOR is completely PSPSDK!

Issues:
-Sleep Mode is not supported Yet.

Notes:
-Place Pak File anywhere You want and select it!
-This version will overwrite/create new settings.sav file.

------------------------------------------------------------------------
Version 0.07 By SamuraiX

Updates:
-New Memory Management System (Should be able to play most if not all bor mods).
-New Memory Thresholds option under PSP Options. (Defaults to High Any issues try Low).
-New Load Time option under PSP Options to help load times. ("Enhanced" will switch CPU to 333 then back to 222)
-New Analog Pad Support.

Fixes:
-Out Of Memory while loading levels refer to (Memory Thresholds Options).

Issues:
-End Of Level Stuck Bug still exists in some games.  This one is tricky!

Usage:
-pak file must be renamed to "bor.pak" lowercase letters.
-Delete settings.sav and allow BOR to create new one for this release!!!!!!!!!
-Use DC, PS2, Xbox and PC mod paks in that order due to PSP Memory limits.

------------------------------------------------------------------------
Version 0.065 By SamuraiX

Fixes:
-Compatibility with 2.0+ (Eloader: 0.95 Patch: 22 Feb 06)
-End Of Level Stuck Bug
-Enemies now walk into level versus falling from sky.

Usage:
-pak file must be renamed to "bor.pak" lowercase letters.
-Use supplied settings.sav
-Use DC, PS2, Xbox and PC mod paks in that order due to PSP Memory limits.

------------------------------------------------------------------------
Version 0.06 By SamuraiX

Updates:
-Finished Widescreen mode.
-Updated font spacing for funky fonts.
-Added Available Memory to PSP Options.
-Added Sound Effects to Pause Screen.
-Defaults to Widescreen Mode.
-Defaults to 222 Mhz since there is no slowdown.

Fixes:
-Crash from using in-game exit.

Known Issues:
-None that I know of...

Next Release:
-Pak File Selector.

------------------------------------------------------------------------
Version 0.05 By SamuraiX

Updates:
-Added Virtual File Management. (Ported from PS2 BOR)

Fixes:
-Load Times are incredibly Quick!!!

Known Issues:
-Large Paks 40 MBytes or Higher cause PSP crash.
-PSP Video Mode Widescreen. (Still not finished)

Future Releases...
-Pak file selector.
-Video Modes.
-More Optimizations....
-One SDL Conversion Left

------------------------------------------------------------------------
Version 0.04 By SamuraiX

Updates:
-Updated Sound Engine/Processing. (Now runs on PSPSDK, Synced Up and Sounds Much Better!)
-Updated Image/Video Processing. (Now runs on PSPSDK)

Known Issues:
-Long Load Times.           (Working On It!)
-PSP Video Mode Widescreen. (Still not finished)

Future Releases...
-Pak file selector.
-Video Modes.
-More Optimizations....
-One SDL Conversion Left

------------------------------------------------------------------------
Version 0.03 By SamuraiX

Updates and New Features:
-Updated Charge Indicator, Time Indicator (Now shows status of AC/Battery Source)
-Updated Sprite Engine. (Ported from Dreamcast BOR uses ALOT less memory to display sprites)
-Added PSP Video Mode Widescreen. (Must restart BOR for changes to take effect. Work in progress!)

Bug Fixes:
-Hopefully no more level crashes!!!

Future Releases...
-Pak file selector.
-Video Modes.
-More Optimizations....


------------------------------------------------------------------------
Version 0.02 By SamuraiX

Ported from Nazo SDL Version 0.01
Powered By PSPSDK && PSPSDL
Icon0.png and Pic1.png from Eli 

New Features:
-Added new screen shot capability.
-Added new menu option for PSP.
-Added new CPU Speed, Bus Speed, Charge Indicator, Time Indicator.
-Added new Save/Load file settings.

Bug Fixes:
-Level change crash!!!

Future Releases...
-Pak file selector.
-Video Modes.
-Optimizations....


Know Issues...  No longer compatible with PSPE!


------------------------------------------------------------------------


0.01 1st release

Beats of Rage for PSP (BOR/PSP)

Download Beats of Rage for PC from http://www.segaforums.com/senileteam/bor.php
extract bor.pak into same folder on EBOOT.PBP
if you want to play mod, rename to 'bor.pak' and put it.
if you run on pspe, use 0.09 or more.

I only have 32MB memory stick but BOR data is 62MB,
so I checked Alien Vs. Predator MOD


