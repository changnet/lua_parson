##### Build defaults #####
CC = gcc -std=gnu99
TARGET_SO =         lua_parson.so
TARGET_A  =         liblua_parson.a
PREFIX =            /usr/local
#CFLAGS =            -g -Wall -pedantic -fno-inline
CFLAGS =            -O3 -Wall -pedantic -DNDEBUG
LUA_INCLUDE_DIR =   $(PREFIX)/include
AR= ar rcu
RANLIB= ranlib

SHAREDDIR = .sharedlib
STATICDIR = .staticlib

BUILD_CFLAGS =      -I$(LUA_INCLUDE_DIR) $(LUA_PARSON_CFLAGS)
OBJS =              parson.o lparson.o

SHAREDOBJS = $(addprefix $(SHAREDDIR)/,$(OBJS))
STATICOBJS = $(addprefix $(STATICDIR)/,$(OBJS))

DEPS := $(SHAREDOBJS + STATICOBJS:.o=.d)

.PHONY: all clean test

$(SHAREDDIR)/%.o: %.c
	@[ ! -d $(SHAREDDIR) ] & mkdir -p $(SHAREDDIR)
	$(CC) -c $(CFLAGS) -I$(LUA_INCLUDE_DIR) -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -o $@ $<

$(STATICDIR)/%.o: %.c
	@[ ! -d $(STATICDIR) ] & mkdir -p $(STATICDIR)
	$(CC) -c $(CFLAGS) -I$(LUA_INCLUDE_DIR) -MMD -MP -MF"$(@:%.o=%.d)" -o $@ $<

all: $(TARGET_SO) $(TARGET_A)

$(TARGET_SO): $(SHAREDOBJS)
	$(CC) $(LDFLAGS) -shared -o $@ $^

$(TARGET_A): $(STATICOBJS)
	$(AR) $@ $^
	$(RANLIB) $@

test:
	lua test.lua

clean:
	rm -f -R *.o $(TARGET_SO) $(TARGET_A) $(STATICDIR) $(SHAREDDIR)
