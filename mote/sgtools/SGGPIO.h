/*
 * $Id: SGGPIO.h,v 1.1 2004-12-16 17:53:32 ricci Exp $
 *
 ****************************************************************************
 *
 * uisp - The Micro In-System Programmer for Atmel AVR microcontrollers.
 * Copyright (C) 2003 University of Utah and The Flux Group
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
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
