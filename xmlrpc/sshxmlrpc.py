#
# EMULAB-COPYRIGHT
# Copyright (c) 2004 University of Utah and the Flux Group.
# All rights reserved.
#
import sys
import types
import urllib
import popen2
import xmlrpclib

##
# Use SSH to transport XML-RPC requests/responses
#
class SSHTransport:
    
    ##
    # Send a request to the destination.
    #
    # @param host The host name on which to execute the request
    # @param handler The python file that will handle the request.
    # @param request_body The XML-RPC encoded request.
    # @param verbose unused.
    # @return The value returned 
    #
    def request(self, host, handler, request_body, verbose=0):
        # Strip the leading slash in the handler, if there is one.
        if handler.startswith('/'):
            handler = handler[1:]
            pass
        
        # SSH to the host and call python on the handler.
        self.myChild = popen2.Popen3("ssh -x "
                                     + host
                                     + " 'python "
                                     + handler
                                     + "'")

        # Send the request over SSH's stdin,
        self.myChild.tochild.write(request_body)
        # ... close to signal the end of the request,
        self.myChild.tochild.close() # XXX Do something smarter here.

        # ... parse the response, and
        retval = self.parse_response()

        # ... wait for SSH to terminate.
        self.myChild.wait()

        return retval

    ##
    # @return A tuple containing the parser and unmarshaller, in that order
    #
    def getparser(self):
        return xmlrpclib.getparser()

    ##
    # Parse the response from the server.
    #
    # @return The python value returned by the server method.
    #
    def parse_response(self):
        parser, unmarshaller = self.getparser()

        while True:
            response = self.myChild.fromchild.read(1024)
            if not response:
                break
            parser.feed(response)
            pass

        return unmarshaller.close()
    
    pass


##
# A wrapper for objects that should be exported via an XML-RPC interface.  Any
# method calls received via XML-RPC will automatically be translated into real
# python calls.
#
class SSHServerWrapper:

    ##
    # Initialize this object.
    #
    # @param object The object to wrap.
    #
    def __init__(self, object):
        self.myObject = object
        return

    ##
    # Handle a single request from a client.  The method will read the request
    # from the client, dispatch the method, and write the response back to the
    # client.
    #
    # @param streams A pair containing the input and output streams.
    #
    def handle_request(self, streams):
        try:
            # Read the request,
            params, method = xmlrpclib.loads(streams[0].read())
            # ... find the corresponding method in the wrapped object,
            meth = getattr(self.myObject, method)
            # ... dispatch the method, and
            if type(meth) == type(self.handle_request):
                response = apply(meth, params) # It is really a method.
                pass
            else:
                response = str(meth) # Is is just a plain variable.
                pass
            # ... ensure there was a valid response.
            if type(response) != type((  )):
                response = (response,)
                pass
            pass
        except xmlrpclib.Fault, faultobj:
            # An XML-RPC related fault occurred, just encode the response.
            response = xmlrpclib.dumps(faultobj)
            pass
        except:
            # Some other exception happened, convert it to an XML-RPC fault.
            response = xmlrpclib.dumps(
                xmlrpclib.Fault(1, "%s:%s" % (sys.exc_type, sys.exc_value)))
            pass
        else:
            # Everything worked, encode the real response.
            response = xmlrpclib.dumps(response, methodresponse=1)
            pass

        # Finally, send the reply to the client.
        streams[1].write(response)

        return

    pass


##
# A proxy for XML-RPC servers that are accessible via SSH.
#
class SSHServerProxy:

    ##
    # Initialize the object.
    #
    # @param uri The URI for the server.  Must start with 'ssh'.
    # @param transport A python object that implements the Transport interface.
    # The default is to use a new SSHTransport object.
    # @param encoding Content encoding.
    # @param verbose unused.
    #
    def __init__(self, uri, transport=None, encoding=None, verbose=0):
        type, uri = urllib.splittype(uri)
        if type not in ("ssh", ):
            raise IOError, "unsupported XML-RPC protocol"

        self.__host, self.__handler = urllib.splithost(uri)

        if transport is None:
            transport = SSHTransport()
            pass
        
        self.__transport = transport

        self.__encoding = encoding
        self.__verbose = verbose
        return

    ##
    # Send a request to the server.
    #
    # @param methodname The name of the method.
    # @param params The parameters for the method.
    #
    def __request(self, methodname, params):
        # Convert the method name and parameters to a string,
        request = xmlrpclib.dumps(params, methodname, encoding=self.__encoding)

        # ... use the transport to send the request and receive the reply, and
        response = self.__transport.request(
            self.__host,
            self.__handler,
            request,
            verbose=self.__verbose
            )

        # ... ensure there was a valid reply.
        if len(response) == 1:
            response = response[0]
            pass

        return response

    def __repr__(self):
        return (
            "<ServerProxy for %s%s>" %
            (self.__host, self.__handler)
            )

    __str__ = __repr__

    def __getattr__(self, name):
        # magic method dispatcher
        return xmlrpclib._Method(self.__request, name)
