PROG = project05
OBJS = project05.o

CFLAGS = -g

# Pattern rules to avoid explicit rules
%.o : %.c
	gcc $(CFLAGS) -c -o $@ $<

all : $(PROG)

$(PROG) : project05.h $(OBJS)
	gcc $(CFLAGS) -o $@ $(OBJS)

clean :
	rm -rf $(PROG) $(OBJS)

