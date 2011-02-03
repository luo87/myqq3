DEPENDPATH = ../src
INCLUDEPATH = ../src

# Input
HEADERS += ../src/buddy.h \
           ../src/config.h \
           ../src/crc32.h \
           ../src/debug.h \
           ../src/group.h \
	   #../src/libqq.h \
           ../src/list.h \
           ../src/loop.h \
           ../src/md5.h \
           ../src/memory.h \
           ../src/packetmgr.h \
           ../src/protocol.h \
           ../src/qqclient.h \
           ../src/qqconn.h \
           ../src/qqcrypt.h \
           ../src/qqdef.h \
           ../src/qqpacket.h \
           ../src/qqsocket.h \
           ../src/qun.h \
           ../src/utf8.h \
	   ../src/util.h
	   #../src/webqq.h
SOURCES += ../src/buddy.c \
           ../src/config.c \
           ../src/crc32.c \
           ../src/debug.c \
           ../src/group.c \
	   #../src/libqq.c \
           ../src/list.c \
           ../src/loop.c \
           ../src/md5.c \
           ../src/memory.c \
           ../src/myqq.c \
           ../src/packetmgr.c \
           ../src/prot_buddy.c \
           ../src/prot_group.c \
           ../src/prot_im.c \
           ../src/prot_login.c \
           ../src/prot_misc.c \
           ../src/prot_qun.c \
           ../src/prot_user.c \
           ../src/protocol.c \
           ../src/qqclient.c \
           ../src/qqconn.c \
           ../src/qqcrypt.c \
           ../src/qqpacket.c \
           ../src/qqsocket.c \
           ../src/qun.c \
           ../src/utf8.c \
	   ../src/util.c
	   #../src/webqq.c
