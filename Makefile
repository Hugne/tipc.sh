SHOBJ_CC = gcc
SHOBJ_CFLAGS = -fPIC
SHOBJ_LD = ld
SHOBJ_LDFLAGS = -shared
SHOBJ_XLDFLAGS = 
SHOBJ_LIBS = 
SHOBJ_STATUS = supported

INC = -I./bash -I./bash/include -I./bash/builtins
LIB = -lpthread
.c.o:
	$(SHOBJ_CC) $(SHOBJ_CFLAGS) $(CCFLAGS) $(INC) -c -o $@ $<

all:	tipc_subscribe.so

tipc_subscribe.so:	tipc_subscribe.o
	$(SHOBJ_LD) $(LIB) $(SHOBJ_LDFLAGS) $(SHOBJ_XLDFLAGS) -o $@ tipc_subscribe.o $(SHOBJ_LIBS)

tipc_subscribe.o: tipc_subscribe.c

clean:	
	rm -f *.so *.o
