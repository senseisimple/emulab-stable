--- src/lib/tcp_transport.c.orig	Mon Nov  8 16:58:37 2004
+++ src/lib/tcp_transport.c	Mon Nov  8 16:58:50 2004
@@ -621,7 +621,7 @@
 
     /* Look up the host address */
     memset(&hint, 0, sizeof(hint));
-    hint.ai_family = AF_UNSPEC;
+    hint.ai_family = PF_INET;
     hint.ai_socktype = SOCK_STREAM;
     sprintf(port, "%d", context->port);
 
