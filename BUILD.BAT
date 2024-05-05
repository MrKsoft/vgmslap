REM Ensure that your Watcom environment variables are already set before running.
del vgmslap.exe vgmslap.obj
WCL -bt=dos -mm -wx -otexan -fe=vgmslap.exe vgmslap.c  ./zlib/zlib.lib