#ifndef __NEIGHBORHOOD_H
#define __NEIGHBORHOOD_H

#include "port.h"
#include "common.h"
#include "physical.h"
#include "vclass.h"
#include "virtual.h"
#include "pclass.h"

/*
 * This overly-verbose function returns true if it's okay to map vn to pn,
 * false otherwise
 */
inline bool pnode_is_match(tb_vnode *vn, tb_pnode *pn);

/*
 * Finds a pnode which:
 * 1) One of the vnode's neighbors is mapped to
 * 2) Satisifies the usual pnode mapping constraints
 * 3) The vnode is not already mapped to
 */
tb_pnode *find_pnode_connected(vvertex vv, tb_vnode *vn);

tb_pnode *find_pnode(tb_vnode *vn);

#endif
