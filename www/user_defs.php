<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006 University of Utah and the Flux Group.
# All rights reserved.
#
class User
{
    var	$user;

    #
    # Constructor by lookup on unique index.
    #
    function User($uid_idx) {
	$safe_uid_idx = addslashes($uid_idx);

	$query_result =
	    DBQueryWarn("select * from users where uid_idx='$safe_uid_idx'");

	if (!$query_result || !mysql_num_rows($query_result)) {
	    $this->user = NULL;
	    return;
	}
	$this->user = mysql_fetch_array($query_result);
    }

    # Hmm, how does one cause an error in a php constructor?
    function IsValid() {
	return !is_null($this->user);
    }

    # Lookup by uid_idx.
    function Lookup($uid_idx) {
	$foo = new User($uid_idx);

	if ($foo->IsValid())
	    return $foo;
	return null;
    }

    # Backwards compatable lookup by uid. Will eventually flush this.
    function LookupByUid($uid) {
	$safe_uid = addslashes($uid);

	$query_result =
	    DBQueryWarn("select uid_idx from users where uid='$safe_uid'");

	if (!$query_result || !mysql_num_rows($query_result)) {
	    return null;
	}
	$row = mysql_fetch_array($query_result);
	$idx = $row['uid_idx'];

	$foo = new User($idx); 

	if ($foo->IsValid())
	    return $foo;
	
	return null;
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
	$this->user = mysql_fetch_array($query_result);
	return 0;
    }

    # accessors
    function field($name) {
	return (is_null($this->user) ? -1 : $this->user[$name]);
    }
    function uid_idx()		{ return $this->field("uid_idx"); }
    function uid()		{ return $this->field("uid"); }
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
    function NewUser($uid, $isleader, $wikionly, $args) {
	global $TBBASE, $TBMAIL_APPROVAL, $TBMAIL_AUDIT, $TBMAIL_WWW;
	global $MIN_UNIX_UID;

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
	$insert_data[] = "wikionly='$wikionly'";
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
	 "will be granted full access to your user account.")) .
       "\n\n".
       "Thanks,\n".
       "Testbed Operations\n",
       "From: $TBMAIL_APPROVAL\n".
       "Bcc: $TBMAIL_AUDIT\n".
       "Errors-To: $TBMAIL_WWW");
	
	return $newuser;
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
}
