TARGET = out/pchat
o = src/libco/libco.o src/common/cfgfiles.o src/common/chanopt.o src/common/ctcp.o src/common/dcc.o src/common/history.o src/common/ignore.o src/common/inbound.o src/common/modes.o src/common/msproxy.o src/common/network.o src/common/notify.o src/common/outbound.o src/common/plugin.o src/common/plugin-timer.o src/common/proto-irc.o src/common/server.o src/common/servlist.o src/common/text.o src/common/tree.o src/common/url.o src/common/userlist.o src/common/util.o src/common/wdkutil.o src/common/xchat.o 

CC = gcc
CXX = g++

HAVE_GTK = 1
#HAVE_WXWIDGETS = 0
HAVE_OPENSSL = 1
#todo compile translations to include with the installation process
LIBS = -pthread
DEFINES = -I. -DLINUX
GTK_LIBS = `pkg-config --libs gtk+-2.0`
GTK_CFLAGS = `pkg-config --cflags gtk+-2.0`
OPENSSL_CFLAGS = `pkg-config --cflags openssl`
LDFLAGS = -L. -s

ifeq ($(HAVE_GTK), 1)
   o += src/fe-gtk/about.o src/fe-gtk/ascii.o src/fe-gtk/banlist.o src/fe-gtk/chanlist.o src/fe-gtk/chanview.o src/fe-gtk/custom-list.o src/fe-gtk/dccgui.o src/fe-gtk/editlist.o src/fe-gtk/fe-gtk.o src/fe-gtk/fkeys.o src/fe-gtk/gtkutil.o src/fe-gtk/ignoregui.o src/fe-gtk/joind.o src/fe-gtk/maingui.o src/fe-gtk/menu.o src/fe-gtk/notifygui.o src/fe-gtk/palette.o src/fe-gtk/pixmaps.o src/fe-gtk/plugingui.o src/fe-gtk/plugin-tray.o src/fe-gtk/rawlog.o src/fe-gtk/search.o src/fe-gtk/servlistgui.o src/fe-gtk/setup.o src/fe-gtk/sexy-spell-entry.o src/fe-gtk/textgui.o src/fe-gtk/urlgrab.o src/fe-gtk/userlistgui.o src/fe-gtk/xtext.o
   LIBS += $(GTK_LIBS)
   DEFINES += $(GTK_CFLAGS)
endif

ifeq ($(HAVE_OPENSSL), 1)
   o += src/common/ssl.o
   LIBS += -lssl
   DEFINES += $(OPENSSL_CFLAGS) -DUSE_OPENSSL
endif

ifneq ($(V),1)
   Q := @
endif


CXXFLAGS = -std=gnu++0x -fPIC -w -O3 -I. -lstdc++
CFLAGS = -std=gnu++0x -fPIC -w -O3 -I. -lstdc++

all: $(TARGET)

$(TARGET): $(o)
	$(Q)$(CXX) -o $@ $(o) $(LIBS) $(LDFLAGS)
	@$(if $(Q), $(shell echo echo llvm-ld $@),) 
	



%.o: %.cpp
	$(Q)$(CC) $(CXXFLAGS) $(DEFINES) -c -o $@ $<
	@$(if $(Q), $(shell echo echo CC $<),)

clean:
	rm -f src/common/*.o
	rm -f src/libco/*.o
	rm -f src/fe-gtk/*.o
	
	rm -f out/$(TARGET)

install:
	install -D -m 755 out/pchat $(DESTDIR)$(prefix)/bin/pchat
	install -D -m 644 src/fe-gtk/pixmaps/PChat.png $(DESTDIR)$(prefix)/share/pixmaps/PChat.png
	install -D -m 644 data/pchat.desktop $(DESTDIR)$(prefix)/share/applications/pchat.desktop

uninstall:
	rm $(DESTDIR)$(prefix)/bin/pchat
	rm $(DESTDIR)$(prefix)/share/pixmaps/PChat.png
	rm $(DESTDIR)$(prefix)/share/applications/pchat.desktop

.PHONY: all install uninstall clean
