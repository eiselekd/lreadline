LUA_VERSION?=5.2
LUA_CFLAGS=-g $(shell pkg-config lua$(LUA_VERSION) --cflags)
LUA_LDFLAGS=-g $(shell pkg-config lua$(LUA_VERSION) --libs)

core/lreadline.so: lreadline.c
	mkdir -p lreadline
	gcc $(LUA_CFLAGS) -O0 -fpic -c -o lreadline.o lreadline.c
	gcc $(LUA_LDFLAGS) -g -shared -fpic -o lreadline/core.so lreadline.o -lreadline

clean:
	rm -rf *.o *.so core/lreadline.so

test: core/lreadline.so
	lua$(LUA_VERSION) test.lua
