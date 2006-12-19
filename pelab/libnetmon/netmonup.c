#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

/*
 * Quick hack: just open a socket.
 * Return 0 if it works, non-zero otherwise.
 * When run under instrument.sh, will tell us if netmond is ready for traffic.
 */
int
main()
{
	int rv = socket(PF_INET, SOCK_DGRAM, 0);
	if (rv >= 0) {
		close(rv);
		exit(0);
	}
	exit(rv);
}
