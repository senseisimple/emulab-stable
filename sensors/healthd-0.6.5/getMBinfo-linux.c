/*
 *
 * Subroutines for healthd such that it can collect data in linux via
 * lm_sensor's libsensors.
 *
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sensors.h"

/* Generalizations made from lm-sensors */
#define SENS_IN0 1
#define SENS_IN1 2
#define SENS_IN2 3
#define SENS_IN3 4
#define SENS_IN4 5
#define SENS_IN5 6
#define SENS_IN6 7
#define SENS_FAN1 31
#define SENS_FAN2 32
#define SENS_FAN3 33
#define SENS_TEMP1 51
#define SENS_TEMP2 54
#define SENS_TEMP3 57
#define SENS_VID 61
#define SENS_ALARMS 81

#define NUM_CRIT 4
#define SENS_CONF "/etc/sensors.conf"

extern int debug;

int crit_feat[] = { SENS_IN0, SENS_TEMP1, SENS_VID, SENS_FAN1 };
int found = 0;
const sensors_chip_name *sens;

/* Don't care about type param, but have it for compat */
int InitMBInfo(char type) {
  FILE *sensconf;
  const sensors_feature_data *sfd;
  int nr = 0, nr2, nr3, ncf, i;

  if (!(sensconf = fopen(SENS_CONF, "r"))) {
    fprintf(stderr, "Oops! Can't read from sensors config file.\n");
    return -1;
  }
  
  if (sensors_init(sensconf)) {
    fprintf(stderr, "Uh oh - sensors_init returned nonzero.\n");
    return -1;
  }

  while (!found && (sens = sensors_get_detected_chips(&nr))) {
    ncf = nr2 = nr3 = 0;
    while ((sfd = sensors_get_all_features(*sens, &nr2, &nr3))) {
      for (i = 0; i < NUM_CRIT; ++i) {
        if (sfd->number == crit_feat[i]) {
          ++ncf;
          if (debug) {
            printf ("Feature is: %s\n", sfd->name);
          }
        }
      }
    }
    if (ncf != NUM_CRIT) {
      fprintf(stderr, "Chip: does not have needed features. skipping.\n");
      fprintf(stderr, "ncf: %d\n", ncf);
    }
    else {
      found = 1;
    }
  }
  
  if (found)
    return 0;

  return -1;
}

int FiniMBinfo(void) {
  sensors_cleanup();
  return 0;
}

/* may want to do chip "set" commands here eventually. */
int RstChip(void) {
  return 0;
}

/* Not really relevant in Linux version. */
unsigned int GetVendorID(void) {
  return 0xFFFFFFFF;
}

unsigned int GetChipID_winbond(void) {
  return 0xFFFFFFFF;
}

int getTemp(double *t1, double *t2, double *t3) {

  if (!found) {
    *t1 = *t2 = *t3 = 0.0;
    return -1;
  }

  if (sensors_get_feature(*sens, SENS_TEMP1, t1) < 0) {
    *t1 = 0.0;
  }

  if (sensors_get_feature(*sens, SENS_TEMP2, t2) < 0) {
    *t2 = 0.0;
  }

  if (sensors_get_feature(*sens, SENS_TEMP3, t3) < 0) {
    *t3 = 0.0;
  }

  return 0;
}

int getVolt(double *vc0, double *vc1, double *v33,\
	    double *v50p, double *v50n,\
	    double *v12p, double *v12n) {

  if (!found) {
    *vc0 = *vc1 = *v33 = *v50p = *v50n = *v12p = *v12n = 0.0;
    return -1;
  }

  if (sensors_get_feature(*sens, SENS_IN1, vc0)) {
    *vc0 = 0.0;
  }

  /* might not be vcore2 ? */
  if (sensors_get_feature(*sens, SENS_IN5, vc1)) {
    *vc1 = 0.0;
  }

  if (sensors_get_feature(*sens, SENS_IN2, v33)) {
    *v33 = 0.0;
  }

  if (sensors_get_feature(*sens, SENS_IN3, v50p)) {
    *v50p = 0.0;
  }

  if (sensors_get_feature(*sens, SENS_IN6, v50n)) {
    *v50n = 0.0;
  }

  if (sensors_get_feature(*sens, SENS_IN4, v12p)) {
    *v12p = 0.0;
  }

  if (sensors_get_feature(*sens, SENS_IN5, v12n)) {
    *v12n = 0.0;
  }

  return 0;
}


int getFanSp(int *r1, int *r2, int *r3) {

  double dr1 = 0, dr2 = 0, dr3 = 0;

  if (!found) {
    *r1 = *r2 = *r3 = 0;
    return -1;
  }

  if (sensors_get_feature(*sens, SENS_FAN1, &dr1)) {
    *r1 = 0;
  }

  if (sensors_get_feature(*sens, SENS_FAN2, &dr2)) {
    *r2 = 0;
  }

  if (sensors_get_feature(*sens, SENS_FAN3, &dr3)) {
    *r3 = 0;
  }
  
  *r1 = (int)dr1;
  *r2 = (int)dr2;
  *r3 = (int)dr3;

  return 0;
}

