<?php
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
      But if you send us an
      <a href='mailto:$TBMAILADDR_OPS?subject=I forgot my password'>email
      message</a>, we will fix things up for you!\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>






