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
del types


MAXHOSTNAMELEN = _tbevent.MAXHOSTNAMELEN
class event_handle(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, event_handle, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, event_handle, name)
    def __repr__(self):
        return "<C event_handle instance at %s>" % (self.this,)
    __swig_setmethods__["server"] = _tbevent.event_handle_server_set
    __swig_getmethods__["server"] = _tbevent.event_handle_server_get
    if _newclass:server = property(_tbevent.event_handle_server_get, _tbevent.event_handle_server_set)
    __swig_setmethods__["status"] = _tbevent.event_handle_status_set
    __swig_getmethods__["status"] = _tbevent.event_handle_status_get
    if _newclass:status = property(_tbevent.event_handle_status_get, _tbevent.event_handle_status_set)
    __swig_setmethods__["keydata"] = _tbevent.event_handle_keydata_set
    __swig_getmethods__["keydata"] = _tbevent.event_handle_keydata_get
    if _newclass:keydata = property(_tbevent.event_handle_keydata_get, _tbevent.event_handle_keydata_set)
    __swig_setmethods__["keylen"] = _tbevent.event_handle_keylen_set
    __swig_getmethods__["keylen"] = _tbevent.event_handle_keylen_get
    if _newclass:keylen = property(_tbevent.event_handle_keylen_get, _tbevent.event_handle_keylen_set)
    __swig_setmethods__["do_loop"] = _tbevent.event_handle_do_loop_set
    __swig_getmethods__["do_loop"] = _tbevent.event_handle_do_loop_get
    if _newclass:do_loop = property(_tbevent.event_handle_do_loop_get, _tbevent.event_handle_do_loop_set)
    __swig_setmethods__["init"] = _tbevent.event_handle_init_set
    __swig_getmethods__["init"] = _tbevent.event_handle_init_get
    if _newclass:init = property(_tbevent.event_handle_init_get, _tbevent.event_handle_init_set)
    __swig_setmethods__["connect"] = _tbevent.event_handle_connect_set
    __swig_getmethods__["connect"] = _tbevent.event_handle_connect_get
    if _newclass:connect = property(_tbevent.event_handle_connect_get, _tbevent.event_handle_connect_set)
    __swig_setmethods__["disconnect"] = _tbevent.event_handle_disconnect_set
    __swig_getmethods__["disconnect"] = _tbevent.event_handle_disconnect_get
    if _newclass:disconnect = property(_tbevent.event_handle_disconnect_get, _tbevent.event_handle_disconnect_set)
    __swig_setmethods__["cleanup"] = _tbevent.event_handle_cleanup_set
    __swig_getmethods__["cleanup"] = _tbevent.event_handle_cleanup_get
    if _newclass:cleanup = property(_tbevent.event_handle_cleanup_get, _tbevent.event_handle_cleanup_set)
    __swig_setmethods__["mainloop"] = _tbevent.event_handle_mainloop_set
    __swig_getmethods__["mainloop"] = _tbevent.event_handle_mainloop_get
    if _newclass:mainloop = property(_tbevent.event_handle_mainloop_get, _tbevent.event_handle_mainloop_set)
    __swig_setmethods__["notify"] = _tbevent.event_handle_notify_set
    __swig_getmethods__["notify"] = _tbevent.event_handle_notify_get
    if _newclass:notify = property(_tbevent.event_handle_notify_get, _tbevent.event_handle_notify_set)
    __swig_setmethods__["subscribe"] = _tbevent.event_handle_subscribe_set
    __swig_getmethods__["subscribe"] = _tbevent.event_handle_subscribe_get
    if _newclass:subscribe = property(_tbevent.event_handle_subscribe_get, _tbevent.event_handle_subscribe_set)
    __swig_setmethods__["unsubscribe"] = _tbevent.event_handle_unsubscribe_set
    __swig_getmethods__["unsubscribe"] = _tbevent.event_handle_unsubscribe_get
    if _newclass:unsubscribe = property(_tbevent.event_handle_unsubscribe_get, _tbevent.event_handle_unsubscribe_set)
    def __init__(self, *args):
        _swig_setattr(self, event_handle, 'this', _tbevent.new_event_handle(*args))
        _swig_setattr(self, event_handle, 'thisown', 1)
    def __del__(self, destroy=_tbevent.delete_event_handle):
        try:
            if self.thisown: destroy(self)
        except: pass

class event_handlePtr(event_handle):
    def __init__(self, this):
        _swig_setattr(self, event_handle, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, event_handle, 'thisown', 0)
        _swig_setattr(self, event_handle,self.__class__,event_handle)
_tbevent.event_handle_swigregister(event_handlePtr)

class event_notification(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, event_notification, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, event_notification, name)
    def __repr__(self):
        return "<C event_notification instance at %s>" % (self.this,)
    __swig_setmethods__["elvin_notification"] = _tbevent.event_notification_elvin_notification_set
    __swig_getmethods__["elvin_notification"] = _tbevent.event_notification_elvin_notification_get
    if _newclass:elvin_notification = property(_tbevent.event_notification_elvin_notification_get, _tbevent.event_notification_elvin_notification_set)
    __swig_setmethods__["has_hmac"] = _tbevent.event_notification_has_hmac_set
    __swig_getmethods__["has_hmac"] = _tbevent.event_notification_has_hmac_get
    if _newclass:has_hmac = property(_tbevent.event_notification_has_hmac_get, _tbevent.event_notification_has_hmac_set)
    def __init__(self, *args):
        _swig_setattr(self, event_notification, 'this', _tbevent.new_event_notification(*args))
        _swig_setattr(self, event_notification, 'thisown', 1)
    def __del__(self, destroy=_tbevent.delete_event_notification):
        try:
            if self.thisown: destroy(self)
        except: pass

class event_notificationPtr(event_notification):
    def __init__(self, this):
        _swig_setattr(self, event_notification, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, event_notification, 'thisown', 0)
        _swig_setattr(self, event_notification,self.__class__,event_notification)
_tbevent.event_notification_swigregister(event_notificationPtr)

class address_tuple(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, address_tuple, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, address_tuple, name)
    def __repr__(self):
        return "<C address_tuple instance at %s>" % (self.this,)
    __swig_setmethods__["site"] = _tbevent.address_tuple_site_set
    __swig_getmethods__["site"] = _tbevent.address_tuple_site_get
    if _newclass:site = property(_tbevent.address_tuple_site_get, _tbevent.address_tuple_site_set)
    __swig_setmethods__["expt"] = _tbevent.address_tuple_expt_set
    __swig_getmethods__["expt"] = _tbevent.address_tuple_expt_get
    if _newclass:expt = property(_tbevent.address_tuple_expt_get, _tbevent.address_tuple_expt_set)
    __swig_setmethods__["group"] = _tbevent.address_tuple_group_set
    __swig_getmethods__["group"] = _tbevent.address_tuple_group_get
    if _newclass:group = property(_tbevent.address_tuple_group_get, _tbevent.address_tuple_group_set)
    __swig_setmethods__["host"] = _tbevent.address_tuple_host_set
    __swig_getmethods__["host"] = _tbevent.address_tuple_host_get
    if _newclass:host = property(_tbevent.address_tuple_host_get, _tbevent.address_tuple_host_set)
    __swig_setmethods__["objtype"] = _tbevent.address_tuple_objtype_set
    __swig_getmethods__["objtype"] = _tbevent.address_tuple_objtype_get
    if _newclass:objtype = property(_tbevent.address_tuple_objtype_get, _tbevent.address_tuple_objtype_set)
    __swig_setmethods__["objname"] = _tbevent.address_tuple_objname_set
    __swig_getmethods__["objname"] = _tbevent.address_tuple_objname_get
    if _newclass:objname = property(_tbevent.address_tuple_objname_get, _tbevent.address_tuple_objname_set)
    __swig_setmethods__["eventtype"] = _tbevent.address_tuple_eventtype_set
    __swig_getmethods__["eventtype"] = _tbevent.address_tuple_eventtype_get
    if _newclass:eventtype = property(_tbevent.address_tuple_eventtype_get, _tbevent.address_tuple_eventtype_set)
    __swig_setmethods__["scheduler"] = _tbevent.address_tuple_scheduler_set
    __swig_getmethods__["scheduler"] = _tbevent.address_tuple_scheduler_get
    if _newclass:scheduler = property(_tbevent.address_tuple_scheduler_get, _tbevent.address_tuple_scheduler_set)
    __swig_setmethods__["timeline"] = _tbevent.address_tuple_timeline_set
    __swig_getmethods__["timeline"] = _tbevent.address_tuple_timeline_get
    if _newclass:timeline = property(_tbevent.address_tuple_timeline_get, _tbevent.address_tuple_timeline_set)
    def __init__(self, *args):
        _swig_setattr(self, address_tuple, 'this', _tbevent.new_address_tuple(*args))
        _swig_setattr(self, address_tuple, 'thisown', 1)
    def __del__(self, destroy=_tbevent.delete_address_tuple):
        try:
            if self.thisown: destroy(self)
        except: pass

class address_tuplePtr(address_tuple):
    def __init__(self, this):
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

event_register_withkeyfile_withretry = _tbevent.event_register_withkeyfile_withretry

event_register_withkeydata_withretry = _tbevent.event_register_withkeydata_withretry

event_unregister = _tbevent.event_unregister

c_event_poll = _tbevent.c_event_poll

c_event_poll_blocking = _tbevent.c_event_poll_blocking

dont_use_this_function_because_it_does_not_work = _tbevent.dont_use_this_function_because_it_does_not_work

event_stop_main = _tbevent.event_stop_main

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

event_subscribe_auth = _tbevent.event_subscribe_auth

event_async_subscribe = _tbevent.event_async_subscribe

event_unsubscribe = _tbevent.event_unsubscribe

event_async_unsubscribe = _tbevent.event_async_unsubscribe

event_notification_insert_hmac = _tbevent.event_notification_insert_hmac

event_notification_pack = _tbevent.event_notification_pack

event_notification_unpack = _tbevent.event_notification_unpack

event_set_idle_period = _tbevent.event_set_idle_period

event_arg_get = _tbevent.event_arg_get

event_arg_dup = _tbevent.event_arg_dup
EA_TAG_DONE = _tbevent.EA_TAG_DONE
EA_Site = _tbevent.EA_Site
EA_Experiment = _tbevent.EA_Experiment
EA_Group = _tbevent.EA_Group
EA_Host = _tbevent.EA_Host
EA_Type = _tbevent.EA_Type
EA_Name = _tbevent.EA_Name
EA_Event = _tbevent.EA_Event
EA_Arguments = _tbevent.EA_Arguments
EA_ArgInteger = _tbevent.EA_ArgInteger
EA_ArgFloat = _tbevent.EA_ArgFloat
EA_ArgString = _tbevent.EA_ArgString
EA_When = _tbevent.EA_When

event_notification_create_v = _tbevent.event_notification_create_v

event_notification_create = _tbevent.event_notification_create

event_do_v = _tbevent.event_do_v

event_do = _tbevent.event_do

xmalloc = _tbevent.xmalloc

xrealloc = _tbevent.xrealloc
class timeval(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, timeval, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, timeval, name)
    def __repr__(self):
        return "<C timeval instance at %s>" % (self.this,)
    __swig_setmethods__["tv_sec"] = _tbevent.timeval_tv_sec_set
    __swig_getmethods__["tv_sec"] = _tbevent.timeval_tv_sec_get
    if _newclass:tv_sec = property(_tbevent.timeval_tv_sec_get, _tbevent.timeval_tv_sec_set)
    __swig_setmethods__["tv_usec"] = _tbevent.timeval_tv_usec_set
    __swig_getmethods__["tv_usec"] = _tbevent.timeval_tv_usec_get
    if _newclass:tv_usec = property(_tbevent.timeval_tv_usec_get, _tbevent.timeval_tv_usec_set)
    def __init__(self, *args):
        _swig_setattr(self, timeval, 'this', _tbevent.new_timeval(*args))
        _swig_setattr(self, timeval, 'thisown', 1)
    def __del__(self, destroy=_tbevent.delete_timeval):
        try:
            if self.thisown: destroy(self)
        except: pass

class timevalPtr(timeval):
    def __init__(self, this):
        _swig_setattr(self, timeval, 'this', this)
        if not hasattr(self,"thisown"): _swig_setattr(self, timeval, 'thisown', 0)
        _swig_setattr(self, timeval,self.__class__,timeval)
_tbevent.timeval_swigregister(timevalPtr)

class callback_data(_object):
    __swig_setmethods__ = {}
    __setattr__ = lambda self, name, value: _swig_setattr(self, callback_data, name, value)
    __swig_getmethods__ = {}
    __getattr__ = lambda self, name: _swig_getattr(self, callback_data, name)
    def __repr__(self):
        return "<C callback_data instance at %s>" % (self.this,)
    __swig_setmethods__["callback_notification"] = _tbevent.callback_data_callback_notification_set
    __swig_getmethods__["callback_notification"] = _tbevent.callback_data_callback_notification_get
    if _newclass:callback_notification = property(_tbevent.callback_data_callback_notification_get, _tbevent.callback_data_callback_notification_set)
    __swig_setmethods__["next"] = _tbevent.callback_data_next_set
    __swig_getmethods__["next"] = _tbevent.callback_data_next_get
    if _newclass:next = property(_tbevent.callback_data_next_get, _tbevent.callback_data_next_set)
    def __init__(self, *args):
        _swig_setattr(self, callback_data, 'this', _tbevent.new_callback_data(*args))
        _swig_setattr(self, callback_data, 'thisown', 1)
    def __del__(self, destroy=_tbevent.delete_callback_data):
        try:
            if self.thisown: destroy(self)
        except: pass

class callback_dataPtr(callback_data):
    def __init__(self, this):
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

import sys
import time

class NotificationWrapper:
    """
    Wrapper class for event_notification structures.  Mostly just adds setter
    and getter methods.
    """

    def __init__(self, handle, notification):
        """
        Construct a NotificationWrapper that wraps the given objects.
        
        @param handle The event_handle used to create notification.
        @param notification The event_notification structure to wrap.
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

    def getTimeline(self, args):
        return event_notification_get_timeline(self.handle, self.notification)

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

        @param handle The event_handle being polled.
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


class EventError:
    """
    Base class for event related exceptions.
    """
    def __init__(self, code):
        self.args = code,
        return
    
    pass


class EventTimedOutError(EventError):
    """
    Exception raised when the run() method of EventClient timed out.
    """
    def __init__(self, timeout):
        self.args = timeout,
        return
    
    pass


# Ugh, elvin likes to crash if we unregister all of the handles, so we keep
# a dummy one around just to keep things happy.
_hack_handle = None

class EventClient:
    """
    Event client class, mostly just wraps the SWIG'd versions of the functions.
    """
    
    def __init__(self, server=None, port=None, url=None, keyfile=None):
        """
        Construct an EventClient object.

        @param url The server name in url'ish form (e.g. elvin://boss)
        @param server The name of the server.
        @param port The server port number.
        """
        global _hack_handle
        
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
            pass

        if keyfile:
            self.handle = event_register_withkeyfile(url, 0, keyfile)
            pass
        else:
            self.handle = event_register(url, 0)
            pass
        
        self.timeout = 0
        
        if not _hack_handle:
            # Open a handle for the sole purpose of keeping the event library
            # from calling the elvin cleanup function, because elvin likes to
            # segfault when it has been init'd/clean'd multiple times.
            _hack_handle = event_register(url, 0)
            pass
        
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

        @return None to continue processing events, any other value to stop.
        """
        return None

    def subscribe(self, tuple):
        """
        Subscribe to some events.

        @param tuple The address tuple describing the subscription.
        """
        return stub_event_subscribe(self.handle, tuple.this)

    def create_notification(self, tuple):
        """
        @return A notification that is bound to this client.
        """
        return NotificationWrapper(self.handle,
                                   event_notification_alloc(self.handle,
                                                            tuple.this))

    def notify(self, en):
        """
        Send a notification.
        """
        return event_notify(self.handle, en.notification)

    def schedule(self, en, tv):
        """
        Schedule a notification.
        """
        return event_schedule(self.handle, en.notification, tv)

    def set_timeout(self, timeout):
        """
        @param timeout The timeout, in seconds, for the run() loop.
        """
        self.timeout = timeout * 1000
        return

    def run(self):
        """
        Main loop used to wait for and process events.

        @return The not None value returned by handle_event.
        """
        retval = None
        while not retval:
            rc = c_event_poll_blocking(self.handle, self.timeout)
            if rc != 0:
                sys.stderr.write("c_event_poll_blocking: " + str(rc) + "\n")
                raise EventError, rc
            else:
                for ev in self._callbacks():
                    retval = self.handle_event(ev)
                    if retval:
                        # Not None return value, stop the bus.
                        break
                    pass
                else:
                    if self.timeout != 0:
                        # We're making a bit of an assumption here that no
                        # callbacks means a timeout occurred, oh well.
                        raise EventTimedOutError, self.timeout
                    pass
                
                if not retval:
                    time.sleep(0.1) # Forcefully slow down the poll.
                    pass
                pass
            pass
        return retval

    def poll(self):
        """
        Polling interface; returns events to caller, one at a time.
        """

        while True:
            try:
                #
                # First see if anything not yet delivered.
                #
                ev = CallbackIterator(self.handle).next()
                return ev;
            except StopIteration, e:
                pass
            
            rc = c_event_poll_blocking(self.handle, 0)        
            if rc != 0:
                sys.stderr.write("c_event_poll_blocking: " + str(rc) + "\n")
                raise IOError, "Reading events"
            pass
    pass

