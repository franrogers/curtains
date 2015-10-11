PREFIX=/usr/local

all: curtains

clean:
	-rm *.o curtains

install:
	install curtains $(PREFIX)/bin/curtains

curtains: curtains.c
	cc -Wall -Werror curtains.c -framework CoreFoundation -framework CoreGraphics -o curtains
