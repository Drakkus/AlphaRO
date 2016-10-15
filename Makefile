# $Id: Makefile,v 1.4 2004/02/02 02:37:02 rovert Exp $

CC = gcc -pipe
PACKETDEF = -DPACKETVER=4 -DNEW_006b
#PACKETDEF = -DPACKETVER=3 -DNEW_006b
#PACKETDEF = -DPACKETVER=2 -DNEW_006b
#PACKETDEF = -DPACKETVER=1 -DNEW_006b

PLATFORM = $(shell uname)

ifeq ($(findstring CYGWIN,$(PLATFORM)), CYGWIN)
OS_TYPE = -DCYGWIN
else
OS_TYPE =
endif

CFLAGS = -g -O2 -Wall -I../common $(PACKETDEF) $(OS_TYPE)

MKDEF = CC="$(CC)" CFLAGS="$(CFLAGS)"


all: common/GNUmakefile login/GNUmakefile char/GNUmakefile map/GNUmakefile conf
	@echo -e "/==========================================/"
	@echo -e " "
	@echo -e " ______  __           __                   "
	@echo -e "/\  _  \/\ \         /\ \                  "
	@echo -e "\ \ \ \ \ \ \   _____\ \ \___      __      "
	@echo -e " \ \  __ \ \ \ /\  _  \ \  _  \  / __ \    "
	@echo -e "  \ \ \/\ \ \ \_ \ \ \ \ \ \ \ \/\ \ \.\_  "
	@echo -e "   \ \_\ \_\ \__\ \  __/\ \_\ \_\ \__/.\_\ "
	@echo -e "    \/_/\/_/\/__/\ \  _/ \/_/\/_/\/__/\/_/ "
	@echo -e "                  \ \_\                    "
	@echo -e "                   \/_/                    "
	@echo -e "Alpha rev159v1.2.0 (< ................... >)"
	@echo -e "/==========================================/ "
	@echo
	@echo "Compiling Alpha RO"
	@echo
	@echo -e " -> /common"
	@make --no-print-directory -C common/ $(MKDEF) $@
	@echo -e " -> /login"
	@make --no-print-directory -C login/ $(MKDEF) $@
	@echo -e " -> /char"
	@make --no-print-directory -C char/ $(MKDEF) $@
	@echo -e " -> /map"
	@make --no-print-directory -C map/ $(MKDEF) $@

conf:
	@echo
	@echo "Created Conf and Save directories"
	@echo
	cp -r conf-tmpl conf
	rm -rf conf/.svn conf/*/.svn
	cp -r save-tmpl save
	rm -rf save/.svn
	@echo

clean:
	cd common && make $(MKDEF) $@ && cd ..
	cd login && make $(MKDEF) $@ && cd ..
	cd char && make $(MKDEF) $@ && cd ..
	cd map && make $(MKDEF) $@ && cd ..
	rm -f *.exe

common/GNUmakefile: common/Makefile
	@sed -e 's/$$>/$$^/' common/Makefile > common/GNUmakefile
login/GNUmakefile: login/Makefile
	@sed -e 's/$$>/$$^/' login/Makefile > login/GNUmakefile
char/GNUmakefile: char/Makefile
	@sed -e 's/$$>/$$^/' char/Makefile > char/GNUmakefile
map/GNUmakefile: map/Makefile
	@sed -e 's/$$>/$$^/' map/Makefile > map/GNUmakefile
