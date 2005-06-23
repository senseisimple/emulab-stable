#include <stdio.h>
#include <stdlib.h>

#include "serialsource.h"

static char *msgs[] = {
  "unknown_packet_type",
  "ack_timeout"	,
  "sync"	,
  "too_long"	,
  "too_short"	,
  "bad_sync"	,
  "bad_crc"	,
  "closed"	,
  "no_memory"	,
  "unix_error"
};

void stderr_msg(serial_source_msg problem)
{
  fprintf(stderr, "Note: %s\n", msgs[problem]);
}

int main(int argc, char **argv)
{
  serial_source src;

  if (argc != 3)
    {
      fprintf(stderr, "Usage: %s <device> <rate> - dump packets from a serial port\n", argv[0]);
      exit(2);
    }
  src = open_serial_source(argv[1], atoi(argv[2]), 0, stderr_msg);
  if (!src)
    {
      fprintf(stderr, "Couldn't open serial port at %s:%s\n",
	      argv[1], argv[2]);
      exit(1);
    }
  for (;;)
    {
      int len, i;
      const unsigned char *packet = read_serial_packet(src, &len);

      if (!packet)
	exit(0);

      int rssi_val;
      int am_type;
      int orig_sender;
      float real_rssi;
      float vdc = 3.0f;
      int crc;

      rssi_val = ((packet[7] & 0xff) << 8) | (packet[6] & 0xff);
      am_type = packet[8];
      orig_sender = ((packet[10] & 0xff) << 8) | (packet[9] & 0xff);
      crc = ((packet[12] & 0xff) << 8) | (packet[11] & 0xff);

      /* for mica 2, ALSO depends on the value of vdc; which for wall plugin
       * is 3 VDC; for 1.5V batts it's something else
       */ 
      real_rssi = -50.0f * (vdc * (rssi_val/1024.0f)) - 45.5f;
      
      printf("original sender: %d; crc: %d; AM type: %d; RSSI: %f dBm\n",
	     orig_sender,
	     crc,
	     am_type,
	     real_rssi);
      fflush(stdout);
      //putchar('\n');
      free((void *)packet);
    }
}
