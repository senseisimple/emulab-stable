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
 * Desc: Table window
 * Author: Andrew Howard
 * Date: 11 Apr 2002
 * CVS: $Id: tablewnd.c,v 1.1 2004-12-12 23:36:34 johnsond Exp $
 ***************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include "mezzcal.h"


// The one and only tablewnd
static tablewnd_t *tablewnd;

// Initialise the table
tablewnd_t *tablewnd_init(rtk_app_t *app, mezz_mmap_t *mmap)
{
  tablewnd = malloc(sizeof(tablewnd_t));

  // Store pointer to the ipc mmap
  tablewnd->mmap = mmap;

  // Create the table
  tablewnd->table = rtk_table_create(app, 200, 400);

  return tablewnd;
}

