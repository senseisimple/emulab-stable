# This file was created automatically by SWIG.
# Don't modify this file, modify the SWIG interface instead.
# This file is compatible with both classic and new-style classes.
import _tbevent
def _swig_setattr(self,class_type,name,value):
    if (name == "this"):
        if isinstance(value, class_type):
            self.__dict__[name] = value.this
            if hasattr(value,"thisown"): self.__dict__["thisown"] = value.thisown
            del value.thisown
            return
    method = class_type.__swig_setmethods__.get(name,None)
    if method: return method(self,value)
    self.__dict__[name] = value

def _swig_getattr(self,class_type,name):
    method = class_type.__swig_getmethods__.get(name,None)
    if method: return method(self)
    raise AttributeError,name

import types
try:
    _object = types.ObjectType
    _newclass = 1
except AttributeError:
    class _object : pass
    _newclass = 0


MAXHOSTNAMELEN = _tbevent.MAXHOSTNAMELEN
class event_handle(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, event_handle, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, event_handle, name)
    __swig_setmethods__["server"] = _tbevent.event_handle_server_set
    __swig_getmethods__["server"] = _tbevent.event_handle_server_get
    if _newclass:server = property(_tbevent.event_handle_server_get,_tbevent.event_handle_server_set)
    __swig_setmethods__["status"] = _tbevent.event_handle_status_set
    __swig_getmethods__["status"] = _tbevent.event_handle_status_get
    if _newclass:status = property(_tbevent.event_handle_status_get,_tbevent.event_handle_status_set)
    __swig_setmethods__["keydata"] = _tbevent.event_handle_keydata_set
    __swig_getmethods__["keydata"] = _tbevent.event_handle_keydata_get
    if _newclass:keydata = property(_tbevent.event_handle_keydata_get,_tbevent.event_handle_keydata_set)
    __swig_setmethods__["keylen"] = _tbevent.event_handle_keylen_set
    __swig_getmethods__["keylen"] = _tbevent.event_handle_keylen_get
    if _newclass:keylen = property(_tbevent.event_handle_keylen_get,_tbevent.event_handle_keylen_set)
    __swig_setmethods__["init"] = _tbevent.event_handle_init_set
    __swig_getmethods__["init"] = _tbevent.event_handle_init_get
    if _newclass:init = property(_tbevent.event_handle_init_get,_tbevent.event_handle_init_set)
    __swig_setmethods__["connect"] = _tbevent.event_handle_connect_set
    __swig_getmethods__["connect"] = _tbevent.event_handle_connect_get
    if _newclass:connect = property(_tbevent.event_handle_connect_get,_tbevent.event_handle_connect_set)
    __swig_setmethods__["disconnect"] = _tbevent.event_handle_disconnect_set
    __swig_getmethods__["disconnect"] = _tbevent.event_handle_disconnect_get
    if _newclass:disconnect = property(_tbevent.event_handle_disconnect_get,_tbevent.event_handle_disconnect_set)
    __swig_setmethods__["cleanup"] = _tbevent.event_handle_cleanup_set
    __swig_getmethods__["cleanup"] = _tbevent.event_handle_cleanup_get
    if _newclass:cleanup = property(_tbevent.event_handle_cleanup_get,_tbevent.event_handle_cleanup_set)
    __swig_setmethods__["mainloop"] = _tbevent.event_handle_mainloop_set
    __swig_getmethods__["mainloop"] = _tbevent.event_handle_mainloop_get
    if _newclass:mainloop = property(_tbevent.event_handle_mainloop_get,_tbevent.event_handle_mainloop_set)
    __swig_setmethods__["notify"] = _tbevent.event_handle_notify_set
    __swig_getmethods__["notify"] = _tbevent.event_handle_notify_get
    if _newclass:notify = property(_tbevent.event_handle_notify_get,_tbevent.event_handle_notify_set)
    __swig_setmethods__["subscribe"] = _tbevent.event_handle_subscribe_set
    __swig_getmethods__["subscribe"] = _tbevent.event_handle_subscribe_get
    if _newclass:subscribe = property(_tbevent.event_handle_subscribe_get,_tbevent.event_handle_subscribe_set)
    def __init__(self,*args):
        _swig_setattr(self, event_handle, 'this', apply(_tbevent.new_event_handle,args))
        _swig_setattr(self, event_handle, 'thisown', 1)
    def __del__(self, destroy= _tbevent.delete_event_handle):
        try:
            if self.thisown: destroy(self)
        except: pass
    def __repr__(self):
        return "<C event_handle instance at %s>" % (self.this,)

class event_handlePtr(event_handle):
    def __init__(self,this):
        _swig_setattr(self, event_handle, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, event_handle, 'thisown', 0)
        _swig_setattr(self, event_handle,self.__class__,event_handle)
_tbevent.event_handle_swigregister(event_handlePtr)

class event_notification(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, event_notification, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, event_notification, name)
    __swig_setmethods__["elvin_notification"] = _tbevent.event_notification_elvin_notification_set
    __swig_getmethods__["elvin_notification"] = _tbevent.event_notification_elvin_notification_get
    if _newclass:elvin_notification = property(_tbevent.event_notification_elvin_notification_get,_tbevent.event_notification_elvin_notification_set)
    __swig_setmethods__["has_hmac"] = _tbevent.event_notification_has_hmac_set
    __swig_getmethods__["has_hmac"] = _tbevent.event_notification_has_hmac_get
    if _newclass:has_hmac = property(_tbevent.event_notification_has_hmac_get,_tbevent.event_notification_has_hmac_set)
    def __init__(self,*args):
        _swig_setattr(self, event_notification, 'this', apply(_tbevent.new_event_notification,args))
        _swig_setattr(self, event_notification, 'thisown', 1)
    def __del__(self, destroy= _tbevent.delete_event_notification):
        try:
            if self.thisown: destroy(self)
        except: pass
    def __repr__(self):
        return "<C event_notification instance at %s>" % (self.this,)

class event_notificationPtr(event_notification):
    def __init__(self,this):
        _swig_setattr(self, event_notification, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, event_notification, 'thisown', 0)
        _swig_setattr(self, event_notification,self.__class__,event_notification)
_tbevent.event_notification_swigregister(event_notificationPtr)

class address_tuple(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, address_tuple, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, address_tuple, name)
    __swig_setmethods__["site"] = _tbevent.address_tuple_site_set
    __swig_getmethods__["site"] = _tbevent.address_tuple_site_get
    if _newclass:site = property(_tbevent.address_tuple_site_get,_tbevent.address_tuple_site_set)
    __swig_setmethods__["expt"] = _tbevent.address_tuple_expt_set
    __swig_getmethods__["expt"] = _tbevent.address_tuple_expt_get
    if _newclass:expt = property(_tbevent.address_tuple_expt_get,_tbevent.address_tuple_expt_set)
    __swig_setmethods__["group"] = _tbevent.address_tuple_group_set
    __swig_getmethods__["group"] = _tbevent.address_tuple_group_get
    if _newclass:group = property(_tbevent.address_tuple_group_get,_tbevent.address_tuple_group_set)
    __swig_setmethods__["host"] = _tbevent.address_tuple_host_set
    __swig_getmethods__["host"] = _tbevent.address_tuple_host_get
    if _newclass:host = property(_tbevent.address_tuple_host_get,_tbevent.address_tuple_host_set)
    __swig_setmethods__["objtype"] = _tbevent.address_tuple_objtype_set
    __swig_getmethods__["objtype"] = _tbevent.address_tuple_objtype_get
    if _newclass:objtype = property(_tbevent.address_tuple_objtype_get,_tbevent.address_tuple_objtype_set)
    __swig_setmethods__["objname"] = _tbevent.address_tuple_objname_set
    __swig_getmethods__["objname"] = _tbevent.address_tuple_objname_get
    if _newclass:objname = property(_tbevent.address_tuple_objname_get,_tbevent.address_tuple_objname_set)
    __swig_setmethods__["eventtype"] = _tbevent.address_tuple_eventtype_set
    __swig_getmethods__["eventtype"] = _tbevent.address_tuple_eventtype_get
    if _newclass:eventtype = property(_tbevent.address_tuple_eventtype_get,_tbevent.address_tuple_eventtype_set)
    __swig_setmethods__["scheduler"] = _tbevent.address_tuple_scheduler_set
    __swig_getmethods__["scheduler"] = _tbevent.address_tuple_scheduler_get
    if _newclass:scheduler = property(_tbevent.address_tuple_scheduler_get,_tbevent.address_tuple_scheduler_set)
    def __init__(self,*args):
        _swig_setattr(self, address_tuple, 'this', apply(_tbevent.new_address_tuple,args))
        _swig_setattr(self, address_tuple, 'thisown', 1)
    def __del__(self, destroy= _tbevent.delete_address_tuple):
        try:
            if self.thisown: destroy(self)
        except: pass
    def __repr__(self):
        return "<C address_tuple instance at %s>" % (self.this,)

class address_tuplePtr(address_tuple):
    def __init__(self,this):
        _swig_setattr(self, address_tuple, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, address_tuple, 'thisown', 0)
        _swig_setattr(self, address_tuple,self.__class__,address_tuple)
_tbevent.address_tuple_swigregister(address_tuplePtr)

ADDRESSTUPLE_ALL = _tbevent.ADDRESSTUPLE_ALL
OBJECTTYPE_TESTBED = _tbevent.OBJECTTYPE_TESTBED
OBJECTTYPE_TRAFGEN = _tbevent.OBJECTTYPE_TRAFGEN
address_tuple_alloc = _tbevent.address_tuple_alloc

address_tuple_free = _tbevent.address_tuple_free

EVENT_HOST_ANY = _tbevent.EVENT_HOST_ANY
EVENT_NULL = _tbevent.EVENT_NULL
EVENT_TEST = _tbevent.EVENT_TEST
EVENT_SCHEDULE = _tbevent.EVENT_SCHEDULE
EVENT_TRAFGEN_START = _tbevent.EVENT_TRAFGEN_START
EVENT_TRAFGEN_STOP = _tbevent.EVENT_TRAFGEN_STOP
event_register = _tbevent.event_register

event_register_withkeyfile = _tbevent.event_register_withkeyfile

event_register_withkeydata = _tbevent.event_register_withkeydata

event_unregister = _tbevent.event_unregister

c_event_poll = _tbevent.c_event_poll

c_event_poll_blocking = _tbevent.c_event_poll_blocking

dont_use_this_function_because_it_does_not_work = _tbevent.dont_use_this_function_because_it_does_not_work

event_notify = _tbevent.event_notify

event_schedule = _tbevent.event_schedule

event_notification_alloc = _tbevent.event_notification_alloc

event_notification_free = _tbevent.event_notification_free

event_notification_clone = _tbevent.event_notification_clone

event_notification_get_double = _tbevent.event_notification_get_double

event_notification_get_int32 = _tbevent.event_notification_get_int32

event_notification_get_int64 = _tbevent.event_notification_get_int64

event_notification_get_opaque = _tbevent.event_notification_get_opaque

c_event_notification_get_string = _tbevent.c_event_notification_get_string

event_notification_put_double = _tbevent.event_notification_put_double

event_notification_put_int32 = _tbevent.event_notification_put_int32

event_notification_put_int64 = _tbevent.event_notification_put_int64

event_notification_put_opaque = _tbevent.event_notification_put_opaque

event_notification_put_string = _tbevent.event_notification_put_string

event_notification_remove = _tbevent.event_notification_remove

c_event_subscribe = _tbevent.c_event_subscribe

event_notification_insert_hmac = _tbevent.event_notification_insert_hmac

event_notification_pack = _tbevent.event_notification_pack

event_notification_unpack = _tbevent.event_notification_unpack

xmalloc = _tbevent.xmalloc

xrealloc = _tbevent.xrealloc

class callback_data(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, callback_data, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, callback_data, name)
    __swig_setmethods__["callback_notification"] = _tbevent.callback_data_callback_notification_set
    __swig_getmethods__["callback_notification"] = _tbevent.callback_data_callback_notification_get
    if _newclass:callback_notification = property(_tbevent.callback_data_callback_notification_get,_tbevent.callback_data_callback_notification_set)
    __swig_setmethods__["next"] = _tbevent.callback_data_next_set
    __swig_getmethods__["next"] = _tbevent.callback_data_next_get
    if _newclass:next = property(_tbevent.callback_data_next_get,_tbevent.callback_data_next_set)
    def __init__(self,*args):
        _swig_setattr(self, callback_data, 'this', apply(_tbevent.new_callback_data,args))
        _swig_setattr(self, callback_data, 'thisown', 1)
    def __del__(self, destroy= _tbevent.delete_callback_data):
        try:
            if self.thisown: destroy(self)
        except: pass
    def __repr__(self):
        return "<C callback_data instance at %s>" % (self.this,)

class callback_dataPtr(callback_data):
    def __init__(self,this):
        _swig_setattr(self, callback_data, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, callback_data, 'thisown', 0)
        _swig_setattr(self, callback_data,self.__class__,callback_data)
_tbevent.callback_data_swigregister(callback_dataPtr)

allocate_callback_data = _tbevent.allocate_callback_data

free_callback_data = _tbevent.free_callback_data

dequeue_callback_data = _tbevent.dequeue_callback_data

enqueue_callback_data = _tbevent.enqueue_callback_data

perl_stub_callback = _tbevent.perl_stub_callback

stub_event_subscribe = _tbevent.stub_event_subscribe

event_notification_get_string = _tbevent.event_notification_get_string

event_notification_get_site = _tbevent.event_notification_get_site

event_notification_get_expt = _tbevent.event_notification_get_expt

event_notification_get_group = _tbevent.event_notification_get_group

event_notification_get_host = _tbevent.event_notification_get_host

event_notification_get_objtype = _tbevent.event_notification_get_objtype

event_notification_get_objname = _tbevent.event_notification_get_objname

event_notification_get_eventtype = _tbevent.event_notification_get_eventtype

event_notification_get_arguments = _tbevent.event_notification_get_arguments

event_notification_set_arguments = _tbevent.event_notification_set_arguments

event_notification_get_sender = _tbevent.event_notification_get_sender

event_notification_set_sender = _tbevent.event_notification_set_sender

cvar = _tbevent.cvar

# -*- python -*-
#
# CODE PAST THIS POINT WAS NOT AUTOMATICALLY GENERATED BY SWIG
#
# For now, this has to get cat'ed onto the end of tbevent.py, since it
# doesn't seem possible to get SWIG to just pass it through into the
# output file
#

import time

class NotificationWrapper:
    """
    Wrapper class for event_notification structures.  Mostly just adds setter
    and getter methods.
    """

    def __init__(self, handle, notification):
        """
        Construct a NotificationWrapper that wraps the given objects.
        
        handle - The event_handle used to create notification.
        notification - The event_notification structure to wrap.
        """
        self.handle = handle
        self.notification = notification
        return

    def __del__(self):
        """
        Deconstruct the object by free'ing the wrapped notification.
        """
        event_notification_free(self.handle, self.notification)
        return

    # For the rest of these, consult the C header file, event.h.

    def getSite(self):
        return event_notification_get_site(self.handle, self.notification)

    def getExpt(self):
        return event_notification_get_expt(self.handle, self.notification)

    def getGroup(self):
        return event_notification_get_group(self.handle, self.notification)

    def getHost(self):
        return event_notification_get_host(self.handle, self.notification)

    def getObjType(self):
        return event_notification_get_objtype(self.handle, self.notification)

    def getObjName(self):
        return event_notification_get_objname(self.handle, self.notification)

    def getEventType(self):
        return event_notification_get_eventtype(self.handle, self.notification)

    def getArguments(self):
        return event_notification_get_arguments(self.handle, self.notification)

    def setArguments(self, args):
        return event_notification_set_arguments(self.handle,
                                                self.notification,
                                                args)

    def getSender(self):
        return event_notification_get_sender(self.handle, self.notification)

    def setSender(self, sender):
        return event_notification_set_sender(self.handle,
                                             self.notification,
                                             sender)

    pass


class CallbackIterator:
    """
    Python iterator for the callback list created by the SWIG stubs.
    """

    def __init__(self, handle):
        """
        Construct an iterator with the given arguments.

        handle - The event_handle being polled.
        """
        self.last = None
        self.handle = handle
        return

    def __del__(self):
        """
        Deconstruct the iterator.
        """
        if self.last:
            free_callback_data(self.last)
            self.last = None
            pass
        return

    def __iter__(self):
        return self

    def next(self):
        """
        Return the next object in the sequence or raise StopIteration if there
        are no more.  The returned object is a wrapped notification.
        """
        if self.last:
            free_callback_data(self.last)
            self.last = None
            pass
        self.last = dequeue_callback_data()
        if not self.last:
            raise StopIteration
        
        return NotificationWrapper(self.handle,
                                   self.last.callback_notification)

    pass


class EventClient:
    """
    Event client class, mostly just wraps the SWIG'd versions of the functions.
    """

    def __init__(self, server=None, port=None, url=None):
        """
        Construct an EventClient object.

        url - The server name in url'ish form (e.g. elvin://boss)
        server - The name of the server.
        port - The server port number.
        """

        if url:
            if not url.startswith("elvin:"):
                raise ValueError, "malformed url: " + url
            pass
        else:
            if not server:
                raise ValueError, "url or server must be given"
            url = "elvin://" + server
            if port and len(port) > 0:
                url = url + ":" + port
            pass
        self.handle = event_register(url, 0)
        return

    def __del__(self):
        """
        Deconstruct this object by disconnecting from the server.
        """
        event_unregister(self.handle)
        self.handle = None
        return

    def _callbacks(self):
        """
        Return an iterator that traverses the list of callbacks generated by
        the SWIG wrapper.
        """
        return CallbackIterator(self.handle)

    def handle_event(self, ev):
        """
        Default implementation of the event handling method.  Should be
        overridden by subclasses.
        """
        return

    def subscribe(self, tuple):
        """
        Subscribe to some events.

        tuple - The address tuple describing the subscription.
        """
        return stub_event_subscribe(self.handle, tuple.this)

    def create_notification(self, tuple):
        """
        return a notification that is bound to this client.
        """
        return NotificationWrapper(self.handle,
                                   event_notification_alloc(self.handle,
                                                            tuple.this))

    def notify(self, en):
        """
        Send a notification.
        """
        return event_notify(self.handle, en.notification)

    def run(self):
        """
        Main loop used to wait for and process events.
        """
        while True:
            rc = c_event_poll_blocking(self.handle, 0)
            if rc != 0:
                sys.stderr.write("c_event_poll_blocking: " + str(rc) + "\n")
                pass
            else:
                for ev in self._callbacks():
                    self.handle_event(ev)
                    pass
                time.sleep(0.1)
                pass
            pass
        return

    pass

