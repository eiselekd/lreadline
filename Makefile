LUA_VERSION?=5.2
LUA_CFLAGS=-g $(shell pkg-config lua$(LUA_VERSION) --cflags)
LUA_LDFLAGS=-g $(shell pkg-config lua$(LUA_VERSION) --libs)

core/lreadline.so: lreadline.c
	mkdir -p lreadline
	gcc $(LUA_CFLAGS) -Og -fpic -c -o lreadline.o lreadline.c
	gcc $(LUA_LDFLAGS)  -shared -fpic -o lreadline/core.so lreadline.o

clean:
	rm -rf *.o *.so

test:
	lua$(LUA_VERSION) test.lua
