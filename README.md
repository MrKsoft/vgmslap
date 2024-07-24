# VGMSlap
VGMSlap (VGM Silly Little Adlib Player) is an open-source, MS-DOS based player for Yamaha OPL1/2/3 Video Game Music (VGM) files.  It features a visually pleasing display of each synthesizer channel's parameters, GD3 tags, and simple playlist support.

![vgmslap-r3](https://github.com/user-attachments/assets/41e2c38b-a961-4b9f-a867-6ee80d9ccc11)

See the program readme (VGMSLAP.TXT) for additional info.


## Building

VGMSlap is written with **Open Watcom C 2.0** in mind.  Assuming you have your Watcom environment variables and PATH set correctly, just run `wmake` from the source directory to create `vgmslap.exe`.

## Licenses

The VGMSlap software and code itself is licensed under the **MIT License** (see LICENSE).

This VGMSlap source distribution also incorporates a prebuilt zlib 1.2.11, the last version I could get to successfully compile for DOS on Watcom, which is licensed under the **zlib license** (see zlib/LICENSE).
