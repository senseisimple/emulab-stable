/* 
 *  Mezzanine - an overhead visual object tracker.
 *  Copyright (C) Andrew Howard 2002
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
/***************************************************************************
 * Desc: Some useful error macros
 * Author: Andrew Howard
 * Date: 11 Apr 2002
 * CVS: $Id: err.h,v 1.1 2004-12-12 23:36:34 johnsond Exp $
 **************************************************************************/

#ifndef ERR_H
#define ERR_H

#include <errno.h>
#include <stdio.h>

// Message macros
#define PRINT_MSG(m)          printf("\r"m"\n")
#define PRINT_MSG1(m, a)      printf("\r"m"\n", a)
#define PRINT_MSG2(m, a, b)   printf("\r"m"\n", a, b)

// Error macros
#define PRINT_ERR(m)         printf("\rmezzanine error : %s %s\n  "m"\n", \
                                    __FILE__, __FUNCTION__)
#define PRINT_ERR1(m, a)     printf("\rmezzanine error : %s %s\n  "m"\n", \
                                    __FILE__, __FUNCTION__, a)
#define PRINT_ERR2(m, a, b)  printf("\rmezzanine error : %s %s\n  "m"\n", \
                                    __FILE__, __FUNCTION__, a, b)
#define PRINT_ERRNO(m)       printf("\rmezzanine error : %s %s\n  "m" : %s\n", \
                                   __FILE__, __FUNCTION__, strerror(errno))
#define PRINT_ERRNO1(m, a)      printf("\rmezzanine error : %s %s\n  "m" : %s\n", \
                                   __FILE__, __FUNCTION__, a, strerror(errno))


#endif
