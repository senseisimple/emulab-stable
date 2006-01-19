SYSTEM :=	$(shell uname -s)

# I like using -pedantic, but it gives us some trouble with function pointers
CFLAGS=-Wall -Werror

ifeq ($(SYSTEM),Linux)
    LD_FLAGS=-ldl
else
    LD_FLAGS=
endif

all: libnetmon.so

%.so: %.c %.h
	$(CC) $(CFLAGS) $(LD_FLAGS) -shared -o $@ $<

clean:
	-rm *.so
