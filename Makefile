##### Build defaults #####
TARGET =            lua_parson.so
PREFIX =            /usr/local
#CFLAGS =            -g -Wall -pedantic -fno-inline
CFLAGS =            -O3 -Wall -pedantic -DNDEBUG
LUA_PARSON_CFLAGS =      -fpic
LUA_PARSON_LDFLAGS =     -shared
LUA_INCLUDE_DIR =   $(PREFIX)/include

BUILD_CFLAGS =      -I$(LUA_INCLUDE_DIR) $(LUA_PARSON_CFLAGS)
OBJS =              parson.o lparson.o

.PHONY: all clean

.c.o:
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(BUILD_CFLAGS) -o $@ $<

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(LUA_PARSON_LDFLAGS) -o $@ $(OBJS)

clean:
	rm -f *.o $(TARGET)
