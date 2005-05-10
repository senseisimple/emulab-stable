/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005 University of Utah and the Flux Group.
 * All rights reserved.
 */

#include "config.h"

#include <stdlib.h>

#include "robot_list.h"

struct robot_list *robot_list_create() {
  struct robot_list *tmp = (struct robot_list *)malloc(sizeof(struct robot_list));
  if (tmp != NULL) {
	tmp->type = -1;
    tmp->item_count = 0;
    tmp->head = NULL;
    tmp->tail = NULL;
  }
  return tmp;
}

void robot_list_destroy(struct robot_list *l) {
  if (l->head != NULL) {
    struct robot_list_item *n = l->head;
    struct robot_list_item *nn = NULL;
    while (n != NULL) {
      nn = n->next;
      free(n);
      n = nn;
    }
  }
  free(l);
}

int robot_list_append(struct robot_list *l,int id,void *data) {
  if (l == NULL || data == NULL) {
    return 0;
  }
  else {
    struct robot_list_item *n = (struct robot_list_item *)malloc(sizeof(struct robot_list_item));
	n->id = id;
    n->data = data;
    n->next = NULL;
    if (n != NULL) {
      if (l->head == NULL) {
		l->head = n;
		l->tail = n;
      }
      else {
		l->tail->next = n;
		l->tail = n;
      }
      ++(l->item_count);
      return 1;
    }
    else {
      return 0;
    }
  }
}

int robot_list_insert(struct robot_list *l,int id,void *data) {
  if (l == NULL || data == NULL) {
    return 0;
  }
  else {
    struct robot_list_item *n = (struct robot_list_item *)malloc(sizeof(struct robot_list_item));
	n->id = id;
    n->data = data;
    n->next = NULL;
    if (n != NULL) {
      if (l->head == NULL) {
	l->head = n;
	l->tail = n;
      }
      else {
	n->next = l->head;
	l->head = n;
      }
      ++(l->item_count);
      return 1;
    }
    else {
      return 0;
    }
  }
}

void *robot_list_search(struct robot_list *l,int id) {
  if (l == NULL) {
	return NULL;
  }
  else {
	struct robot_list_item *h = NULL;
    struct robot_list_item *i = l->head;

    void *retval = NULL;

    while (i != NULL) {
      if (i->id == id) {
		retval = i->data;
		break;
      }
      h = i;
      i = i->next;
    }
    
    return retval;
  }
}
	

int robot_list_remove_by_data(struct robot_list *l,void *data) {
  if (l == NULL || data == NULL) {
    return -1;
  }
  else {
    struct robot_list_item *h = NULL;
    struct robot_list_item *i = l->head;
	
    int retval = -1;
	
    while (i != NULL) {
      if (i->data == data) {
		// if we're at head and there's only a single elm
		if (i == l->head && l->head == l->tail) {
		  l->head = l->tail = NULL;
		}
		else {
		  if (i == l->head) {
			l->head = i->next;
		  }
		  else if (i == l->tail) {
			h->next = NULL;
			l->tail = h;
		  }
		  else {
			h->next = i->next;
		  }
		}
		
		retval = i->id;
		free(i);
		--(l->item_count);
		
		break;
      }
      
      h = i;
      i = i->next;
    }
    
    return retval;
  }
}

void *robot_list_remove_by_id(struct robot_list *l,int id) {
  if (l == NULL) {
    return NULL;
  }
  else {
    struct robot_list_item *h = NULL;
    struct robot_list_item *i = l->head;

    void *retval = NULL;

    while (i != NULL) {
      if (i->id == id) {
		// if we're at head and there's only a single elm
		if (i == l->head && l->head == l->tail) {
		  l->head = l->tail = NULL;
		}
		else {
		  if (i == l->head) {
			l->head = i->next;
		  }
		  else if (i == l->tail) {
			h->next = NULL;
			l->tail = h;
		  }
		  else {
			h->next = i->next;
		  }
		}
		
		retval = i->data;
		free(i);
		--(l->item_count);
		
		break;
      }
      
      h = i;
      i = i->next;
    }
    
    return retval;
  }
}

struct robot_list_enum *robot_list_enum(struct robot_list *l) {
  if (l == NULL || l->item_count == 0) {
    return NULL;
  }
  else {
    struct robot_list_enum *e = (struct robot_list_enum *)malloc(sizeof(struct robot_list_enum));
    void **tmp = NULL;
    if (e == NULL) {
      return NULL;
    }
    else {
      struct robot_list_item *a;
      int i;
      
      tmp = (void *)malloc(sizeof(void *)*(l->item_count));
      if (tmp == NULL) {
		free(e);
		return NULL;
      }
	  
      e->data = tmp;
      e->current_index = 0;
      e->size = l->item_count;
	  
      /* scroll through list forwards and copy data into e->data */
      a = l->head;
      i = 0;
      
      while (a != NULL) {
		e->data[i] = a->data;
		a = a->next;
		++i;
      }
      return e;
    }
  }
}

void *robot_list_enum_next_element(struct robot_list_enum *e) {
  if (e != NULL) {
    if (e->current_index == e->size) {
      return NULL;
    }
    else {
      void *retval = e->data[e->current_index];
      ++(e->current_index);
      return retval;
    }
  }
  else {
    return NULL;
  }
}

void robot_list_enum_destroy(struct robot_list_enum *e) {
  if (e != NULL) {
    free(e->data);
    free(e);
  }
}
