/*
 * $Id: SGGPIO.C,v 1.2 2006-12-01 22:59:37 mike Exp $
 *
 ****************************************************************************
 *
 * uisp - The Micro In-System Programmer for Atmel AVR microcontrollers.
 *
 * EMULAB-COPYRIGHT
 * Copyright (c) 2003 University of Utah and the Flux Group.
 * All rights reserved.
 *
 ****************************************************************************
 */

/*
  SGGPIO.C
  
  Direct Stargate/PXA GPIO Access
  
*/


#include "SGGPIO.h"

char *pxa_reg_names[] = {"GPLR", "GPDR", "GPSR", "GPCR"};

int 
SGGPIO_PORT::setDir(int pin, unsigned int indir) {
  
  unsigned int val = readGPIO(PXA_GPDR, GPIO_nr(pin));
  
  if (indir) {
    val = val | GPIO_bit(pin);
  } else {
    val = val & ~(GPIO_bit(pin));
  }
  
  writeGPIO(PXA_GPDR, GPIO_nr(pin), val);
  
  return 0;
}

int 
SGGPIO_PORT::setPin(int pin, unsigned int inval) {
  if (inval) {
    writeGPIO(PXA_GPSR, GPIO_nr(pin), GPIO_bit(pin));
  } else {
    writeGPIO(PXA_GPCR, GPIO_nr(pin), GPIO_bit(pin));
  }
  return 0;
}

int 
SGGPIO_PORT::readPin(int pin) {
  return ((readGPIO(PXA_GPLR, GPIO_nr(pin)) & GPIO_bit(pin)) >> (0x1f & pin));
}

unsigned int 
SGGPIO_PORT::readGPIO(PXA_REG reg,  int regnum) {
  int fdesc = getGPIO(reg, regnum);
  char buf[16];
  int cnt = 0;
  unsigned int retval = 0;

  
  if (! fdesc) {
    printf("Couldn't get GPIO handle\n");
    return 0;
  }

  if ((cnt = pread(fdesc, buf, sizeof(buf)-1, 0)) <= 0) {
    printf("Error reading GPIO register\n");
    return 0;
  }

  buf[cnt] = '\0';
  retval = strtoul(buf, NULL, 0);
  return retval;
}

int 
SGGPIO_PORT::writeGPIO(PXA_REG reg, int regnum, unsigned int value) {
  int fdesc = getGPIO(reg, regnum);
  char buf[16];
  int len;
  
  if (! fdesc) {
    printf("Couldn't get GPIO handle\n");
    return -1;
  }
  
  len = sprintf(buf, "0x%08X\n", value);
  
  if (pwrite(fdesc, buf, len, 0) < 0) {
    perror("Problem setting GPIO register");
    printf("Register: %s%d\n", PXA_REG_NAME(reg), regnum);
    printf("fdesc: %d\n", fdesc);
    printf("Data: %s\n", buf);
    return -1;
  }

  return 0;
}

SGGPIO_PORT::SGGPIO_PORT() {
  int i, j;
  
  for (i = 0; i < NUM_REGS; ++i) {
    for (j = 0; j < NUM_REGNUMS; ++j) {
      GPIO_fds[i][j] = 0;
    }
  }
}

SGGPIO_PORT::~SGGPIO_PORT() {
  int i, j;

  for (i = 0; i < NUM_REGS; ++i) {
    for (j = 0; j < NUM_REGNUMS; ++j) {
      if (GPIO_fds[i][j]) {
        close(GPIO_fds[i][j]);
        GPIO_fds[i][j] = 0;
      }
    }
  }
}


int 
SGGPIO_PORT::getGPIO(PXA_REG reg, int regnum) {
  char gpioPath[256];
  
  if (! GPIO_fds[reg][regnum]) {
    sprintf(gpioPath, "%s/%s%d", PROC_REG_PREFIX, PXA_REG_NAME(reg), regnum);
    if ((GPIO_fds[reg][regnum] = open(gpioPath, O_RDWR | O_FSYNC, 0)) < 0) {
      printf("Error opening %s proc file: %s\n", PXA_REG_NAME(reg), 
             strerror(errno));
      return -1;
    }
  }
  
  return GPIO_fds[reg][regnum];
}
