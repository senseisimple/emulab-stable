#ifndef __ROBOT_LIST_H__
#define __ROBOT_LIST_H__

/* this is essentially an advanced list class -- data are of type void*;
 * support for enumerations is provided for ease-of-use
 */

/* it essentially does for C what the STL does in c++ -- but I hate c++ --
 * so the upshot is that it's not type-safe -- oh well.
 */

#ifndef NULL
#define NULL (void*)0
#endif


typedef struct robot_list {
  int item_count;
  int type;
  struct robot_list_item *head;
  struct robot_list_item *tail;
  struct robot_list_item *iterator;
} robot_list_t;

typedef struct robot_list_item {
  int id;
  void *data;
  struct robot_list_item *next;
} robot_list_item_t;

typedef struct robot_list_enum {
  int size;
  int current_index;
  void **data;
} robot_list_enum_t;

/* creates a robot_list */
struct robot_list *robot_list_create();
/* frees all structs used by the list -- but leaves data intact */
void robot_list_destroy(struct robot_list *l);
/* return 0 on failure, 1 on success */
int robot_list_append(struct robot_list *l,int id,void *data);
int robot_list_insert(struct robot_list *l,int id,void *data);

/* return a pointer to the data on success; NULL otherwise */
void *robot_list_search(struct robot_list *l,int id);

/* returns the data for this id, if any */
void *robot_list_remove_by_id(struct robot_list *l,int id);
/* returns the id for this data; if no match on data ptr, returns -1 */
int robot_list_remove_by_data(struct robot_list *l,void *data);

/* enumeration crap -- very very similiar to java's java.util.Enumeration
 * mumbo-jumbo.
 */
struct robot_list_enum *robot_list_enum(struct robot_list *l);
void *robot_list_enum_next_element(struct robot_list_enum *e);
void robot_list_enum_destroy(struct robot_list_enum *e);

#endif
