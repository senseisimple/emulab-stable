<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#

#
# A cache of users to avoid lookups. Indexed by uid_idx.
#
$user_cache = array();

class User
{
    var	$user;

    #
    # For pedantic checks early in pages.
    #
    function ValidWebID($token) {
	if (! preg_match("/^[-\w]+$/", $token)) {
	    return 0;
	}
	return 1;
    }

    #
    # Constructor by lookup on unique index.
    #
    function &User($uid_idx) {
	$safe_uid_idx = addslashes($uid_idx);

	$query_result =
	    DBQueryWarn("select * from users where uid_idx='$safe_uid_idx'");

	if (!$query_result || !mysql_num_rows($query_result)) {
	    $this->user = NULL;
	    return;
	}
	$this->user =& mysql_fetch_array($query_result);
    }

    # Hmm, how does one cause an error in a php constructor?
    function IsValid() {
	return !is_null($this->user);
    }

    # Lookup by uid_idx.
    function &Lookup($uid_idx) {
	global $user_cache;

        # Look in cache first
	if (array_key_exists("$uid_idx", $user_cache))
	    return $user_cache["$uid_idx"];

	$foo =& new User($uid_idx);

	if (! $foo->IsValid()) {
	    # Try lookup by plain uid.
	    $foo =& User::LookupByUid($uid_idx);
	    
	    if (! $foo->IsValid())
		return null;
	    
	    # Already in the cache from LookupByUid() so just return it.
	    return $foo;
	}
	# Insert into cache.
	$user_cache["$uid_idx"] =& $foo;
	return $foo;
    }

    # Backwards compatable lookup by uid. Will eventually flush this.
    function &LookupByUid($uid) {
	$safe_uid = addslashes($uid);

	$query_result =
	    DBQueryWarn("select uid_idx from users where uid='$safe_uid'");

	if (!$query_result || !mysql_num_rows($query_result)) {
	    return null;
	}
	$row = mysql_fetch_array($query_result);
	$idx = $row['uid_idx'];

	return User::Lookup($idx);
    }

    # Used in the change password code and to make sure that emails are
    # locally unique.
    function &LookupByEmail($email) {
	$safe_email = addslashes($email);

	$query_result =
	    DBQueryWarn("select uid_idx from users ".
			"where LCASE(usr_email)=LCASE('$safe_email')");

	if (!$query_result || !mysql_num_rows($query_result)) {
	    return null;
	}
	$row = mysql_fetch_array($query_result);
	$idx = $row['uid_idx'];

	return User::Lookup($idx);
    }
    
    # Used in new/join project code to make sure that wikinames are
    # locally unique.
    function &LookupByWikiName($wikiname) {
	$safe_wikiname = addslashes($wikiname);

	$query_result =
	    DBQueryWarn("select uid_idx from users ".
			"where wikiname='$safe_wikiname')");

	if (!$query_result || !mysql_num_rows($query_result)) {
	    return null;
	}
	$row = mysql_fetch_array($query_result);
	$idx = $row['uid_idx'];

	return User::Lookup($idx);
    }
    
    #
    # Refresh an instance by reloading from the DB.
    #
    function Refresh() {
	if (! $this->IsValid())
	    return -1;

	$uid_idx = $this->uid_idx();

	$query_result =
	    DBQueryWarn("select * from users where uid_idx='$uid_idx'");
    
	if (!$query_result || !mysql_num_rows($query_result)) {
	    $this->user = NULL;
	    return -1;
	}
	$this->user =& mysql_fetch_array($query_result);
	return 0;
    }

    #
    # Equality test.
    #
    function SameUser($user) {
	return $user->uid_idx() == $this->uid_idx();
    }

    #
    # At some point we will stop passing uid and start using uid_idx.
    # Use this function to avoid having to change a bunch of code twice.
    #
    function URLParam() {
	return $this->uid();
    }

    # accessors
    function field($name) {
	return (is_null($this->user) ? -1 : $this->user[$name]);
    }
    function uid_idx()		{ return $this->field("uid_idx"); }
    function uid()		{ return $this->field("uid"); }
    function webid()		{ return $this->field("uid_idx"); }
    function created()		{ return $this->field("usr_created"); }
    function expires()		{ return $this->field("usr_expires"); }
    function modified()		{ return $this->field("usr_modified"); }
    function name()		{ return $this->field("usr_name"); }
    function title()		{ return $this->field("usr_title"); }
    function affil()		{ return $this->field("usr_affil"); }
    function email()		{ return $this->field("usr_email"); }
    function URL()		{ return $this->field("usr_URL"); }
    function addr()		{ return $this->field("usr_addr"); }
    function addr2()		{ return $this->field("usr_addr2"); }
    function city()		{ return $this->field("usr_city"); }
    function state()		{ return $this->field("usr_state"); }
    function zip()		{ return $this->field("usr_zip"); }
    function country()		{ return $this->field("usr_country"); }
    function phone()		{ return $this->field("usr_phone"); }
    function shell()		{ return $this->field("usr_shell"); }
    function pswd()		{ return $this->field("usr_pswd"); }
    function w_pswd()		{ return $this->field("usr_w_pswd"); }
    function unix_uid()		{ return $this->field("unix_uid"); }
    function status()		{ return $this->field("status"); }
    function admin()		{ return $this->field("admin"); }
    function foreign_admin()	{ return $this->field("foreign_admin"); }
    function dbedit()		{ return $this->field("dbedit"); }
    function stud()		{ return $this->field("stud"); }
    function webonly()		{ return $this->field("webonly"); }
    function pswd_expires()	{ return $this->field("pswd_expires"); }
    function cvsweb()		{ return $this->field("cvsweb"); }
    function emulab_pubkey()	{ return $this->field("emulab_pubkey"); }
    function home_pubkey()	{ return $this->field("home_pubkey"); }
    function adminoff()		{ return $this->field("adminoff"); }
    function verify_key()	{ return $this->field("verify_key"); }
    function widearearoot()	{ return $this->field("widearearoot"); }
    function wideareajailroot() { return $this->field("wideareajailroot"); }
    function notes()		{ return $this->field("notes"); }
    function weblogin_frozen()  { return $this->field("weblogin_frozen"); }
    function weblogin_failcount(){return $this->field("weblogin_failcount");}
    function weblogin_failstamp(){return $this->field("weblogin_failstamp");}
    function plab_user()	{ return $this->field("plab_user"); }
    function user_interface()	{ return $this->field("user_interface"); }
    function chpasswd_key()	{ return $this->field("chpasswd_key"); }
    function chpasswd_expires() { return $this->field("chpasswd_expires"); }
    function wikiname()		{ return $this->field("wikiname"); }
    function wikionly()		{ return $this->field("wikionly"); }
    function mailman_password() { return $this->field("mailman_password"); }

    #
    # Class function to create new user and return object.
    #
    function NewUser($uid, $flags, $args) {
	global $TBBASE, $TBMAIL_APPROVAL, $TBMAIL_AUDIT, $TBMAIL_WWW;
	global $MIN_UNIX_UID;

	$isleader = ($flags & TBDB_NEWACCOUNT_PROJLEADER ? 1 : 0);
	$wikionly = ($flags & TBDB_NEWACCOUNT_WIKIONLY   ? 1 : 0);
	$webonly  = ($flags & TBDB_NEWACCOUNT_WEBONLY    ? 1 : 0);

	#
	# If no uid, we need to generate a unique one for the user.
	#
	if (! $uid) {
	    #
	    # Take the first 5 letters of the email to form a root. That gives
	    # us 3 digits to make it unique, since unix uids are limited to 8
	    # chars, sheesh!
	    #
	    $email = $args["usr_email"];

	    if (! preg_match('/^([-\w\+\.]+)\@([-\w\.]+)$/', $email, $matches))
		return null;

	    $token = $matches[1];

            # Squeeze out any dots or dashes.
	    $token = preg_replace('/\./', '', $token);
	    $token = preg_replace('/\-/', '', $token);

            # Trim off any trailing numbers or +foo tokens.
	    if (! preg_match('/^([a-zA-Z]+)/', $token, $matches)) {
		return null;
	    }
	    
	    $token = $matches[1];

	    # First 5 chars, at most.
	    $token = substr($token, 0, 5);

	    # Grab all root matches from the DB.
	    $query_result =
		DBQueryFatal("select uid from users ".
			     "where uid like '${token}%'");

	    if (!$query_result)
		return null;

	    # Easy; no matches at all!
	    if (!mysql_num_rows($query_result)) {
		$uid = "$token" . "001";
	    }
	    else {
		$max = 0;
		
		#
		# Find unused slot. Must be a better way to do this!
		#
		while ($row = mysql_fetch_array($query_result)) {
		    $foo = $row[0];

                    # Split name from number
		    if (! preg_match('/^([a-zA-Z]+)(\d*)$/', $foo, $matches)) {
			return null;
		    }
		    $name   = $matches[1];
		    $number = $matches[2];

		    # Must be exact root
		    if ($name != $token)
			continue;

		    # Backwards compatability; might not have appended number.
		    if (isset($number) && intval($number) > $max)
			$max = intval($number);
		}
		$max++;
		$uid = $token . sprintf("%03d", $max);
	    }
	}
	
	#
	# The array of inserts is assumed to be safe already. Generate
	# a list of actual insert clauses to be joined below.
	#
	$insert_data = array();
	
	foreach ($args as $name => $value) {
	    $insert_data[] = "$name='$value'";
	}

	# Every user gets a new unique index.
	$uid_idx = TBGetUniqueIndex('next_uid');

	# Get me an unused unix id. Nice query, eh? Basically, find 
	# unused numbers by looking at existing numbers plus one, and check
	# to see if that number is taken. 
	$query_result =
	    DBQueryFatal("select u.unix_uid + 1 as start from users as u ".
			 "left outer join users as r on ".
			 "  u.unix_uid + 1 = r.unix_uid ".
			 "where u.unix_uid>$MIN_UNIX_UID and ".
			 "      u.unix_uid<60000 and ".
			 "      r.unix_uid is null limit 1");

	if (!$query_result || !mysql_num_rows($query_result)) {
	    TBERROR("Could not find an unused unix_uid!", 1);
	}
	$row = mysql_fetch_row($query_result);
	$unix_uid = $row[0];

        # Initial mailman_password.
	$mailman_password = substr(GENHASH(), 0, 10);

	# And a verification key.
	$verify_key = md5(uniqid(rand(),1));

	# Now tack on other stuff we need.
	if ($wikionly)
	    $insert_data[] = "wikionly='1'";
	if ($webonly)
	    $insert_data[] = "webonly='1'";
	
	$insert_data[] = "usr_created=now()";
	$insert_data[] = "usr_modified=now()";
	$insert_data[] = "pswd_expires=date_add(now(), interval 1 year)";
	$insert_data[] = "unix_uid=$unix_uid";
	$insert_data[] = "status='newuser'";
	$insert_data[] = "mailman_password='$mailman_password'";
	$insert_data[] = "verify_key='$verify_key'";
	$insert_data[] = "uid_idx='$uid_idx'";
	$insert_data[] = "uid='$uid'";

	# Insert into DB. Should probably lock the table ...
	if (!DBQueryWarn("insert into users set ".
			 implode(",", $insert_data))) {
	    return null;
	}

	if (! DBQueryWarn("insert into user_stats (uid, uid_idx) ".
			  "VALUES ('$uid', $uid_idx)")) {
	    DBQueryFatal("delete from users where uid_idx='$uid_idx'");
	    return null;
	}
	$newuser = User::Lookup($uid_idx);
	if (! $newuser)
	    return null;

        #
        # See if we are in an initial Emulab setup.
        #
        $FirstInitState = (TBGetFirstInitState() == "createproject");
	if ($FirstInitState)
	    return $newuser;

        # stuff for email message.
	$key       = $newuser->verify_key();
	$usr_name  = $newuser->name();
	$usr_email = $newuser->email();
	    
	# Email to user.
	TBMAIL("$usr_name '$uid' <$usr_email>",
       "Your New User Key",
       "\n".
       "Dear $usr_name ($uid):\n\n".
       "This is your account verification key: $key\n\n".
       "Please use this link to verify your user account:\n".
       "\n".
       "    ${TBBASE}/login.php3?vuid=$uid&key=$key\n".
       "\n".
       ($wikionly ?
	"Once you have verified your account, you will be able to access\n".
	"the Wiki. You MUST verify your account first!"
	:
	($webonly ?
	 "Once you have verified your account, Testbed Operations will be\n".
	 "able to approve you. You MUST verify your account first!"
	 :
	($isleader ?
	 "You will then be verified as a user. When you have been both\n".
	 "verified and approved by Testbed Operations, you will be marked\n".
	 "as an active user and granted full access to your account.\n".
	 "You MUST verify your account before your project can be approved!\n"
	 :
	 "Once you have verified your account, the project leader will be\n".
	 "able to approve you.\n\n".
	 "You MUST verify your account before the project leader can ".
	 "approve you\n".
	 "After project approval, you will be marked as an active user, and\n".
	 "will be granted full access to your user account."))) .
       "\n\n".
       "Thanks,\n".
       "Testbed Operations\n",
       "From: $TBMAIL_APPROVAL\n".
       "Bcc: $TBMAIL_AUDIT\n".
       "Errors-To: $TBMAIL_WWW");
	
	return $newuser;
    }

    #
    # Delete a user, but JUST from the users table. 
    #
    function Delete() {
	global $user_cache;

	$uid_idx = $this->uid_idx();

	DBQueryFatal("delete from users where uid_idx='$uid_idx'");

	if (array_key_exists("$uid_idx", $user_cache))
	    unset($user_cache["$uid_idx"]);
	
	return 0;
    }

    #
    # Stats
    #
    function ShowStats() {
	$uid_idx = $this->uid_idx();
	
	$query_result =
	    DBQueryFatal("select s.* from user_stats as s ".
			 "where s.uid_idx='$uid_idx'");

	if (! mysql_num_rows($query_result)) {
	    return "";
	}
	$row = mysql_fetch_assoc($query_result);

        #
        # Not pretty printed yet.
        #
	$html = "<table align=center border=1>\n";
    
	foreach($row as $key => $value) {
	    $html .= "<tr><td>$key:</td><td>$value</td></tr>\n";
	}
	$html .= "</table>\n";
	return $html;
    }

    function Show() {
	global $WIKISUPPORT;

	$user = $this;

	$uid         = $user->uid();
	$webid       = $user->webid();
	$uid_idx     = $user->uid_idx();
	$usr_email   = $user->email();
	$usr_URL     = $user->URL();
	$usr_addr    = $user->addr();
	$usr_addr2   = $user->addr2();
	$usr_city    = $user->city();
	$usr_state   = $user->state();
	$usr_zip     = $user->zip();
	$usr_country = $user->country();
	$usr_name    = $user->name();
	$usr_phone   = $user->phone();
	$usr_shell   = $user->shell();
	$usr_title   = $user->title();
	$usr_affil   = $user->affil();
	$status      = $user->status();
	$admin       = $user->admin();
	$notes       = $user->notes();
	$frozen      = $user->weblogin_frozen();
	$failcount   = $user->weblogin_failcount();
	$failstamp   = $user->weblogin_failstamp();
	$wikiname    = $user->wikiname();
	$cvsweb      = $user->cvsweb();
	$wikionly    = $user->wikionly();

	if (!strcmp($usr_addr2, ""))
	    $usr_addr2 = "&nbsp;";
	if (!strcmp($usr_city, ""))
	    $usr_city = "&nbsp;";
	if (!strcmp($usr_state, ""))
	    $usr_state = "&nbsp;";
	if (!strcmp($usr_zip, ""))
	    $usr_zip = "&nbsp;";
	if (!strcmp($usr_country, ""))
	    $usr_country = "&nbsp;";
	if (!strcmp($notes, ""))
	    $notes = "&nbsp;";

        #
        # Last Login info.
        #
	if (($lastweblogin = LASTWEBLOGIN($uid)) == 0)
	    $lastweblogin = "&nbsp;";
	if (($lastuserslogininfo = TBUsersLastLogin($uid)) == 0)
	    $lastuserslogin = "N/A";
	else {
	    $lastuserslogin = $lastuserslogininfo["date"] . " " .
		$lastuserslogininfo["time"];
	}
    
	if (($lastnodelogininfo = TBUidNodeLastLogin($uid)) == 0)
	    $lastnodelogin = "N/A";
	else {
	    $lastnodelogin = $lastnodelogininfo["date"] . " " .
		$lastnodelogininfo["time"] . " " .
		"(" . $lastnodelogininfo["node_id"] . ")";
	}
    
	echo "<table align=center border=1>\n";
    
	echo "<tr>
                  <td>Username:</td>
                  <td>$uid ($uid_idx)</td>
              </tr>\n";
    
        echo "<tr>
                  <td>Full Name:</td>
                  <td>$usr_name</td>
              </tr>\n";
    
        echo "<tr>
                  <td>Email Address:</td>
                  <td>$usr_email</td>
              </tr>\n";

        echo "<tr>
                  <td>Home Page URL:</td>
                  <td><a href='$usr_URL'>$usr_URL</a></td>
              </tr>\n";

	if ($WIKISUPPORT && isset($wikiname)) {
	    $wikiurl = "gotowiki.php3?redurl=Main/$wikiname";
	
	    echo "<tr>
                      <td>Emulab Wiki Page:</td>
                      <td class=\"left\">
                          <a href='$wikiurl'>$wikiname</a></td>
                  </tr>\n";
	}
    
	echo "<tr>
                  <td>Address 1:</td>
                  <td>$usr_addr</td>
             </tr>\n";
    
        echo "<tr>
                  <td>Address 2:</td>
                  <td>$usr_addr2</td>
              </tr>\n";
    
        echo "<tr>
                  <td>City:</td>
                  <td>$usr_city</td>
              </tr>\n";
    
	echo "<tr>
                  <td>State:</td>
                  <td>$usr_state</td>
              </tr>\n";
    
        echo "<tr>
                  <td>ZIP:</td>
                  <td>$usr_zip</td>
              </tr>\n";

        echo "<tr>
                  <td>Country:</td>
                  <td>$usr_country</td>
              </tr>\n";
    
        echo "<tr>
                  <td>Phone #:</td>
                  <td>$usr_phone</td>
              </tr>\n";

	echo "<tr>
	          <td>Shell:</td>
	          <td>$usr_shell</td>
              </tr>\n";
    
        echo "<tr>
                  <td>Title/Position:</td>
                  <td>$usr_title</td>
             </tr>\n";
    
	echo "<tr>
                  <td>Institutional Affiliation:</td>
                  <td>$usr_affil</td>
              </tr>\n";
    
        echo "<tr>
                  <td>Status:</td>
                  <td>$status</td>
              </tr>\n";

	if ($wikionly) {
	    echo "<tr>
                      <td><b>Wikionly</b>:</td>
                      <td>Yes</td>
                  </tr>\n";
	}

	if ($admin) {
	    echo "<tr>
                      <td>Administrator:</td>
                      <td>Yes</td>
                  </tr>\n";
	}
    
	echo "<tr>
                  <td>Last Web Login:</td>
                  <td>$lastweblogin</td>
              </tr>\n";
    
	echo "<tr>
                  <td>Last Users Login:</td>
                  <td>$lastuserslogin</td>
              </tr>\n";
    
        echo "<tr>
                  <td>Last Node Login:</td>
                  <td>$lastnodelogin</td>
              </tr>\n";

	if (ISADMIN()) {
	    $cvswebflip = ($cvsweb ? 0 : 1);

	    $toggle_url = CreateURL("toggle", $user,
				    "type", "cvsweb", "value", $cvswebflip);

	    echo "<tr>
                      <td>CVSWeb Access:</td>
                      <td>$cvsweb (<a href='$toggle_url'>Toggle</a>)
                  </tr>\n";
	
	    $freezeflip = ($frozen ? 0 : 1);
	    $toggle_url = CreateURL("toggle", $user,
				    "type", "webfreeze", "value", $freezeflip);

	    echo "<tr>
                      <td>Web Freeze:</td>
                      <td>$frozen (<a href='$toggle_url'>Toggle</a>)
                  </tr>\n";
	
	    if ($frozen && $failstamp && $failcount) {
		$when = strftime("20%y-%m-%d %H:%M:%S", $failstamp);
	    
		echo "<tr>
                          <td>Login Failures:</td>
                          <td>$failcount ($when)</td>
                      </tr>\n";
	    }
	    echo "<tr>
                      <td>Notes:</td>
                      <td>$notes</td>
                  </tr>\n";
	}
	echo "</table>\n";
    }

    #
    # Access Check, determines if $user can access $this record.
    #
    #	returns 0 if not allowed.
    #   returns 1 if allowed.
    # 
    function AccessCheck($user, $access_type) {
	global $TB_USERINFO_READINFO;
	global $TB_USERINFO_MODIFYINFO;
	global $TB_USERINFO_MIN;
	global $TB_USERINFO_MAX;

	$this_idx = $this->uid_idx();
	$auth_idx = $user->uid_idx();

	if ($access_type < $TB_USERINFO_MIN ||
	    $access_type > $TB_USERINFO_MAX) {
	    TBERROR("UserAccessCheck: Invalid access type $access_type!", 1);
	}

	if ($this->uid_idx() == $user->uid_idx()) {
	    return 1;
	}

        #
        # Admins do whatever they want.
        # 
        if (ISADMIN()) {
	    return 1;
	}

        #
        # This join will allow the operation if the current user is in the 
        # same group (any group) as the target user, but with root permissions.
        # 
	$query_result =
	    DBQueryFatal("select g.trust from group_membership as g ".
			 "left join group_membership as authed on ".
			 "     g.pid_idx=authed.pid_idx and ".
			 "     g.gid_idx=authed.gid_idx and ".
			 "     g.uid_idx='$uid_idx' ".
			 "where authed.uid_idx='$auth_idx' and ".
			 "      (authed.trust='group_root' or ".
			 "       authed.trust='project_root')");

	if (mysql_num_rows($query_result) == 0) {
	    return 0;
	}
	return 1;
    }

    #
    # How many PCs is user using. Again, use global function for now.
    #
    function PCsInUse() {
	return TBUserPCs($this->uid());
    }

    #
    # Functions to change various DB values.
    #
    function SetStatus($status) {
	$idx = $this->uid_idx();

	DBQueryFatal("update users set status='$status' ".
		     "where uid_idx='$idx'");
	$this->user["status"] = $status;
	return 0;
    }
    function SetEmail($email) {
	$idx = $this->uid_idx();

	DBQueryFatal("update users set usr_email='$email' ".
		     "where uid_idx='$idx'");
	$this->user["usr_email"] = $email;
	return 0;
    }
    function SetPassword($encoding, $expires) {
	$idx = $this->uid_idx();

	# Clear the chpasswd stuff anytime passwd is set.
	DBQueryFatal("update users set ".
		     "  usr_pswd='$encoding', pswd_expires=$expires, ".
		     "  chpasswd_key=NULL,chpasswd_expires=0 ".
		     "where uid_idx='$idx'");
	$this->user["usr_pswd"] = $encoding;
	return 0;
    }
    function SetChangePassword($key, $expires) {
	$idx = $this->uid_idx();

	DBQueryFatal("update users set ".
		     "  chpasswd_key='$key',chpasswd_expires=$expires ".
		     "where uid_idx='$idx'");
	return 0;
    }
    function SetWindowsPassword($password) {
	$idx = $this->uid_idx();

	DBQueryFatal("update users set ".
		     "  usr_w_pswd='$password' ".
		     "where uid_idx='$idx'");
	$this->user["usr_w_pswd"] = $password;
	return 0;
    }
    function SetNotes($notes) {
	$idx   = $this->uid_idx();
	$notes = addslashes($notes);
			    
	DBQueryFatal("update users set ".
		     "  notes='$notes' ".
		     "where uid_idx='$idx'");
	$this->user["notes"] = $notes;
	return 0;
    }
    function SetUserInterface($interface) {
	$idx   = $this->uid_idx();
			    
	DBQueryFatal("update users set ".
		     "  user_interface='$interface' ".
		     "where uid_idx='$idx'");
	$this->user["user_interface"] = $interface;
	return 0;
    }
    function SetWebFreeze($freeze) {
	$idx   = $this->uid_idx();

	$freeze = ($freeze ? 1 : 0);
			    
	DBQueryFatal("update users set ".
		     "   weblogin_frozen='$freeze' ".
		     "where uid_idx='$idx'");
	$this->user["weblogin_frozen"] = $freeze;
	return 0;
    }
    function SetCVSWeb($onoff) {
	$idx   = $this->uid_idx();

	$onoff = ($onoff ? 1 : 0);
			    
	DBQueryFatal("update users set ".
		     "   cvsweb='$onoff' ".
		     "where uid_idx='$idx'");
	$this->user["cvsweb"] = $onoff;
	return 0;
    }
    function UpdateWebLoginFail() {
	$idx   = $this->uid_idx();

	DBQueryFatal("update users set ".
		     "       weblogin_failcount=weblogin_failcount+1, ".
		     "       weblogin_failstamp='$now' ".
		     "where uid_idx='$idx'");

	return $this->Refresh();
    }
    function ChangeProfile($usr_name,  $usr_title,
			   $usr_affil, $usr_addr,
			   $usr_addr2, $usr_city,
			   $usr_state, $usr_zip, $usr_country,
			   $usr_phone, $usr_shell, $usr_URL) {

	$idx          = $this->uid_idx();
	$usr_name     = addslashes($usr_name);
	$usr_title    = addslashes($usr_title);
	$usr_affil    = addslashes($usr_affil);
	$usr_addr     = addslashes($usr_addr);
	$usr_addr2    = addslashes($usr_addr2);
	$usr_city     = addslashes($usr_city);
	$usr_state    = addslashes($usr_state);
	$usr_zip      = addslashes($usr_zip);
	$usr_country  = addslashes($usr_country);
	$usr_phone    = addslashes($usr_phone);
	$usr_shell    = addslashes($usr_shell);
	$usr_URL      = addslashes($usr_URL);

	$query_result =
	    DBQueryFatal("UPDATE users SET ".
			 "usr_name=\"$usr_name\",       ".
			 "usr_URL=\"$usr_URL\",         ".
			 "usr_addr=\"$usr_addr\",       ".
			 "usr_addr2=\"$usr_addr2\",     ".
			 "usr_city=\"$usr_city\",       ".
			 "usr_state=\"$usr_state\",     ".
			 "usr_zip=\"$usr_zip\",         ".
			 "usr_country=\"$usr_country\", ".
			 "usr_phone=\"$usr_phone\",     ".
			 "usr_title=\"$usr_title\",     ".
			 "usr_affil=\"$usr_affil\",     ".
			 "usr_shell=\"$usr_shell\"      ".
			 "WHERE uid_idx=\"$idx\"");

	if (mysql_affected_rows()) {
	    DBQueryFatal("update users set usr_modified=now() ".
			 "where uid_idx='$idx'");
	    
	    return 1;
	}
	return 0;
    }

    #
    # Return project access list for a user. This returns just pid,eid for
    # now, later return actual objects. 
    #
    function ProjectAccessList($access_type) {
	global $TB_PROJECT_CREATEEXPT;
	global $TB_PROJECT_MAKEOSID;
	global $TB_PROJECT_MAKEIMAGEID;
	global $TB_PROJECT_MAKEGROUP;
	global $TB_PROJECT_READINFO;

	$uid_idx     = $this->uid_idx();
	$result      = array();
	$user_clause = "where uid_idx='$uid_idx' and";
	$trust_clause= "";

	# Constants.
	$trust_none   = TBDB_TRUSTSTRING_NONE;
	$trust_user   = TBDB_TRUSTSTRING_USER;
	$trust_local  = TBDB_TRUSTSTRING_LOCALROOT;
	$trust_group  = TBDB_TRUSTSTRING_GROUPROOT;
	$trust_project= TBDB_TRUSTSTRING_PROJROOT;
	
	if ($access_type == $TB_PROJECT_READINFO) {
	    $trust_clause = "trust!='$trust_none'";
	}
	elseif ($access_type == $TB_PROJECT_MAKEGROUP) {
	    $trust_clause = "trust='$trust_project'";
	}
	elseif ($access_type == $TB_PROJECT_CREATEEXPT) {
	    $trust_clause =
		"(trust='$trust_project' or trust='$trust_group' or ".
		" trust='$trust_local')";
	}
	elseif ($access_type == $TB_PROJECT_MAKEOSID ||
		$access_type == $TB_PROJECT_MAKEIMAGEID) {
	    if (ISADMIN()) {
		$user_clause = "";
	    }
	    else {
		$trust_clause =
		    "(trust='$trust_project' or trust='$trust_group' or ".
		    " trust='$trust_local')";
	    
	    }
	}
	else {
	    TBERROR("Invalid access type $access_type!", 1);
	}
    
	$query_result =
	    DBQueryFatal("SELECT distinct pid,gid FROM group_membership ".
			 "$user_clause $trust_clause order by pid");

	if (mysql_num_rows($query_result) == 0) {
	    return $result;
	}

	while ($row = mysql_fetch_array($query_result)) {
	    $pid = $row['pid'];
	    $gid = $row['gid'];
	
	    $result[$pid][] = $gid;
	}
	return $result;
    }

    #
    # Return project membership list for a user.
    #
    function ProjectMembershipList() {
	$uid_idx = $this->uid_idx();
	$result  = array();

	$query_result =
	    DBQueryFatal("select pid_idx from group_membership ".
			 "where pid_idx=gid_idx and ".
			 "      uid_idx='$uid_idx'");

	while ($row = mysql_fetch_array($query_result)) {
	    $pid_idx = $row["pid_idx"];

	    if (! ($project = Project::Lookup($pid_idx))) {
		TBERROR("User::ProjectMembershipList: ".
			"Could not load project $pid_idx!", 1);
	    }
	    $result[] = $project;
	}
	return $result;
    }

    #
    # First approved project.
    #
    function FirstApprovedProject() {
	$uid_idx = $this->uid_idx();

	$query_result =
	    DBQueryFatal("select pid_idx from group_membership ".
			 "where uid_idx='$uid_idx' and pid=gid and ".
			 "      trust!='". TBDB_TRUSTSTRING_NONE . "' ".
			 "order by date_approved asc limit 1");
	
	if (mysql_num_rows($query_result) == 0) {
	    return null;
	}
	$row = mysql_fetch_array($query_result);
	$pid_idx = $row["pid_idx"];

	if (! ($project = Project::Lookup($pid_idx))) {
	    TBERROR("User::FirstApprovedProject: ".
		    "Could not load project $pid_idx!", 1);
	}
	return $project;
    }

    #
    # Are there users waiting to be approved by this user?
    #
    function ApprovalList($listify = 1) {
	$uid_idx = $this->uid_idx();

	#
        # Find all of the groups that this person has project/group root in,
	# and then in all of those groups, all of the people who are awaiting
	# to be approved (status = none).
	#
        # Okay, so this operation sucks out the right people by joining the
        # group_membership table with itself.
	#
	$query_result =
	    DBQueryFatal("select g.uid_idx,g.gid_idx ".
			 "   from group_membership as authed ".
			 "left join group_membership as g on ".
			 "    g.pid_idx=authed.pid_idx and ".
			 "    g.gid_idx=authed.gid_idx ".
			 "left join users as u on u.uid_idx=g.uid_idx ".
			 "where u.status!='". TBDB_USERSTATUS_UNVERIFIED . "'".
			 "  and u.status!='". TBDB_USERSTATUS_NEWUSER . "'".
			 "  and g.uid_idx!='$uid_idx' and ".
			 "      g.trust='". TBDB_TRUSTSTRING_NONE . "' ".
			 "  and authed.uid_idx='$uid_idx' and ".
			 "     (authed.trust='group_root' or ".
			 "      authed.trust='project_root') ".
			 "ORDER BY g.uid,g.pid,g.gid");

	if (! $listify) {
	    return mysql_num_rows($query_result);
	}
	
	# Else, create a list of the groups.
	$result  = array();

	while ($row = mysql_fetch_array($query_result)) {
	    $uid_idx = $row["uid_idx"];
	    $gid_idx = $row["gid_idx"];

	    if (! ($group = Group::Lookup($gid_idx))) {
		TBERROR("User::ApprovalList: ".
			"Could not load group $gid_idx!", 1);
	    }
	    if (! ($user = User::Lookup($uid_idx))) {
		TBERROR("User::ApprovalList: ".
			"Could not load user $uid_idx!", 1);
	    }
	    if (! array_key_exists("$uid_idx", $result)) {
		$result["$uid_idx"] = array();
	    }
	    $result["$uid_idx"][] =& $group;
	}
	return $result;
    }

    #
    # See if user has enough permission to view the webcams. If not an admin
    # person, then must be a project with permission to use the robots.
    # Eventually this needs to be a much more restrictive test.
    #
    function WebCamAllowed() {
	$uid_idx = $this->uid_idx();
	
	$query_result =
	    DBQueryFatal("select distinct class from group_membership as g ".
			 "left join nodetypeXpid_permissions as p on ".
			 "     g.pid=p.pid ".
			 "left join node_types as nt on nt.type=p.type ".
			 "where g.uid_idx='$uid_idx' and class='robot'");
	
	return mysql_num_rows($query_result);
    }
}
