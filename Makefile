##### Build defaults #####
CC = gcc -std=gnu99
TARGET_SO =         lua_parson.so
TARGET_A  =         liblua_parson.a

# CFLAGS =            -g -Wall -pedantic -fno-inline -Iparson
CFLAGS =            -O3 -Wall -pedantic -DNDEBUG -Iparson

# add submodule dir to make
VPATH=parson

AR= ar rc
RANLIB= ranlib

SHAREDDIR = .sharedlib
STATICDIR = .staticlib

OBJS =              parson.o lparson.o

SHAREDOBJS = $(addprefix $(SHAREDDIR)/,$(OBJS))
STATICOBJS = $(addprefix $(STATICDIR)/,$(OBJS))

DEPS := $(SHAREDOBJS + STATICOBJS:.o=.d)

.PHONY: all clean test

$(SHAREDDIR)/%.o: %.c
	@[ ! -d $(SHAREDDIR) ] & mkdir -p $(SHAREDDIR)
	$(CC) -c $(CFLAGS) -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -o $@ $<

$(STATICDIR)/%.o: %.c
	@[ ! -d $(STATICDIR) ] & mkdir -p $(STATICDIR)
	$(CC) -c $(CFLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -o $@ $<

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
