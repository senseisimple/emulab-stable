<?php
#
# Database Constants
#
$TBDBNAME       = "tbdb";

$TBDB_UIDLEN    = 8;
$TBDB_PIDLEN    = 12;

#
# Current policy is to prefix the EID with the PID. Make sure it is not
# too long for the database. PID is 12, and the max is 32, so the user
# cannot have provided an EID more than 19, since other parts of the system
# may concatenate them together with a hyphen.
#
$TBDB_EIDLEN    = 19;


?>
