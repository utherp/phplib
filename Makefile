include config.make

DEPEND		=
SRCS		=	php_helpers.c php_serialize.c php_unser_stack.c

all		:	$(OBJS)

debug   :   CFLAGS+=-D_DEBUG_
debug   :   all

clean: 
	$(RM) *.o
