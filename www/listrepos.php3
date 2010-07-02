<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2005, 2006, 2007 University of Utah and the Flux Group.
# All rights reserved.
#
include("defs.php3");

if (!$CVSSUPPORT) {
    header("Location: index.php3");
    return;
}

#
# Standard Testbed Header is below.
#

#
# Only known and logged in users.
#
$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

#
# Verify page arguments.
#
$reqargs = RequiredPageArguments("target_user", PAGEARG_USER);

# Need these below.
$userstatus = $target_user->status();
$target_uid = $target_user->uid();

#
# Standard Testbed Header, now that we know what we want to say.
#
if ($uid != $target_uid) {
    PAGEHEADER("CVS Repositories for: $target_uid");
}
else {
    PAGEHEADER("My CVS Repositories");
}

#
# Verify Permission.
#
if (!$isadmin && 
    !$target_user->AccessCheck($this_user, $TB_USERINFO_READINFO)) {
    USERERROR("You do not have permission to view ${uid}'s information!", 1);
}

#
# See what projects the uid is a member of, and print some repo pointers
#
$projlist = $target_user->ProjectAccessList($TB_PROJECT_READINFO);

if (! count($projlist)) {
    USERERROR("$target_uid is not a member of any Projects!", 1);
}

echo "<br>
      <center><font size=+1>
      CVS repositories for the projects you are a member of</font><br>
      (Click on the CvsWeb link to visit the repository)
      </font></center><br>\n";

echo "<table align=center cellpadding=2 border=1>\n";
echo "<tr>
        <th>Project</th>
        <th>Repository</th>
        <th>CvsWeb Link</th>
      </tr>\n";

while (list($pid) = each($projlist)) {
    $cvsdir = "$TBCVSREPO_DIR/$pid";
    $cvsurl = "cvsweb/cvswebwrap.php3?pid=$pid";
	
    echo "<tr>
              <td><A href='showproject.php3?pid=$pid'>$pid</A></td>
              <td>$cvsdir</td>
              <td align=center><A href='$cvsurl'>
                     <img src=\"arrow4.ico\"
                          border=0 alt='cvsrepo'></A></td>
          </tr>\n";
}
echo "</table>\n";

if ($target_user->cvsweb()) {
    echo "<br><center>
          You also have CVSweb access to the
          <a href=cvsweb/cvswebwrap.php3>Emulab Source Repository</a>.
          </center><br>\n";
          
}

echo "<blockquote><blockquote>
      Emulab gives each project its own CVS repository in $TBCVSREPO_DIR.
      The repository is available both via FreeBSD's
      <a hrep=http://www.freebsd.org/projects/cvsweb.html>CVSweb</a>
      interface, and via
      <a href=http://www.freebsd.org/cgi/man.cgi?query=cvs&sektion=1>CVS</a>
      over
      <a href=http://www.freebsd.org/cgi/man.cgi?query=ssh&sektion=1>SSH</a>
      to <tt>$CVSNODE</tt>.<br>
      <br>
      Project CVS repositories are initially <b>private</b>; only members
      of the project may access the repository. Repositories may also be
      set to <b>public</b> via a toggle on the project home page. When a
      repository is set to public, anyone on the Internet may access the
      repository (read-only), either with the CVSWeb interface, or with
      anonymous CVS using CVS's pserver, both hosted on <tt>$CVSNODE</tt>.
      <br><br>
      When a project repository <b>is</b> public, the CVSweb address is:
      <blockquote>
      <tt>http://$CVSNODE/cvsweb/cvsweb.cgi/?cvsroot=yourprojectname</tt>
      </blockquote>
      or you can use CVS pserver with:
      <blockquote>
      <tt>cvs -R -d :pserver:anoncvs@$CVSNODE:/cvsrepos/yourprojectname ...
        </tt>
      </blockquote>
      just hit carriage return for the password.
      </blockquote></blockquote>\n";

#
# Standard Testbed Footer
# 
PAGEFOOTER();
?>
