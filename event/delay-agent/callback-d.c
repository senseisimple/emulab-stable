/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2000-2002 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * agent-callback-dummy.c --
 *
 *      Delay node agent callback handling.
 */


/******************************* INCLUDES **************************/
#include "main-d.h"
/******************************* INCLUDES **************************/


/******************************* EXTERNS **************************/
extern structlink_map link_map[MAX_LINKS];
extern int link_index;
extern int s_dummy; 
/******************************* EXTERNS **************************/


/********************************FUNCTION DEFS *******************/


/*************************** agent_callback **********************
 This function is called from the event system when an event
 notification is recd. from the server. It checks whether the 
 notification is valid (sanity check). If not print a warning,else
 call handle_pipes which does the rest of thejob
 *************************** agent_callback **********************/

void agent_callback(event_handle_t handle,
		    event_notification_t notification, void *data)
{
  #define MAX_LEN 50

  char objname[MAX_LEN];
  char eventtype[MAX_LEN];
  int l_index = -1;
#ifdef DEBUG
  info ("entering callback \n");
#endif
  
  /* get the name of the object, eg. link0 or link1*/
  if(event_notification_get_string(handle,
                                   notification,
                                   "OBJNAME", objname, MAX_LEN) == 0){
    error("could not get the objname \n");
    return;
  }

  /* check we are handling the objectname for which we have recd an event */
    if ((l_index = check_object(objname)) == -1){
    error("not handling events for this object\n");
    return;
  }
  
  /* get the eventtype, eg up/down/modify*/
  if(event_notification_get_string(handle,
                                   notification,
                                   "EVENTTYPE", eventtype, MAX_LEN) == 0){
    error("could not get the eventtype \n");
    return;
  }

  /* call function to handle this event for this object */
  handle_event(objname, eventtype, notification,handle);
#ifdef DEBUG
  info ("exiting callback \n");
#endif
   
  return;
}

/******************** handle_pipes ***************************************

 ******************** handle_pipes ***************************************/

void handle_event (char *objname, char *eventtype,
		   event_notification_t notification, event_handle_t handle)
{
#ifdef DEBUG
  info ("entering handle_event \n");
#endif

  if(strcmp(eventtype, TBDB_EVENTTYPE_UP) == 0){
      handle_link_up(objname);
    }
    else if(strcmp(eventtype, TBDB_EVENTTYPE_DOWN) == 0){
      handle_link_down(objname);
    }
    else if(strcmp(eventtype, TBDB_EVENTTYPE_MODIFY) == 0){
      handle_link_modify(objname, handle, notification);
    }
    else error("unknown link event type\n");
#ifdef DEBUG
  info ("exiting handle_event \n");
#endif
	   
}

void handle_link_up(char * linkname){
#ifdef DEBUG
  info ("entering handle_link_up \n");
#endif

  info("recd. UP event for link = %s\n", linkname);
#ifdef DEBUG
  info ("exiting handle_link_up \n");
#endif

}

void handle_link_down(char * linkname)
{
#ifdef DEBUG
  info ("entering handle_link_down \n");
#endif
   info("recd. DOWN event for link = %s\n", linkname);
#ifdef DEBUG
  info ("exiting handle_link_down \n");
#endif
}

void handle_link_modify(char *linkname, event_handle_t handle,
			event_notification_t notification)
{
  char argvalue[50];
  char * argtype;
  int bw = 0,  delay = 0;
#ifdef DEBUG
  info ("entering handle_link_modify \n");
#endif

  info("recd. MODIFY event for link = %s\n", linkname);
 
 
  if(event_notification_get_string(handle,
                                   notification,
                                   "ARGS", argvalue, 50) != 0){
    argtype = strtok(argvalue,"=");

    if(strcmp(argtype,"BANDWIDTH")== 0){
      bw = atoi(strtok(NULL," "));
      info("Bandwidth = %d \n", bw);
    }else if (strcmp(argtype,"DELAY")== 0){
      delay = atoi(strtok(NULL," "));
      info("Delay = %d \n", delay);
    }else error("unrecognized argument\n");
  }

#ifdef DEBUG
  info ("exiting handle_link_modify \n");
#endif

}

int check_object (char *objname)
{
  return search(objname);
}


/*********************** search ********************************
This function does a linear search on the link_map and returns
the index of the link_map entry which matches with the objectname
from the event notification

There will hardly be a few entries in this table, so we dont need
anything fancier than a linear search.

*********************** search ********************************/

int search(char* objname){
  /* for now we do a linear search, maybe bin search later*/
  int i;
  for(i = 0; i < link_index; i++){
    if(strcmp(link_map[i].linkname, objname) == 0)
      return i;
  }
  return -1;
}
