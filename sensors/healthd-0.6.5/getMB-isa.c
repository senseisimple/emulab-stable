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
 *	$Id: getMB-isa.c,v 1.1 2001-12-05 18:45:08 kwebb Exp $
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <fcntl.h>
#include <unistd.h>
#include <machine/cpufunc.h>
#include "methods.h"

#ifdef __OpenBSD__
#include <i386/pio.h>
#include <sys/types.h>
#include <machine/sysarch.h>
#else /* !__OpenBSD__ */
static int iofl;
#endif /* !__OpenBSD__ */

#define WBIO1 0x295
#define WBIO2 0x296

static int
OpenIO(void) {
#ifdef __OpenBSD__
  /* get the necessary I/O permissions */
  if (i386_iopl(3)) {
    return -1;
  }

  /* drop root priveleges to minimize security risk of running suid root */
  seteuid(getuid());
  setegid(getgid());
#else /* !__OpenBSD__ */
  if ((iofl = open("/dev/io",000)) < 0) {
    return -1;
  }
#endif /* !__OpenBSD__ */
  return 0;
}

static int
CloseIO(void) {
#ifdef __OpenBSD__
  return 0;
#else /* !__OpenBSD__ */
  return close(iofl);
#endif /* !__OpenBSD__ */
}

static int
WriteByte(int addr, int value) {
  outb(WBIO1, addr);
  outb(WBIO2, value);
  return 0;
}

static int 
ReadByte(int addr) {
  outb(WBIO1, addr);
  return (unsigned int)inb(WBIO2);
}

struct lm_methods method_isa = {
  OpenIO, 
  CloseIO, 
  ReadByte, 
  WriteByte
};
