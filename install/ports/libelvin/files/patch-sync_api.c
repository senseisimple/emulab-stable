*** src/lib/sync_api.c.orig	Sun May 13 01:16:04 2001
--- src/lib/sync_api.c	Fri Mar  7 15:29:19 2003
***************
*** 729,735 ****
              timeout_ptr = NULL;
          }
      } else {
!         timeout.tv_usec = 0;
          timeout.tv_usec = 0;
          timeout_ptr = &timeout;
      }
--- 729,735 ----
              timeout_ptr = NULL;
          }
      } else {
!         timeout.tv_sec = 0;
          timeout.tv_usec = 0;
          timeout_ptr = &timeout;
      }
