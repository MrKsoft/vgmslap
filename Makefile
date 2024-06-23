.EXTENSIONS:
.EXTENSIONS: .obj .lib .c .h

TARGET  = vgmslap.exe

OBJFILES	= vgmslap.obj ./deps/zlib.lib

CFLAGS  = -bt=dos -mm -wx -otexan

all: $(TARGET) .SYMBOLIC

$(TARGET): $(OBJFILES)
    wcl $(CFLAGS) -fe=$(TARGET) $(OBJFILES)

clean: .SYMBOLIC
    rm -f *.obj
    rm -f *.o
    rm -f $(TARGET)
    @echo Cleanup complete!
    
.c.obj:
    wcc $(CFLAGS) -fo=.obj $^&
