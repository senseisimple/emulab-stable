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
 * Desc: Some useful color conversions.
 * Author: Andrew Howard
 * Date: 28 Mar 2002
 * CVS: $Id: color.h,v 1.1 2004-12-12 23:36:34 johnsond Exp $
 **************************************************************************/

#ifndef SMCOLOR_H
#define SMCOLOR_H

#define RGB16(r, g, b) (((b) >> 3) | (((g) & 0xFC) << 5) | (((r) & 0xF8) << 16))

#define R_RGB16(x) (((x) >> 8) & 0xF8)
#define G_RGB16(x) (((x) >> 3) & 0xFC)
#define B_RGB16(x) (((x) << 3) & 0xF8)

#define Y_RGB16(x) ((0.257 * R_RGB16(x)) + (0.504 * G_RGB16(x)) + (0.098 * B_RGB16(x)) + 16)
#define V_RGB16(x) ((0.439 * R_RGB16(x)) - (0.368 * G_RGB16(x)) - (0.071 * B_RGB16(x)) + 128)
#define U_RGB16(x) (-(0.148 * R_RGB16(x)) - (0.291 * G_RGB16(x)) + (0.439 * B_RGB16(x)) + 128)

#define RGB32(r, g, b) ((r) | ((g) << 8) | ((b) << 16))

#define R_RGB32(x) ((x) & 0xFF)
#define G_RGB32(x) ((x >> 8) & 0xFF)
#define B_RGB32(x) ((x >> 16) & 0xFF)

#define Y_RGB32(x) ((0.257 * R_RGB32(x)) + (0.504 * G_RGB32(x)) + (0.098 * B_RGB32(x)) + 16)
#define V_RGB32(x) ((0.439 * R_RGB32(x)) - (0.368 * G_RGB32(x)) - (0.071 * B_RGB32(x)) + 128)
#define U_RGB32(x) (-(0.148 * R_RGB32(x)) - (0.291 * G_RGB32(x)) + (0.439 * B_RGB32(x)) + 128)

#define Y_RGB(r, g, b) ((0.257 * r) + (0.504 * g) + (0.098 * b) + 16)
#define V_RGB(r, g, b) ((0.439 * r) - (0.368 * g) - (0.071 * b) + 128)
#define U_RGB(r, g, b) (-(0.148 * r) - (0.291 * g) + (0.439 * b) + 128)

#endif
