<?php
#
# EMULAB-COPYRIGHT
# Copyright (c) 2000-2005 University of Utah and the Flux Group.
# All rights reserved.
#
chdir("..");
include("defs.php3");

$uid = GETLOGIN();
LOGGEDINORDIE($uid);

#$TARGET_FILE = "wireless-stats.data.zip";
$TARGET_FILE = "wireless-stats.data";

function death($msg) {
    PAGEHEADER("Wireless Connectivity Data");
    PAGEARGERROR($msg);
    PAGEFOOTER();

    exit(1);
}

## we need a type and a dataset.
if (isset($type) && isset($dataset) && !preg_match("/^[-\w]+$/", $dataset)) {
    $dbq = DBQueryFatal("select * from wireless_stats where name='$dataset'");

    if (mysql_num_rows($dbq)) {
        $row = mysql_fetch_array($dbq);
        $floor = $row["floor"];
        $building = $row["building"];
        $eid = $row["data_eid"];
        $pid = $row["data_pid"];
        $altsrc = $row["altsrc"];

        ## generate the data
	if ($type == "data") {
            ## find the file...
            #$path = "/proj/$pid/exp/$eid/logs/$TARGET_FILE";
            $path = "./wireless-stats/$pid.$eid-$TARGET_FILE";
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

            #header("Content-type: image/jpg");
            #header("Content-Length: " . (filesize($tmpfile . ".jpg")));
            if (($fp = fopen("$tmpfile.jpg", "r"))) {
                header("Content-type: image/jpg");
                fpassthru($fp);
            }
	    else {
	        death("Error while reading image file!");
            }

	    $gen_args = "-k -o $tmpfile";
	    $retval = SUEXEC($uid,"nobody","webfloormap $gen_args",
                             SUEXEC_ACTION_IGNORE);

            #unlink($tmpfile . ".jpg");
	    #unlink($tmp
        }
        else if ($type == "posit") {
            $dbq = DBQueryFatal("select node_id,loc_x,loc_y,loc_z from " . 
                "location_info where floor=$floor and building='$building'");
            if (mysql_num_rows($dbq)) {
                header("Content-Type: text/plain");

                while ($row = mysql_fetch_array($dbq)) {
                    echo "" . $row["node_id"] . " \t" . $row["loc_x"] . 
                        " \t" . $row["loc_y"] . " \t" . $row["loc_z"] . "\n";
                }
            }
            else {
                death("No position information for floor $floor and " . 
                      "building $building!");
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
