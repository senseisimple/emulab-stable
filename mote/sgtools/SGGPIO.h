/*
 * $Id: SGGPIO.h,v 1.2 2006-12-01 22:59:37 mike Exp $
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
  SGGPIO.h
  
  Direct Stargate/PXA GPIO Access
  
*/

#ifndef __SGGPIO_H
#define __SGGPIO_H

#include <stdlib.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#define GPIO_bit(x)	(1 << ((x) & 0x1f))
#define GPIO_nr(x)      ((x) >> 5)

enum PXA_REG {
  PXA_GPLR = 0,
  PXA_GPDR = 1,
  PXA_GPSR = 2,
  PXA_GPCR = 3
};
#define NUM_REGS 4
#define NUM_REGNUMS 3

extern char *pxa_reg_names[];

#define PXA_REG_NAME(reg) pxa_reg_names[(reg)]

#define PROC_REG_PREFIX "/proc/cpu/registers"

class SGGPIO_PORT {

public:

  int setDir(int pin, unsigned int indir);
  int setPin(int pin, unsigned int inval);
  int readPin(int pin);
  unsigned int readGPIO(PXA_REG reg,  int regnum);
  int writeGPIO(PXA_REG reg, int regnum, unsigned int value);
  SGGPIO_PORT();
  ~SGGPIO_PORT();

private:

  int getGPIO(PXA_REG reg, int regnum);
  int GPIO_fds[NUM_REGS][NUM_REGNUMS];
};

#endif /* __SGGPIO_H */
