/* Just a little test to see how libsensors operates. */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sensors.h"

#define CONF_FILE "/etc/sensors.conf"

int main (int argc, char **argv) {

  int i1 = 0, i2 = 0, i3 = 0, iter, err;
  const sensors_chip_name *sens;
  const sensors_feature_data *sfd;
  char *label;
  double res;
  FILE *foo;

  if (!(foo = fopen(CONF_FILE, "r"))) {
    printf ("Zoinks!  Couldn't open conf file.  Bailing out.\n");
    exit(1);
  }

  if (sensors_init(foo)) {
    printf ("Were in trouble!  sensors_init returned nonzero\n");
    exit(1);
  }

  while (sens = sensors_get_detected_chips(&i1)) {
    printf("Chip: %s-%#x-%#x\n", sens->prefix, sens->bus, sens->addr);
    printf("Features:\n");
    i2 = i3 = 0;
    while (sfd = sensors_get_all_features(*sens, &i2, &i3)) {
      printf("*************************\n");
      printf("\tName: %s\n", sfd->name);
      printf("\tNumber: %d\n", sfd->number);
      printf("\tMapping: %d\n", sfd->mapping);
      printf("\tUnused: %d\n", sfd->unused);
      printf("\tMode: %d\n", sfd->mode);
      if ((sfd->mode & SENSORS_MODE_R) && 
          !(err = sensors_get_feature(*sens, sfd->number, &res))) {
        printf("\tValue: %f\n", res);
      }
      else if (err) {
        printf("Error reading sensor: %d\n", err);
      }
    }
    printf("*************************\n\n");
  }
  return 0;
}
    
