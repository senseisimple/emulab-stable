SYSTEM :=	$(shell uname -s)

# I like using -pedantic, but it gives us some trouble with function pointers
CFLAGS=-Wall -Werror

ifeq ($(SYSTEM),Linux)
    LD_FLAGS=
else
    LD_FLAGS=
endif

all: schedmon

clean:
	-rm schedmon
