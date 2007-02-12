<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2007 University of Utah and the Flux Group.
# All rights reserved.
#
chdir("..");
include("defs.php3");

$this_user = CheckLoginOrDie();
$uid       = $this_user->uid();
$isadmin   = ISADMIN();

function death($msg) {
    PAGEHEADER("Wireless Connectivity Data");
    PAGEARGERROR($msg);
    PAGEFOOTER();
    exit(1);
}

#
# Verify page arguments
#
$reqargs = RequiredPageArguments("type",    PAGEARG_STRING.
				 "dataset", PAGEARG_STRING);

## we need a type and a dataset.
if (isset($type) && isset($dataset) && preg_match("/^[a-zA-Z0-9\-_]+$/", $dataset)) {
    $dbq = DBQueryFatal("select * from wireless_stats where name='$dataset'");

    if (mysql_num_rows($dbq)) {
        $row = mysql_fetch_array($dbq);
        $dbFloor = $row["floor"];
        $dbBuilding = $row["building"];
        $eid = $row["data_eid"];
        $pid = $row["data_pid"];
        $altsrc = $row["altsrc"];

        ## generate the data
	if ($type == "data") {
	    ## find the file...
	    $path = "/usr/testbed/www/wireless-stats/$dataset-$pid-$eid.log";
	    if (file_exists($path)) {
		## read and dump the file:
                #header("Content-Type: application/zip");
                #header("Content-Length: " . (filesize($path)));
		header("Content-Type: text/plain");
		## obviously, this duplication of headers might not
		## make all browsers very happy... if there's a 
                ## problem with the readfile call.
                readfile($path) or death("Error while reading data file!");
            }
            else if ($altsrc != "" && file_exists($altsrc)) {
                header("Content-Type: text/plain");
                header("Content-Length: " . (filesize($altsrc)));
                readfile($altsrc) 
                   or death("Error while reading alternate data file!");
            }
            else {
                death("Unable to find any data file at $path !");
            }
        }
        else if ($type == "map") {
            ## first check for required params
            if ((isset($building) && preg_match("/^[A-Za-z0-9\-]+$/", $building))
                && (isset($scale) && preg_match("/^[-\d\.]+$/", $scale))
                && (isset($floor) && preg_match("/^[-\d\.]+$/", $floor))) {
                ;
            }
            else {
                death("Improper map request argument!");
            }

            $pushfile = '';

            # the one special case... reason is, we want the obstacles
            # overlaid, so we have to generate any of these right off.
            if ($building == 'MEB-ROBOTS') {
                ## generate a map and dump the data.  then unlink it...
                $tmpfile = tempnam("/tmp","wimap");
                $gen_args = "-o $tmpfile -t -z -y -f $floor $building";
                $retval = SUEXEC($uid,"nobody","webfloormap $gen_args",
                                 SUEXEC_ACTION_IGNORE);
	        sleep(1);
                if ($retval) {
                    SUEXECERROR(SUEXEC_ACTION_USERERROR);
                    # Never returns.
                    die("");
                }
                $pushfile = $tmpfile . ".jpg";
            }
            else {
                # grab path from db
                $query = "select image_path from floorimages where " .
                    "building='$building' and floor=$floor and scale=$scale";
                $dbq = DBQueryFatal($query);
                if (mysql_num_rows($dbq)) {
                    $row = mysql_fetch_array($dbq);
                    $if = $row['image_path'];
                    $if = "/usr/testbed/www/floormap/$if";
                    $pushfile = $if;
                }
                else {
                    death("No image file in db!");
                }

            }

            #header("Content-type: image/jpg");
            #header("Content-Length: " . (filesize($tmpfile)));
            if (($fp = fopen("$pushfile", "r"))) {
                header("Content-type: image/jpg");
                fpassthru($fp);
            }
	    else {
	        death("Error while reading image file!");
            }

            # clean up if building was MEB-ROBOTS
            if ($building == 'MEB-ROBOTS') {
                $gen_args = "-k -o $tmpfile";
	        $retval = SUEXEC($uid,"nobody","webfloormap $gen_args",
                                 SUEXEC_ACTION_IGNORE);

                #unlink($tmpfile . ".jpg");
	        #unlink($tmp
            }
        }
        else if ($type == "posit") {
            $dbq = DBQueryFatal("select node_id,loc_x,loc_y,loc_z,floor from " . 
                "location_info where building='$dbBuilding'");

            if (mysql_num_rows($dbq)) {
                header("Content-Type: text/plain");

                while ($row = mysql_fetch_array($dbq)) {
                    $z = '';
                    if ($row["loc_z"] == '') {
                        $z = '0';
                    }
                    echo "" . $row["node_id"] . " \t" . $row["loc_x"] . 
                        " \t" . $row["loc_y"] . " \t" . $z . 
                        " \t" . $row["floor"] . "\n";
                }
            }
            else {
                death("No position information for floor $dbFloor and " . 
                      "building $dbBuilding!");
            }
          
        }
        else {
            death("Unknown type of output requested!");
        }
    }
    else {
        death("Unknown dataset.");
    }
}
else {
    ## error
    death("You must supply a valid dataset name and " . 
          "type of data to generate!");
}

## we're good, theoretically...
exit(0);

?>
