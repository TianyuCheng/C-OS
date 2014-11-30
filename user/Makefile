PROGS = shell ls shutdown echo cat login sha1hash su chmod cp test

all : $(PROGS)

-include ../common.mak

login : CFILES=login.c libc.c heap.c sha1.c err.c

shell : CFILES=shell.c libc.c heap.c err.c

echo : CFILES=echo.c libc.c heap.c

ls : CFILES=ls.c libc.c heap.c err.c

shutdown : CFILES=shutdown.c libc.c heap.c

cat : CFILES=cat.c libc.c heap.c err.c

sha1hash : CFILES=sha1hash.c libc.c heap.c sha1.c

su : CFILES=su.c libc.c heap.c sha1.c

test: CFILES=test.c libc.c heap.c err.c

chmod: CFILES=chmod.c libc.c heap.c err.c

cp: CFILES=cp.c libc.c heap.c err.c

$(PROGS) : % : Makefile $(OFILES)
	ld -N -m elf_i386 -e start -Ttext=0x80000000 -o $@ $(OFILES)

clean ::
	rm -f $(PROGS)
