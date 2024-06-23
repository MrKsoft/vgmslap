# VGMSlap
VGMSlap (VGM Silly Little Adlib Player) is an open-source, MS-DOS based player for Yamaha OPL1/2/3 Video Game Music (VGM) files.  It features a visually pleasing display of each synthesizer channel's parameters, GD3 tags, and simple playlist support.

![vgmslap](https://github.com/MrKsoft/vgmslap/assets/10276605/1a042cd7-5fbe-4780-a3a5-d1f6a9e5f0fe)

See the program readme (VGMSLAP.TXT) for additional info.

## Building

VGMSlap is written with **Open Watcom C 2.0** in mind.  Assuming you're on DOS/Windows and you have your Watcom environment variables and PATH set correctly, the following command should build VGMSLAP.EXE:

    WCL -bt=dos -mm -wx -otexan -fe=vgmslap.exe vgmslap.c ./zlib/zlib.lib

You may need to modify this slightly if cross-compiling from Linux (probably just the path to zlib.lib).

## Licenses

The VGMSlap software and code itself is licensed under the **MIT License** (see LICENSE).

This VGMSlap source distribution also incorporates a prebuilt zlib 1.2.11, the last version I could get to successfully compile for DOS on Watcom, which is licensed under the **zlib license** (see zlib/LICENSE).
