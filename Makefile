.EXTENSIONS:
.EXTENSIONS: .obj .lib .c .h

TARGET  = vgmslap.exe

OBJFILES	= vgmslap.obj opl.obj playlist.obj settings.obj timer.obj txtgfx.obj txtmode.obj ui.obj vgm.obj ./deps/zlib.lib

CFLAGS  = -bt=dos -mm -wx -otexan

all: $(TARGET) .SYMBOLIC

$(TARGET): $(OBJFILES)
    wcl $(CFLAGS) -fe=$(TARGET) $(OBJFILES)

clean: .SYMBOLIC
    rm -f *.obj
    rm -f *.o
    rm -f *.err
    rm -f $(TARGET)
    @echo Cleanup complete!
    
.c.obj:
    wcc $(CFLAGS) -fo=.obj $^&
