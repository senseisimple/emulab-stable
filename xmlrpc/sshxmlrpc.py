#
# EMULAB-COPYRIGHT
# Copyright (c) 2004 University of Utah and the Flux Group.
# All rights reserved.
#
# Permission to use, copy, modify and distribute this software is hereby
# granted provided that (1) source code retains these copyright, permission,
# and disclaimer notices, and (2) redistributions including binaries
# reproduce the notices in supporting documentation.
#
# THE UNIVERSITY OF UTAH ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
# CONDITION.  THE UNIVERSITY OF UTAH DISCLAIMS ANY LIABILITY OF ANY KIND
# FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
#
############################################################################
# Some bits of this file are from xmlrpclib.py, which is:
# --------------------------------------------------------------------
# Copyright (c) 1999-2002 by Secret Labs AB
# Copyright (c) 1999-2002 by Fredrik Lundh
#
# By obtaining, using, and/or copying this software and/or its
# associated documentation, you agree that you have read, understood,
# and will comply with the following terms and conditions:
#
# Permission to use, copy, modify, and distribute this software and
# its associated documentation for any purpose and without fee is
# hereby granted, provided that the above copyright notice appears in
# all copies, and that both that copyright notice and this permission
# notice appear in supporting documentation, and that the name of
# Secret Labs AB or the author not be used in advertising or publicity
# pertaining to distribution of the software without specific, written
# prior permission.
#
# SECRET LABS AB AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD
# TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANT-
# ABILITY AND FITNESS.  IN NO EVENT SHALL SECRET LABS AB OR THE AUTHOR
# BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
# DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
# WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
# ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
# OF THIS SOFTWARE.
# --------------------------------------------------------------------
#
import os
import sys
import types
import urllib
import popen2
import rfc822
import xmlrpclib
import syslog

# XXX This should come from configure.
LOG_TESTBED = syslog.LOG_LOCAL5;

##
# Base class for exceptions in this module.
#
class SSHException(Exception):
    pass

##
# Indicates a poorly formatted response from the server.
#
class BadResponse(SSHException):

    ##
    # @param host The server host name.
    # @param handler The handler being accessed on the server.
    # @param arg Description of the problem.
    #
    def __init__(self, host, handler, msg):
        self.args = host, handler, msg,
        return
    
    pass

##
# Class used to decode headers.
#
class SSHMessage(rfc822.Message):
    pass

##
# An SSH based connection class.
#
class SSHConnection:

    ##
    # @param host The peer host name.
    # @param handler The handler being accessed.
    # @param streams A pair containing the input and output files respectively.
    # If this value is not given, ssh will be used to connect to the host.
    # @param ssh_config The ssh config file to use when initiating a new
    # connection.
    # 
    def __init__(self, host, handler, streams=None, ssh_config=None):
        # Store information about the peer and
        self.handler = handler
        self.host = host

        # ... initialize the read and write file objects.
        if streams:
            self.myChild = None
            self.rfile = streams[0]
            self.wfile = streams[1]
            pass
        else:
            self.user, self.host = urllib.splituser(self.host)
            # print self.user + " " + self.host + " " + handler
            
            flags = ""
            if self.user:
                flags = flags + " -l " + self.user
                pass
            if ssh_config:
                flags = flags + " -F " + ssh_config
                pass
            self.myChild = popen2.Popen3("ssh -x -C -o 'CompressionLevel 5' "
                                         + flags
                                         + " "
                                         + self.host
                                         + " "
                                         + handler,
                                         1)
            self.rfile = self.myChild.fromchild
            self.wfile = self.myChild.tochild
            pass
        return

    ##
    # @param len The amount of data to read. (Default: 1024)
    # @return The amount of data read.
    #
    def read(self, len=1024):
        return self.rfile.read(len)

    ##
    # @return A line of data or None if there is no more input.
    #
    def readline(self):
        return self.rfile.readline()

    ##
    # @param stuff The data to send to the other side.
    # @return The amount of data written.
    #
    def write(self, stuff):
        return self.wfile.write(stuff)

    ##
    # Flush any write buffers.
    #
    def flush(self):
        self.wfile.flush()
        return

    ##
    # Close the connection.
    #
    def close(self):
        self.rfile.close()
        self.wfile.close()
        if self.myChild:
            self.myChild.wait()
            self.myChild = None
            pass
        return

    ##
    # Send an rfc822 style header to the other side.
    #
    # @param key The header key.
    # @param value The value paired with the given key.
    #
    def putheader(self, key, value):
        self.write("%s: %s\r\n" % (key, str(value)))
        return

    ##
    # Terminate the list of headers so the body can follow.
    #
    def endheaders(self):
        self.write("\r\n")
        self.flush()
        return

    def __repr__(self):
        return "<SSHConnection %s%s>" % (self.host, self.handler)

    __str__ = __repr__

    pass

##
# Use SSH to transport XML-RPC requests/responses
#
class SSHTransport:

    ##
    # @param ssh_config The ssh config file to use when making new connections.
    #
    def __init__(self, ssh_config=None):
        self.connections = {}
        self.ssh_config = ssh_config
        return
    
    ##
    # Send a request to the destination.
    #
    # @param host The host name on which to execute the request
    # @param handler The python file that will handle the request.
    # @param request_body The XML-RPC encoded request.
    # @param verbose unused.
    # @return The value returned 
    #
    def request(self, host, handler, request_body, verbose=0, path=None):
        # Strip the leading slash in the handler, if there is one.
        if path:
            handler = path + handler
            pass
        elif handler.startswith('/'):
            handler = handler[1:]
            pass

        # Try to get a new connection,
        if not self.connections.has_key((host,handler)):
            if verbose:
                sys.stderr.write("New connection for %s %s\n" %
                                 (host, handler))
                pass
            
            self.connections[(host,handler)] = SSHConnection(host, handler)
            pass
        connection = self.connections[(host,handler)]

        # ... send our request, and
        connection.putheader("content-length", len(request_body))
        connection.endheaders()
        connection.write(request_body)
        connection.flush()

        # ... parse the response.
        retval = self.parse_response(connection)

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
    def parse_response(self, connection):
        parser, unmarshaller = self.getparser()

        try:
            # Get the headers,
            headers = SSHMessage(connection, False)
            # ... the length of the body, and
            length = int(headers['content-length'])
            # ... read in the body.
            response = connection.read(length)
            pass
        except KeyError, e:
            # Bad header, drop the connection, and
            del self.connections[(connection.host,connection.handler)]
            connection.close()
            # ... tell the user.
            raise BadResponse(connection.host, connection.handler, e.args)
        
        parser.feed(response)

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
        self.ssh_connection = os.environ['SSH_CONNECTION'].split()
        self.myObject = object

        #
        # Init syslog
        #
        syslog.openlog("sshxmlrpc", syslog.LOG_PID, LOG_TESTBED);
        syslog.syslog(syslog.LOG_INFO,
                      "Connect by " + os.environ['USER'] + " from " +
                      self.ssh_connection[0]);
                      
        return

    ##
    # Handle a single request from a client.  The method will read the request
    # from the client, dispatch the method, and write the response back to the
    # client.
    #
    # @param connection An initialized SSHConnection object.
    #
    def handle_request(self, connection):
        retval = False
        try:
            # Read the request,
            hdrs = SSHMessage(connection, False)
            length = int(hdrs['content-length'])
            params, method = xmlrpclib.loads(connection.read(length))
            syslog.syslog(syslog.LOG_INFO, "Calling method '" + method + "'");
            try:
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
            except:
                # Some other exception happened, convert it to an XML-RPC fault
                response = xmlrpclib.dumps(
                    xmlrpclib.Fault(1,
                                    "%s:%s" % (sys.exc_type, sys.exc_value)))
                pass
            else:
                # Everything worked, encode the real response.
                response = xmlrpclib.dumps(response, methodresponse=1)
                pass
            pass
        except xmlrpclib.Fault, faultobj:
            # An XML-RPC related fault occurred, just encode the response.
            response = xmlrpclib.dumps(faultobj)
            retval = True
            pass
        except:
            # Some other exception happened, convert it to an XML-RPC fault.
            response = xmlrpclib.dumps(
                xmlrpclib.Fault(1, "%s:%s" % (sys.exc_type, sys.exc_value)))
            retval = True
            pass

        # Finally, send the reply to the client.
        connection.putheader("content-length", len(response))
        connection.endheaders()
        connection.write(response)
        connection.flush()

        return retval

    ##
    # Handle all of the user requests.
    #
    # @param streams A pair containing the input and output streams.
    #
    def serve_forever(self, streams):
        # Make a new connection from the streams and handle requests until the
        # streams are closed or there is a protocol error.
        connection = SSHConnection(self.ssh_connection[0], '', streams=streams)
        try:
            done = False
            while not done:
                done = self.handle_request(connection)
                pass
            pass
        finally:
            connection.close()
            syslog.syslog(syslog.LOG_INFO, "Connection closed");
            syslog.closelog()
            pass
        return
    
    pass


##
# A client-side proxy for XML-RPC servers that are accessible via SSH.
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
    def __init__(self, uri, transport=None, encoding=None, verbose=0, path=None):
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
        self.__path = path
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
            verbose=self.__verbose,
            path=self.__path
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

    pass
