# Makefile for System & Process Monitor ATOP (Linux version)
#
# Gerlof Langeveld - gerlof.langeveld@atoptool.nl
#
DESTDIR  =

BINPATH  = /usr/bin
SBINPATH = /usr/sbin
SCRPATH  = /usr/share/atop
LOGPATH  = /var/log/atop
MAN1PATH = /usr/share/man/man1
MAN5PATH = /usr/share/man/man5
MAN8PATH = /usr/share/man/man8
INIPATH  = /etc/init.d
DEFPATH  = /etc/default
SYSDPATH = /lib/systemd/system
CRNPATH  = /etc/cron.d
ROTPATH  = /etc/logrotate.d
PMPATH1  = /usr/lib/pm-utils/sleep.d
PMPATH2  = /usr/lib64/pm-utils/sleep.d
PMPATHD  = /usr/lib/systemd/system-sleep

CFLAGS  += -O2 -I. -Wall -Wno-stringop-truncation # -DNOPERFEVENT   # -DHTTPSTATS
OBJMOD0  = version.o
OBJMOD1  = various.o  deviate.o   procdbase.o
OBJMOD2  = acctproc.o photoproc.o photosyst.o  rawlog.o ifprop.o parseable.o
OBJMOD3  = showgeneric.o          showlinux.o  showsys.o showprocs.o
OBJMOD4  = atopsar.o  netatopif.o gpucom.o
ALLMODS  = $(OBJMOD0) $(OBJMOD1) $(OBJMOD2) $(OBJMOD3) $(OBJMOD4)

all: 		pmon

pmon:		pmon.o atop.o $(ALLMODS) Makefile
		$(CC) pmon.o atop.o $(ALLMODS) -o pmon -lncursesw -lz -lm -lrt $(LDFLAGS)

clean:
		rm -f *.o pmon


##########################################################################

pmon.o:		atop.h  photoproc.h photosyst.h  acctproc.h showgeneric.h
