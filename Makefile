# vim: set noexpandtab:
# 
#   Aniruddha. A (aniruddha.a@gmail.com)
#  

CC = gcc
CFLAGS = -g -ggdb -O2 -Wall
OBJECTS = logg.o timer.o list.o ancp.o ancp_fsm.o utils.o tcp_readwr.o getmac.o 
SRVOBJS = $(OBJECTS) ancp_server.o scli_hdlr.o 
CLIOBJS = $(OBJECTS) ancp_client.o
DIRS = cli

all :  ancp_fsmtbl.h server client srvrcli

ancp_fsmtbl.h : ancp.fsm
	@./fsmgen.pl ancp.fsm

# for now making server and client dependent on fsm table
# ideally, ancp_fsm.c should be dependent
server : $(SRVOBJS) ancp_fsmtbl.h
	$(CC) $(CFLAGS) $(SRVOBJS) -o $@

client : $(CLIOBJS) ancp_fsmtbl.h
	$(CC) $(CFLAGS) $(CLIOBJS) -o $@

srvrcli : force_look 
	@cd cli; $(MAKE)

%.o : %.c %.h
	$(CC) $(CFLAGS) -c $<

force_look:
	@true
clean:
	@rm -f $(OBJECTS) $(SRVOBJS) $(CLIOBJS) ancp_fsmtbl.h server client srvrcli
	@for i in $(DIRS); do (cd $$i; $(MAKE) clean); done

pkg:
	@tar zcvf ancp_lin.tgz Makefile \
	   	  README \
		  *.[ch] \
		  cli/*.[ch] \
		  cli/Makefile  
