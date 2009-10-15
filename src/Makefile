# Makefile for MyQQ

CC=		gcc
WINDRES=	windres
RM =		del
CFLAGS=		-c -Wall -O -Werror -I"../lib/pthread"
LDFLAGS=	-L"../lib/pthread" -lpthreadGC2 -lws2_32
LD=		gcc

RES=		myqq_private.o
RES_SRC=	..\myqq_private.rc
RES_ICON=	..\myqq.ico
OBJS=		qqsocket.o qqcrypt.o md5.o debug.o qqclient.o memory.o config.o \
		packetmgr.o qqpacket.o prot_login.o protocol.o prot_misc.o \
		prot_im.o prot_user.o list.o buddy.o group.o qun.o prot_group.o \
		prot_qun.o prot_buddy.o loop.o utf8.o myqq.o util.o crc32.o qqconn.o \
		$(RES)

TARGET=	..\myqq.exe

all: $(TARGET)
	@echo done.

$(TARGET): $(OBJS)
	$(LD) $(OBJS) $(RES) $(LDFLAGS) -o $@

.c.o:
	$(CC) $(CFLAGS) -o $@ $<

$(RES): $(RES_SRC) $(RES_ICON)
	$(WINDRES) -i  $(RES_SRC) -o $(RES)

.PHONY: clean cleanobj
clean:
	$(RM) *.o
	$(RM) $(TARGET)

cleanobj:
	$(RM) *.o
