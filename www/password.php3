<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2002 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

#
# Standard Testbed Header
#
PAGEHEADER("Forgot Your Password?");

echo "<p>
      So you've forgotten your password. Happens to all of us!

      <p>
      Unfortunately, we don't do those little reminder hints like \"What was
      your first dog's maiden name?.\"

      <p>
      If you are a <b>project member</b> then please send email to your
      project or group leader. She/He can change your password for you via
      the 'Update User Information' menu item.

      <p>
      If you are a <b>project leader</b>, then send us an
      <a href='mailto:$TBMAILADDR_OPS?subject=I forgot my password'>email
      message</a>, and we will fix things up for you!\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>






