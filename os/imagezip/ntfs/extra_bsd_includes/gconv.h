/*All code here is taken from gconv.h in the glibc.  I have removed what I do not need.*/

#ifndef _GCONV_H
#define _GCONV_H

#define __need_mbstate_t
#include <wchar.h>

/* Additional data for steps in use of conversion descriptor.  This is
   allocated by the `init' function.  */
struct __gconv_step_data
{
  unsigned char *__outbuf;    /* Output buffer for this step.  */
  unsigned char *__outbufend; /* Address of first byte after the output
				 buffer.  */

  /* Is this the last module in the chain.  */
  int __flags;

  /* Counter for number of invocations of the module function for this
     descriptor.  */
  int __invocation_counter;

  /* Flag whether this is an internal use of the module (in the mb*towc*
     and wc*tomb* functions) or regular with iconv(3).  */
  int __internal_use;

  __mbstate_t *__statep;
  __mbstate_t __state;	/* This element must not be used directly by
			   any module; always use STATEP!  */

  /* Transliteration information.  */
  struct __gconv_trans_data *__trans;
};

/* Flags the `__gconv_open' function can set.  */
enum
{
  __GCONV_IS_LAST = 0x0001,
  __GCONV_IGNORE_ERRORS = 0x0002
};

#endif //_GCONV_H
