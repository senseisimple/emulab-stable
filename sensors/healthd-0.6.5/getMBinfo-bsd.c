/*-
 * Copyright (c) 1999-2000 James E. Housley <jim@thehousleys.net>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	$Id: getMBinfo-bsd.c,v 1.1 2001-12-05 18:45:08 kwebb Exp $
 */

/************************************************
	Subroutines to get Mother Board Information
	assuming WinBond chips

	Information related to WinBond W83781D Chip
		and National Semiconductor LM78/LM79/LM75 Chips
	----------------------------------------------
	http://www.euronet.nl/users/darkside/mbmonitor
	----------------------------------------------
	by Alex van Kaam
 ************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <sys/types.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>
#include <netdb.h>
#include "methods.h"
#include "parameters.h"
#include "healthd.h"
extern struct lm_methods method_isa;

#ifdef HAVE_SMBUS
extern struct lm_methods method_smb;
#endif
static struct lm_methods *this_method;

#define OpenIO()             (this_method->Open());
#define CloseIO()            (this_method->Close());
#define ReadByte(addr)       (this_method->Read((addr)))
#define WriteByte(addr, val) (this_method->Write((addr), (val)))

/*----------------------
  Restarting Chip
  ----------------------*/
int
InitMBInfo(char type) {
  if(type == 'I') {
    this_method = &method_isa;
  }
#ifdef HAVE_SMBUS
  else if(type == 'S'){
    this_method = &method_smb;
  }
#endif
  else {
    return -1;
  }
  return OpenIO();
}

int
FiniMBInfo(void)
{
  return CloseIO();
}

int 
RstChip(void) {
  return WriteByte(0x40,0x01);
}

unsigned int 
GetVendorID(void) {
  unsigned int result;

  /*
   * First check for WinBond chips
   */
  /* changing to bank 0 */
  WriteByte(0x4E, 0x00);
  /* Select HBACS to get MSW of ID */
  WriteByte(0x4E, 0x80);
  result = ReadByte(0x4F) << 8;
  /* Clear HBACS to get LSW of ID */
  WriteByte(0x4E, 0x00);
  result += ReadByte(0x4F) & 0x0FF;
  if (result != 0)
    return(result);
  /*
   * Try LM78/LM79
   */
  result = ReadByte(0x49);
  if (result != 0)
    return(0xFFFF0000 | result);
  return(result);
}

unsigned int 
GetChipID_winbond(void) {
  unsigned int result;

  result = ReadByte(0x58);
  return(result);
}

/*-------------------------------------------
  Getting Temperatures (assuming WinBond)
  -------------------------------------------*/

int getTemp(double *t1, double *t2, double *t3) {
  int n;

  /* Force bank 0 */
  WriteByte(0x4E, 0);
  n = ReadByte(0x27);
  *t1 = n;

  /* changing to bank 1 */
  WriteByte(0x4E, 0x01);
  n = ReadByte(0x50);
  if (ReadByte(0x51) == 128)
    *t2 = n + 0.5;
  else
    *t2 = n;
  /* returning to bank 0 */
  WriteByte(0x4E, 0);
  /* 208 if no connection to thermal chip !*/
  if (n > 200)
    *t2 = 0.0;

 /* changing to bank 2 */
  WriteByte(0x4E, 0x02);
  n = ReadByte(0x50);
  if (ReadByte(0x51) == 128)
    *t3 = n + 0.5;
  else
    *t3 = n;
  /* returning to bank 0 */
  WriteByte(0x4E,0);
  /* 208 if no connection to thermal chip !*/
  if (n > 200)
    *t3=0.0;
  return 0;
}

/*-------------------------------------------
  Getting Voltages (assuming WinBond)
  -------------------------------------------*/

int getVolt(double *vc0, double *vc1, double *v33,\
	    double *v50p, double *v50n,\
	    double *v12p, double *v12n) {
  unsigned char n;

  extern enum eChip MonitorType;
  extern int UseVbat;

  /*
   * Each step in the A/D is 16mV
   */

  /*
   * Vcore0
   */
  *vc0 = (double)ReadByte(0x20) * 0.016;

  /*
   * Vcore1
   */
  if (UseVbat) {
    /*
     * Some systems seem to like using Vbat for Vcore1
     */
    /* changing to bank 5 */
    WriteByte(0x4E, 0x05);
    *vc1 = (double)ReadByte(0x51) * 0.016;
    /* changing to bank 0 */
    WriteByte(0x4E, 0x00);
  } else {
    *vc1 = (double)ReadByte(0x21) * 0.016;
  }

  /*
   * 3.3Volt
   */
  *v33 = (double)ReadByte(0x22) * 0.016;

  /*
   * +5Volt
   */
  n=ReadByte(0x23);
  *v50p = (double)n * 0.016 * 1.68;

  /*
   * +12Volt
   */
  n=ReadByte(0x24);
  if ((MonitorType == LM78) || (MonitorType == LM79) || (MonitorType == AS99127F)) {
    /*
     * LM78 & LM79
     */
    *v12p = (double)n * 0.016 * 4.00;
  } else {
    /*
     * Winbond W83781D or W83782D
     */
    *v12p = (double)n * 0.016 * 3.80;
  }

  /*
   * -12Volt
   */
  n=ReadByte(0x25) ;
  if ((MonitorType == LM78) || (MonitorType == LM79) || (MonitorType == AS99127F)) {
    /*
     * LM78 & LM79
     */
    *v12n = - n * 0.016 * 4.00;
  }
  else if ((MonitorType == W83781D)) {
    /*
     * Winbond W83781D ONLY
     */
    *v12n = - n * 0.016 * 3.47;
  } else {
    /*
     * Winbond W83782D
     */
    *v12n = n * 0.016 * 5.143 - 14.914;
  }

  /*
   * -5Volt
   */
  n=ReadByte(0x26);
  if ((MonitorType == LM78) || (MonitorType == LM79)) {
    /*
     * LM78 & LM79
     */
    *v50n = - n * 0.016 * 1.6666667;
  }
  else if ((MonitorType == W83781D) || (MonitorType == AS99127F)) {
    /*
     * Winbond W83781D ONLY
     */
    *v50n = - n * 0.016 * 1.50;
  } else {
    /*
     * Winbond W83782D
     */
    *v50n = n * 0.016 * 3.143 - 7.714;
  }

  return 0;
}

/*----------------------
  Getting Fan Speed
  ----------------------*/

int getFanSp(int *r1, int *r2, int *r3) {
  /*
    FanType = 1 for a normal fan
    FanType = 2 for a fan which gives 2 pulses per rotation
  */
  int FanType1 = 1, FanType2 = 1, FanType3 = 1;
  int div1, div2, div3 = 2;
  unsigned char n;
  n=ReadByte(0x47);
  div1 = 1 << ((n & 0x30) >> 4);
  div2 = 1 << ((n & 0xC0) >> 6);
  n=ReadByte(0x28);
  if ((n == 255) || (n == 0) || (div1 == 0))
    *r1 = 0;
  else
    *r1 = 1350000 / (n * div1 * FanType1);
  n=ReadByte(0x29);
  if ((n == 255) || (n == 0) || (div2 == 0))
    *r2 = 0;
  else
    *r2 = 1350000 / (n * div2 * FanType2);
  n=ReadByte(0x2A);
  if ((n == 255) || (n == 0))
    *r3 = 0;
  else
    *r3 = 1350000 / (n * div3 * FanType3);
	
  return 0;
}
