# vim: set noexpandtab:
# 
#   IOS like CLI completion - non input reading
#   Using unichar uncompressed trie
#   Tue Sep 29 16:09:15 IST 2009
#   Aniruddha. A (aniruddha.a@gmail.com)
#  

CC = gcc
CFLAGS = -g -ggdb -O2
OBJECTS = srvrcli.o strie.o srv_hdlr.o

all : ../srvrcli

../srvrcli : $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

%.o : %.c
	$(CC) $(CFLAGS) -c $<
clean:
	@rm -f $(OBJECTS) 
