#!/usr/bin/perl -w
#
######################################################
#-
# Copyright (c) 1999-2000 James E. Housley <jim@thehousleys.net>
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
# OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
# OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.
#
# $Id: NotePage.pl,v 1.1 2001-12-05 18:45:06 kwebb Exp $
#
######################################################

use Socket;

$destination = "www.notepage.com";

$message = $ARGV[1];
$message =~ s/[ \n\r]/+/g;
$url = "FRM=healthd\&USER=$ARGV[0]&MSG=$message";

$length = length($url);
$string = "POST /cgi-bin/webgate.exe HTTP/1.0\nContent-type: application/x-www-form-urlencoded\nContent-length: $length\n\n$url\n\n";

$iaddr = gethostbyname($destination);
$paddr = sockaddr_in("80", $iaddr);
$proto = getprotobyname("tcp");

socket(DestSock, PF_INET, SOCK_STREAM, $proto);
connect(DestSock, $paddr);
send (DestSock, $string, 0);
close DestSock;
