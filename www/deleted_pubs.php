<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("pub_defs.php");

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid_idx   = $this_user->uid_idx();
$isadmin   = ISADMIN();
$optargs   = OptionalPageArguments("showdeleted", PAGEARG_BOOLEAN);

PAGEHEADER("Deleted Publications");
?>

<p>To undelete a publication simply edit it again and unclick "Mark As Deleted"</p>

<?php

$where_clause = '';
$deleted_clause = '`deleted`';

if ($isadmin) {
  $where_clause = '1';
} else {
  $where_clause = "(`owner` = $uid_idx or `last_edit_by` = $uid_idx)";
}

$query_result = GetPubs($where_clause, $deleted_clause);
echo MakeBibList($this_user, $isadmin, $query_result);

PAGEFOOTER();
?>
