<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2006-2011 University of Utah and the Flux Group.
# All rights reserved.
#
include_once("osinfo_defs.php");	# For SpitOSIDLink() below.

class Image
{
    var	$image;
    var $types;
    var $group;
    var $project;

    #
    # Constructor by lookup on unique ID
    #
    function Image($id) {
	$safe_id = addslashes($id);

	$query_result =
	    DBQueryWarn("select * from images ".
			"where imageid='$safe_id'");

	if (!$query_result || !mysql_num_rows($query_result)) {
	    $this->image = NULL;
	    return;
	}
	$this->image = mysql_fetch_array($query_result);

	#
	# Load the type info.
	#
	$types = array();
	
	$query_result = 
	    DBQueryFatal("select distinct type from osidtoimageid ".
			 "where imageid='$safe_id'");
	
	while ($row = mysql_fetch_array($query_result)) {
	    $types[] = $row['type'];
	}
	$this->types = $types;

	# Load lazily;
	$this->group      = null;
	$this->project    = null;
    }

    # Hmm, how does one cause an error in a php constructor?
    function IsValid() {
	return !is_null($this->image);
    }

    # Lookup by imageid
    function Lookup($id) {
	$foo = new Image($id);

	if (! $foo->IsValid())
	    return null;

	return $foo;
    }

    # Lookup by imagename in a project
    function LookupByName($project, $name) {
	$pid       = $project->pid();
	$safe_name = addslashes($name);
	
	$query_result =
	    DBQueryFatal("select imageid from images ".
			 "where pid='$pid' and imagename='$safe_name'");

	if (mysql_num_rows($query_result) == 0) {
	    return null;
	}
	$row = mysql_fetch_array($query_result);
	return Image::Lookup($row["imageid"]);
    }

    #
    # Refresh an instance by reloading from the DB.
    #
    function Refresh() {
	if (! $this->IsValid())
	    return -1;

	$imageid = $this->imageid();

	$query_result =
	    DBQueryWarn("select * from images where imageid='$imageid'");
    
	if (!$query_result || !mysql_num_rows($query_result)) {
	    $this->imageid = NULL;
	    return -1;
	}
	$this->imageid = mysql_fetch_array($query_result);

	#
	# Reload the type info.
	#
	$types = array();
	
	$query_result = 
	    DBQueryFatal("select distinct type from osidtoimageid ".
			 "where imageid='$imageid'");
	
	while ($row = mysql_fetch_array($query_result)) {
	    $types[] = $row['type'];
	}
	$this->types = $types;
	
	return 0;
    }

    #
    # Class function to create a new image descriptor.
    #
    function NewImageId($ez, $imagename, $args, &$errors) {
	global $suexec_output, $suexec_output_array;

        #
        # Generate a temporary file and write in the XML goo.
        #
	$xmlname = tempnam("/tmp", $ez ? "newimageid_ez" : "newimageid");
	if (! $xmlname) {
	    TBERROR("Could not create temporary filename", 0);
	    $errors[] = "Transient error(1); please try again later.";
	    return null;
	}
	if (! ($fp = fopen($xmlname, "w"))) {
	    TBERROR("Could not open temp file $xmlname", 0);
	    $errors[] = "Transient error(2); please try again later.";
	    return null;
	}

	# Add these. Maybe caller should do this?
	$args["imagename"] = $imagename;

	fwrite($fp, "<image>\n");
	foreach ($args as $name => $value) {
	    fwrite($fp, "<attribute name=\"$name\">");
	    fwrite($fp, "  <value>" . htmlspecialchars($value) . "</value>");
	    fwrite($fp, "</attribute>\n");
	}
	fwrite($fp, "</image>\n");
	fclose($fp);
	chmod($xmlname, 0666);

	$script = "webnewimageid" . ($ez ? "_ez" : "");
	$retval = SUEXEC("nobody", "nobody", "$script $xmlname",
			 SUEXEC_ACTION_IGNORE);

	if ($retval) {
	    if ($retval < 0) {
		$errors[] = "Transient error(3, $retval); please try again later.";
		SUEXECERROR(SUEXEC_ACTION_CONTINUE);
	    }
	    else {
		# unlink($xmlname);
		if (count($suexec_output_array)) {
		    for ($i = 0; $i < count($suexec_output_array); $i++) {
			$line = $suexec_output_array[$i];
			if (preg_match("/^([-\w]+):\s*(.*)$/",
				       $line, $matches)) {
			    $errors[$matches[1]] = $matches[2];
			}
			else
			    $errors[] = $line;
		    }
		}
		else
		    $errors[] = "Transient error(4, $retval); please try again later.";
	    }
	    return null;
	}

        #
        # Parse the last line of output. Ick.
        #
	unset($matches);
	
	if (!preg_match("/^IMAGE\s+([^\/]+)\/(\d+)\s+/",
			$suexec_output_array[count($suexec_output_array)-1],
			$matches)) {
	    $errors[] = "Transient error(5); please try again later.";
	    SUEXECERROR(SUEXEC_ACTION_CONTINUE);
	    return null;
	}
	$image = $matches[2];
	$newimage = image::Lookup($image);
	if (! $newimage) {
	    $errors[] = "Transient error(6); please try again later.";
	    TBERROR("Could not lookup new image $image", 0);
	    return null;
	}

	# Unlink this here, so that the file is left behind in case of error.
	# We can then create the image by hand from the xmlfile, if desired.
	unlink($xmlname);
	return $newimage; 
    }

    #
    # Class function to edit an image descriptor.
    #
    function EditImageid($image, $args, &$errors) {
	global $suexec_output, $suexec_output_array;

        #
        # Generate a temporary file and write in the XML goo.
        #
	$xmlname = tempnam("/tmp", "editimageid");
	if (! $xmlname) {
	    TBERROR("Could not create temporary filename", 0);
	    $errors[] = "Transient error(1); please try again later.";
	    return null;
	}
	if (! ($fp = fopen($xmlname, "w"))) {
	    TBERROR("Could not open temp file $xmlname", 0);
	    $errors[] = "Transient error(2); please try again later.";
	    return null;
	}

	# Add these. Maybe caller should do this?
	$args["imageid"] = $image->imageid();

	fwrite($fp, "<image>\n");
	foreach ($args as $name => $value) {
	    fwrite($fp, "<attribute name=\"$name\">");
	    fwrite($fp, "  <value>" . htmlspecialchars($value) . "</value>");
	    fwrite($fp, "</attribute>\n");
	}
	fwrite($fp, "</image>\n");
	fclose($fp);
	chmod($xmlname, 0666);

	$retval = SUEXEC("nobody", "nobody", "webeditimageid $xmlname",
			 SUEXEC_ACTION_IGNORE);

	if ($retval) {
	    if ($retval < 0) {
		$errors[] = "Transient error(3, $retval); please try again later.";
		SUEXECERROR(SUEXEC_ACTION_CONTINUE);
	    }
	    else {
		# unlink($xmlname);
		if (count($suexec_output_array)) {
		    for ($i = 0; $i < count($suexec_output_array); $i++) {
			$line = $suexec_output_array[$i];
			if (preg_match("/^([-\w]+):\s*(.*)$/",
				       $line, $matches)) {
			    $errors[$matches[1]] = $matches[2];
			}
			else
			    $errors[] = $line;
		    }
		}
		else
		    $errors[] = "Transient error(4, $retval); please try again later.";
	    }
	    return null;
	}

	# There are no return value(s) to parse at the end of the output.

	# Unlink this here, so that the file is left behind in case of error.
	# We can then create the image by hand from the xmlfile, if desired.
	unlink($xmlname);
	return true;
    }

    #
    # Equality test.
    #
    function SameImage($image) {
	return $image->imageid() == $this->imageid();
    }

    # accessors
    function field($name) {
	return (is_null($this->image) ? -1 : $this->image[$name]);
    }
    function imagename()	{ return $this->field("imagename"); }
    function pid()		{ return $this->field("pid"); }
    function gid()		{ return $this->field("gid"); }
    function pid_idx()		{ return $this->field("pid_idx"); }
    function gid_idx()		{ return $this->field("gid_idx"); }
    function imageid()		{ return $this->field("imageid"); }
    function uuid()		{ return $this->field("uuid"); }
    function creator()		{ return $this->field("creator"); }
    function creator_idx()	{ return $this->field("creator_idx"); }
    function created()		{ return $this->field("created"); }
    function description()	{ return $this->field("description"); }
    function loadpart()		{ return $this->field("loadpart"); }
    function loadlength()	{ return $this->field("loadlength"); }
    function part1_osid()	{ return $this->field("part1_osid"); }
    function part2_osid()	{ return $this->field("part2_osid"); }
    function part3_osid()	{ return $this->field("part3_osid"); }
    function part4_osid()	{ return $this->field("part4_osid"); }
    function default_osid()	{ return $this->field("default_osid"); }
    function path()		{ return $this->field("path"); }
    function magic()		{ return $this->field("magic"); }
    function ezid()		{ return $this->field("ezid"); }
    function shared()		{ return $this->field("shared"); }
    function isglobal()		{ return $this->field("global"); }
    function updated()		{ return $this->field("updated"); }

    # Return the DB data.
    function DBData()		{ return $this->image; }
    # and the types array
    function Types()		{ reset($this->types); return $this->types; }

    #
    # Access Check, determines if $user can access $this record.
    # 
    function AccessCheck($user, $access_type) {
	global $TB_IMAGEID_READINFO;
	global $TB_IMAGEID_MODIFYINFO;
	global $TB_IMAGEID_DESTROY;
	global $TB_IMAGEID_ACCESS;
	global $TB_IMAGEID_EXPORT;
	global $TB_IMAGEID_MIN;
	global $TB_IMAGEID_MAX;
	global $TBDB_TRUST_USER;
	global $TBDB_TRUST_GROUPROOT;
	global $TBDB_TRUST_LOCALROOT;
	$mintrust = $TB_IMAGEID_READINFO;

	if ($access_type < $TB_IMAGEID_MIN || $access_type > $TB_IMAGEID_MAX) {
	    TBERROR("Invalid access type $access_type!", 1);
	}

        #
        # Admins do whatever they want!
        # 
	if (ISADMIN()) {
	    return 1;
	}

	$shared = $this->shared();
	$global = $this->isglobal();
	$imageid= $this->imageid();
	$pid    = $this->pid();
	$gid    = $this->gid();
	$uid    = $user->uid();
	$uid_idx= $user->uid_idx();
	$pid_idx= $user->uid_idx();
	$gid_idx= $user->uid_idx();

        #
        # Global ImageIDs can be read by anyone but written by Admins only.
        # 
	if ($global) {
	    if ($access_type == $TB_IMAGEID_READINFO) {
		return 1;
	    }
	    return 0;
	}

        #
        # Otherwise must have proper trust in the project.
        # 
	if ($access_type == $TB_IMAGEID_READINFO) {
	    $mintrust = $TBDB_TRUST_USER;
            #
            # Shared imageids are readable by anyone in the project.
            #
	    if ($shared)
		$gid = $pid;
	}
	else {
	    $mintrust = $TBDB_TRUST_LOCALROOT;
	}

	if (TBMinTrust(TBGrpTrust($uid, $pid, $gid), $mintrust) ||
	    TBMinTrust(TBGrpTrust($uid, $pid, $pid), $TBDB_TRUST_GROUPROOT)) {
	    return 1;
	}
        # No point in looking further; never allowed.
	if ($access_type == $TB_IMAGEID_EXPORT) {
	    return 0;
	}
	
	#
	# Look in the image permissions. First look for a user permission,
	# then look for a group permission.
	#
	$query_result = 
	    DBQueryFatal("select allow_write from image_permissions ".
			 "where imageid='$imageid' and ".
			 "      permission_type='user' and ".
			 "      permission_idx='$uid_idx'");
	
	if (mysql_num_rows($query_result)) {
	    $row  = mysql_fetch_array($query_result);

            # Only allowed to read.
	    if ($access_type == $TB_IMAGEID_READINFO ||
		$access_type == $TB_IMAGEID_ACCESS)
		return 1;
	}
	$trust_none = TBDB_TRUSTSTRING_NONE;
	$query_result = 
	    DBQueryFatal("select allow_write from group_membership as g ".
			 "left join image_permissions as p on ".
			 "     p.permission_type='group' and ".
			 "     p.permission_idx=g.gid_idx ".
			 "where g.uid_idx='$uid_idx' and ".
			 "      p.imageid='$imageid' and ".
			 "      trust!='$trust_none'");

	if (mysql_num_rows($query_result)) {
            # Only allowed to read.
	    if ($access_type == $TB_IMAGEID_READINFO ||
		$access_type == $TB_IMAGEID_ACCESS)
		return 1;
	}
	return 0;
    }

    #
    # Load the project object for an experiment.
    #
    function Project() {
	$pid_idx = $this->pid_idx();

	if ($this->project)
	    return $this->project;

	$this->project = Project::Lookup($pid_idx);
	if (! $this->project) {
	    TBERROR("Could not lookup project $pid_idx!", 1);
	}
	return $this->project;
    }
    #
    # Load the group object for an experiment.
    #
    function Group() {
	$gid_idx = $this->gid_idx();

	if ($this->group)
	    return $this->group;

	$this->group = Group::Lookup($gid_idx);
	if (! $this->group) {
	    TBERROR("Could not lookup group $gid_idx!", 1);
	}
	return $this->group;
    }

    function Show($showperms = 0) {
	$imageid	= $this->imageid();
	$imagename	= $this->imagename();
	$pid		= $this->pid();
	$gid		= $this->gid();
	$description	= $this->description();
	$loadpart	= $this->loadpart();
	$loadlength	= $this->loadlength();
	$part1_osid	= $this->part1_osid();
	$part2_osid	= $this->part2_osid();
	$part3_osid	= $this->part3_osid();
	$part4_osid	= $this->part4_osid();
	$default_osid	= $this->default_osid();
	$path		= $this->path();
	$shared		= $this->shared();
	$globalid	= $this->isglobal();
	$creator	= $this->creator();
	$created	= $this->created();
	$uuid           = $this->uuid();

	if (!$description)
	    $description = "&nbsp;";
	if (!$path)
	    $path = "&nbsp;";
	if (!$created)
	    $created = "N/A";
    
        #
        # Generate the table.
        #
	echo "<table align=center border=2 cellpadding=2 cellspacing=2>\n";

	echo "<tr>
                <td>Image Name: </td>
                <td class=\"left\">$imagename</td>
              </tr>\n";

	echo "<tr>
                <td>Project: </td>
                <td class=\"left\">
                  <a href='showproject.php3?pid=$pid'>$pid</a></td>
              </tr>\n";

	echo "<tr>
                  <td>Group: </td>
                  <td class=\"left\">
                    <a href='showgroup.php3?pid=$pid&gid=$gid'>$gid</a></td>
              </tr>\n";
    
	echo "<tr>
                <td>Creator: </td>
                <td class=left>$creator</td>
     	      </tr>\n";

	echo "<tr>
                <td>Created: </td>
                <td class=left>$created</td>
 	      </tr>\n";

	echo "<tr>
                <td>Description: </td>
                <td class=left>\n";
	echo "$description";
	echo "   </td>
 	      </tr>\n";

	echo "<tr>
                <td>Load Partition: </td>
                <td class=\"left\">$loadpart</td>
              </tr>\n";

	echo "<tr>
                <td>Load Length: </td>
                <td class=\"left\">$loadlength</td>
              </tr>\n";

	if ($part1_osid) {
	    echo "<tr>
                     <td>Partition 1 OS: </td>
                     <td class=\"left\">";
	    SpitOSIDLink($part1_osid);
	    echo "   </td>
                  </tr>\n";
	}

	if ($part2_osid) {
	    echo "<tr>
                     <td>Partition 2 OS: </td>
                     <td class=\"left\">";
	    SpitOSIDLink($part2_osid);
	    echo "   </td>
                  </tr>\n";
	}

	if ($part3_osid) {
	    echo "<tr>
                     <td>Partition 3 OS: </td>
                     <td class=\"left\">";
	    SpitOSIDLink($part3_osid);
	    echo "   </td>
                  </tr>\n";
	}

	if ($part4_osid) {
	    echo "<tr>
                     <td>Partition 4 OS: </td>
                     <td class=\"left\">";
	    SpitOSIDLink($part4_osid);
	    echo "   </td>
                  </tr>\n";
	}

	if ($default_osid) {
	    echo "<tr>
                     <td>Boot OS: </td>
                     <td class=\"left\">";
	    SpitOSIDLink($default_osid);
	    echo "   </td>
                  </tr>\n";
	}

	echo "<tr>
                <td>Filename: </td>
                <td class=left>\n";
	echo "$path";
	echo "  </td>
              </tr>\n";

	echo "<tr>
                  <td>Types: </td>
                  <td class=left>\n";
	echo "&nbsp;";
	foreach ($this->Types() as $type) {
	    echo "$type &nbsp; ";
	}
	echo "  </td>
              </tr>\n";

	echo "<tr>
                <td>Shared?: </td>
                <td class=left>\n";
	if ($shared)
	    echo "Yes";
	else
	    echo "No";
    
	echo "  </td>
              </tr>\n";

	echo "<tr>
                <td>Global?: </td>
                <td class=left>\n";

	if ($globalid)
	    echo "Yes";
	else
	    echo "No";
    
	echo "  </td>
              </tr>\n";

	echo "<tr>
                <td>Internal ID: </td>
                <td class=left>$imageid</td>
              </tr>\n";

	echo "<tr>
                <td>UUID: </td>
                <td class=left>$uuid</td>
              </tr>\n";

	#
	# Show who all can access this image outside the project.
	#
	if ($showperms) {
	    $query_result =
		DBQueryFatal("select * from image_permissions ".
			     "where imageid='$imageid' ".
			     "order by permission_type,permission_id");
	    if (mysql_num_rows($query_result)) {
		echo "<tr>
                      <td align=center colspan=2>
                      External permissions
                      </td>
                  </tr>\n";

		while ($row = mysql_fetch_array($query_result)) {
		    $perm_type = $row['permission_type'];
		    $perm_idx  = $row['permission_idx'];
		    $writable  = $row['allow_write'];

		    if ($writable) {
			$writable = "(read/write)";
		    }
		    else {
			$writable = "(read only)";
		    }

		    if ($perm_type == "user") {
			$user = User::Lookup($perm_idx);
			if (isset($user)) {
			    $uid = $user->uid();
			    echo "<tr>
                                    <td>User: </td>
                                    <td class=left>$uid $writable</td>
                                  </tr>\n";
			}
		    }
		    elseif ($perm_type == "group") {
			$group = Group::Lookup($perm_idx);
			if (isset($group)) {
			    $pid = $group->pid();
			    $gid = $group->gid();
			    echo "<tr>
                                    <td>Group: </td>
                                    <td class=left>$pid/$gid $writable</td>
                                  </tr>\n";
			}
		    }
		}
	    }
	}

	echo "</table>\n";
    }

    #
    # See if an image is inuse.
    #
    function InUse() {
	$imageid = $this->imageid();

	$query_result1 =
	    DBQueryFatal("select * from current_reloads ".
			 "where image_id='$imageid'");
	$query_result2 =
	    DBQueryFatal("select * from scheduled_reloads ".
			 "where image_id='$imageid'");
	$query_result3 =
	    DBQueryFatal("select * from node_type_attributes ".
			 "where attrkey='default_imageid' and ".
			 "      attrvalue='$imageid' limit 1");

	if (mysql_num_rows($query_result1) ||
	    mysql_num_rows($query_result2) ||
	    mysql_num_rows($query_result3)) {
	    return 1;
	}
	return 0;
    }
}
