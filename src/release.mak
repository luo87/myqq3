# Makefile for MyQQ

CC=		gcc
CFLAGS=		-c -Wall -O -Werror -s
LDFLAGS=	-lpthreadGC2 -lws2_32 -s -I"../lib/pthread" -L"../lib/pthread"
LD=		gcc

OBJS=		qqsocket.o qqcrypt.o md5.o debug.o qqclient.o memory.o config.o packetmgr.o qqpacket.o \
		prot_login.o protocol.o prot_misc.o prot_im.o prot_user.o list.o buddy.o group.o qun.o \
		prot_group.o prot_qun.o prot_buddy.o loop.o utf8.o myqq.o util.o crc32.o qqconn.o

TARGET=	../myqq.exe

all: $(TARGET)
	@echo done.

$(TARGET): $(OBJS)
	$(LD) $(OBJS) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: clean cleanobj
clean:
	rm -f *.o
	rm -f $(TARGET)

cleanobj:
	rm -f *.o