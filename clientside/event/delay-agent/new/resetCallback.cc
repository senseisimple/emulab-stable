// resetCallback.cc

#include "lib.hh"


void resetCallback(event_handle_t handle,
                   event_notification_t notification, void *data)
{
  char name[EVENT_BUFFER_SIZE];
  unsigned long token = ~0;

  int errorCode = system("delaysetup -r");

  event_notification_get_int32(handle, notification,
                               "TOKEN", (int32_t *)&token);
  event_notification_get_objname(handle, notification,
                                 name, EVENT_BUFFER_SIZE);

  /* ... notify the scheduler of the completion. */
  event_do(handle,
           EA_Experiment, g::experimentName.c_str(),
           EA_Type, TBDB_OBJECTTYPE_LINK,
           EA_Name, name,
           EA_Event, TBDB_EVENTTYPE_COMPLETE,
           EA_ArgInteger, "ERROR", errorCode,
           EA_ArgInteger, "CTOKEN", token,
           EA_TAG_DONE);
}
