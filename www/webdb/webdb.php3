<?
/*
 * WebDB - testbed adaptation of: 
 *
 * MySQL Web Interface Version 0.8
 * -------------------------------
 * Developed By SooMin Kim (smkim@popeye.snu.ac.kr)
 * License : GNU Public License (GPL)
 * Homepage : http://popeye.snu.ac.kr/~smkim/mysql
 */

// if they're not using https, now is a great time to start. 
if (getenv("HTTPS") != "on") {
	$towhere = "https://" . $SERVER_NAME . $PHP_SELF;
	echo "<html>";
	echo "<head>";
  	echo "<META HTTP-EQUIV=Refresh CONTENT='0; URL=" . $towhere . "'>";
	echo "</head><body>";
        echo "<h1>Redirecting to HTTPS...</h1>";
        echo "</body></html>";
	exit;
}

header("Pragma: no-cache");

$HOSTNAME = "localhost";

// forgo the login process; toss them where they likely want to be. 
function logon_submit() {
	global $username, $password, $PHP_SELF;

//	setcookie( "mysql_web_admin_username", $username );
//	setcookie( "mysql_web_admin_password", $password );
	echo "<html>";
	echo "<head>";
	echo "<META HTTP-EQUIV=Refresh CONTENT='0; URL=$PHP_SELF?action=view&dbname=tbdb'>";
	echo "</head>";
	echo "</html>";
}

function echoQueryResult() {
	global $queryStr, $errMsg;

	if( $errMsg == "" ) $errMsg = "Success";
	if( $queryStr != "" ) {
		echo "<table cellpadding=5>\n";
		echo "<tr><td>Query</td><td>$queryStr</td></tr>\n";
		echo "<tr><td>Result</td><td>$errMsg</td></tr>\n";
		echo "</table><p>\n";
	}
}


function linkToViewDB( $text, $dbname ) {
  global $PHP_SELF;
  return "<a href=\"$PHP_SELF?action=view&dbname=$dbname\">$text</a>";
}

function linkToViewServer( $text ) {
  global $PHP_SELF;
  return "<a href=\"$PHP_SELF?action=view\">$text</a>";
}

function linkToViewTable( $text, $dbname, $tablename ) {
  global $PHP_SELF;
  return "<a href=\"$PHP_SELF?action=view&dbname=$dbname&tablename=$tablename\">$text</a>";
}

function pathStringNoLinks() {
  	global $PHP_SELF, $dbname, $tablename;
        $r = "MySQL";
        if ($dbname) {
	   $r .= "::" . $dbname;
           if ($tablename) {
             $r .= "::" . $tablename;
             $r = "Table " . $r;
	   } else {
             $r = "Database " . $r;
           }
        } else {
	  $r = "Server " . $r;
	}

	return $r;
}

function pathString() {
  	global $PHP_SELF, $dbname, $tablename;

        $r  = linkToViewServer( "MySQL" );
        if ($dbname) {
	   $r .= "::" . linkToViewDB( $dbname, $dbname );
           if ($tablename) {
             $r .= "::" . linkToViewTable( $tablename, $dbname, $tablename ); 
             $r = "Table " . $r;
           } else {
             $r = "Database " . $r;
           }
	} else {
	  $r = "Server " . $r;
        }  

	return $r;
}	

function sanitizeSQLName( $n ) {
  $r = $n;
  // XXX fix.
  $r = strtr( $r, " '\"$!@#%^&()*\\[]{}<>,./?=+~`",
                  "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
//  $r = str_replace( "\\", "\\\\", $r );
//  $r = str_replace( "'", "\\'", $r );
  return $r;
}



function quoteForSQL( $l ) {
        if ($l == "") { return "''"; }
	$r = addslashes( $l );
	$r = str_replace('$', '\$', $r );
	$r = "'" . $r . "'";
/*
        $r = $l;
	$r = str_replace( "\\", "\\\\", $r );
	$r = str_replace( "'", "\\'", $r );
	$r = "'".$r."'";
*/
	return $r;
}

function badPerms() {
?>
  <h1>Error:</h1>
  <h3>You do not have permission to perform this action.</h3>
<?php }


function tablePerms() {
  global $PHP_SELF, $dbname, $tablename;
  global $readTablePerm, $addEditRowPerm, $deleteRowPerm;
 
  $readTablePerm  = 1;
  $addEditRowPerm = 0;
  $deleteRowPerm  = 0;

//  echo "hi2";
  $q = "SELECT * FROM webdb_table_permissions  WHERE table_name=". quoteForSQL( $tablename );
//  echo "<pre>$q</pre>";
  $pResult = mysql_db_query( $dbname, $q ); 
//  echo "<pre>Result: $pResult</pre>";
  if ($pResult != FALSE && mysql_num_rows($pResult) > 0) {
//    echo "result!";
//    echo "<pre>Found perm line.</pre>\n";
    $field = mysql_fetch_array( $pResult );
    if ($field["allow_read"] != "1") { $readTablePerm = 0; }
    if ($field["allow_row_add_edit"] == "1") { $addEditRowPerm = 1; }
    if ($field["allow_row_delete"] == "1") { $deleteRowPerm = 1; } 
  } 
}

function listDatabases() {
	global $mysqlHandle, $PHP_SELF;
	
	echo "<h1>" . pathString() . "</h1>";
	echo "<p>Click to view tables in database.</p>\n";
		 echo "<p><b>Note:</b> <i>Due to security concerns," .
                      " you may only view databases with names" .
		      " containing the substring \"tbdb\".</i></p>"; 

	echo "<table cellspacing=1 cellpadding=5>\n";

	echo "<tr><th>database name</th></tr>";
	$pDB = mysql_list_dbs( $mysqlHandle );
	$num = mysql_num_rows( $pDB );
	for( $i = 0; $i < $num; $i++ ) {
		$dbname = mysql_dbname( $pDB, $i );
		echo "<tr>\n";
		echo "<td>" . linkToViewDB( $dbname, $dbname ) . "</td>";
		echo "</tr>\n";
	}
	echo "</table>\n";
}

function listTables() {
	global $mysqlHandle, $dbname, $PHP_SELF;

	echo "<h1>" . pathString() . "</h1>";
	echo "<p>Click to view or edit table information.</p>\n";
//	echoQueryResult();
	
	$pResult = mysql_db_query( $dbname, "SELECT * FROM comments WHERE column_name IS NULL" );
        if ($pResult != "") {	
	  $num = mysql_num_rows( $pResult );
	  for( $i = 0; $i < $num; $i++ ) {
		$field = mysql_fetch_array( $pResult );
  		$table_comment[$field["table_name"]] = $field["description"]; 
	  }
        }

	$pTable = mysql_list_tables( $dbname );

	if( $pTable == 0 ) {
		$msg  = mysql_error();
		echo "<h3>Error : $msg</h3><p>\n";
		return;
	}
	$num = mysql_num_rows( $pTable );

	echo "<table cellspacing=1 cellpadding=5>\n";

	echo "<tr>\n";
	echo "<th align=left>table name</th>\n";
	echo "<th align=left>description</th>\n";
	echo "</tr>\n";

	for( $i = 0; $i < $num; $i++ ) {
		$tablename = mysql_tablename( $pTable, $i );

		echo "<tr>\n";
		echo "<td>\n";
		echo "<b>" . linkToViewTable( $tablename, $dbname, $tablename ) . "</b>";
		echo "</td>\n";
		if ($table_comment[$tablename] != "") {
		  echo "<td>\n";
		  echo "<i>" . $table_comment[$tablename] . "</i>\n";
		  echo "</td>\n";
		}	

		echo "</tr>\n";
	}

	echo "</table>";
}

function setTableDescription( $tablename, $description )
{
  global $mysqlhandle, $dbname;
  $a = quoteForSQL( $tablename );
  $b = "NULL";
  $c = quoteForSQL( $description );

  $q1 = "DELETE FROM comments WHERE table_name=" . $a . " AND column_name IS NULL";
  $q2 = "INSERT INTO comments VALUES (" . $a . "," . $b . "," . $c . ")";

  mysql_db_query( $dbname, $q1 );
  if ($description != "") {
    mysql_db_query( $dbname, $q2 );
  }
}

function printEditDescriptionForm( $oldvalue )
{ 
  global $dbname, $tablename, $PHP_SELF;

  echo "<form action='" . $PHP_SELF . "' method=post>";
  echo "<b>Description:</b>";
  echo "<input type=hidden name=dbname value=\"" . $dbname . "\">";
  echo "<input type=hidden name=tablename value=\"" . $tablename . "\">";
  echo "<input type=hidden name=action value=\"view\">";
  echo "<input type=text   name=description size=80 value=\"" . $oldvalue . "\">";
  echo "<input type=hidden name=set value=\"1\">\n";
  echo "<input type=submit value=\"set\"    name=set>\n";
  echo "<input type=submit value=\"cancel\" name=revert>\n";
  echo "</form>";
}

function viewData() {
	global $mysqlHandle, $dbname, $tablename, $PHP_SELF, $errMsg, $page, $rowperpage, $orderby, $liquidDesc, $set, $description, $revert;
        global $readTablePerm, $addEditRowPerm, $deleteRowPerm;

        tablePerms();

        if ($readTablePerm == 0) {
          badPerms();
	  return;
        }

	if ($set != "" && $revert == "") {
	  setTableDescription( $tablename, $description );
	}

        echo "<h1>" . pathString() . "</h1>";

	$pResult = mysql_db_query( $dbname, "SELECT * FROM comments WHERE column_name IS NULL AND table_name=" . quoteForSQL( $tablename ) );

        if ($pResult != "") {
	  $num = mysql_num_rows( $pResult );
	  if ($num == 0) {
	    if ($liquidDesc != "1") {
	      echo "<b>No description.</b>" .	 
	           "[<a href=\"" . $PHP_SELF . 
	           "?action=view&dbname=$dbname&tablename=$tablename&" . 
                   "liquidDesc=1\">change</a>]<br>\n";
	    } else {
 	       printEditDescriptionForm("");
	    }
	  } 
	  for( $i = 0; $i < $num; $i++ ) {
		$field = mysql_fetch_array( $pResult );

                if ($liquidDesc != "1") {
		echo "<b>Description:</b>";
		echo " <i>" . 
		  $field["description"] . 
		  "</i>[<a href=\"" . $PHP_SELF . 
		  "?action=view&dbname=$dbname&tablename=$tablename&" .
                  "liquidDesc=1\">change</a>]<br>\n";
		} else {
	          printEditDescriptionForm( $field["description"] );
		}	
	  }
        }

	$queryStr = "SELECT * FROM " . sanitizeSQLName( $tablename );
	if( $orderby != "" )
	  $queryStr .= " ORDER BY " . sanitizeSQLName( $orderby );
	
	$pResult = mysql_db_query( $dbname, $queryStr );
	$errMsg = mysql_error();

	$GLOBALS[queryStr] = $queryStr;

	if( $pResult == false ) {
		echoQueryResult();
		return;
	}
	if( $pResult == 1 ) {
		$errMsg = "Success";
		echoQueryResult();
		return;
	}

	echo "<hr>\n";

	$row = mysql_num_rows( $pResult );
	$col = mysql_num_fields( $pResult );

	if( $row == 0 ) {
	  // XXX ick
	  if ($col != 0) {
echo "<table cellspacing=1 cellpadding=2>\n";
	echo "<tr>\n";
	    for( $i = 0; $i < $col; $i++ ) {
		$field = mysql_fetch_field( $pResult, $i );
		echo "<th>";
		echo $field->name;
		echo "</th>\n";
	    }
 	  }
	echo "</tr></table>\n";

		echo "<h1>No Data.</h1>";
		echo "<br>";
if ($addEditRowPerm == 1) {
		echo "[<a href='$PHP_SELF?action=addData&dbname=$dbname&tablename=$tablename'><b>Add data row</b></a>]\n";
}
		return;
	}
	
	if( $rowperpage == "" ) $rowperpage = 20;
	if( $page == "" ) $page = 0;
	else $page--;
	mysql_data_seek( $pResult, $page * $rowperpage );

	echo "<table cellspacing=1 cellpadding=2>\n";
	echo "<tr>\n";
	for( $i = 0; $i < $col; $i++ ) {
		$field = mysql_fetch_field( $pResult, $i );
		echo "<th>";
		echo "<a href='$PHP_SELF?action=view&dbname=$dbname&tablename=$tablename&orderby=".$field->name."'>".$field->name."</a>\n";
		echo "</th>\n";
	}
	echo "<th colspan=2>Action</th>\n";
	echo "</tr>\n";

	for( $i = 0; $i < $rowperpage; $i++ ) {
		$rowArray = mysql_fetch_row( $pResult );
		if( $rowArray == false ) break;
		echo "<tr>\n";
		$key = "";
		for( $j = 0; $j < $col; $j++ ) {
			$data = $rowArray[$j];

			$field = mysql_fetch_field( $pResult, $j );
			if( $field->primary_key == 1 )
				$key .= "&" . $field->name . "=" . $data;

			if( strlen( $data ) > 32 )
				$data = substr( $data, 0, 20 ) . "...";
			$data = htmlspecialchars( $data );
			echo "<td>\n";
			echo "$data\n";
			echo "</td>\n";
		}

		if( $key == "" )
			echo "<td colspan=2>no Key</td>\n";
		else {
if ($addEditRowPerm == 1) {
			echo "<td>[<a href='$PHP_SELF?action=editData&dbname=$dbname&tablename=$tablename$key'>Edit</a>]</td>\n";
}
if ($deleteRowPerm == 1) {
			echo "<td>[<a href='$PHP_SELF?action=deleteData&dbname=$dbname&tablename=$tablename$key' onClick=\"return confirm('Delete Row?')\">Delete</a>]</td>\n";
}
		}
		echo "</tr>\n";
	}
	echo "</table>\n";

	echo "<font size=2>\n";
	echo "<form action='$PHP_SELF?action=view&dbname=$dbname&tablename=$tablename' method=post>\n";
	echo "<font color=green>\n";
	echo ($page+1)."/".(int)($row/$rowperpage+1)." page";
	echo "</font>\n";
	echo " | ";
	if( $page > 0 ) {
		echo "<a href='$PHP_SELF?action=view&dbname=$dbname&tablename=$tablename&page=".($page);
		if( $orderby != "" )
			echo "&orderby=$orderby";
		echo "'>Prev</a>\n";
	} else
		echo "Prev";
	echo " | ";
	if( $page < ($row/$rowperpage)-1 ) {
		echo "<a href='$PHP_SELF?action=view&dbname=$dbname&tablename=$tablename&page=".($page+2);
		if( $orderby != "" )
			echo "&orderby=$orderby";
		echo "'>Next</a>\n";
	} else
		echo "Next";
	if( $row > $rowperpage ) {
		echo " | ";
		echo "<input type=text size=4 name=page>\n";
		echo "<input type=submit value='Go'>\n";
	}
	echo "</form>\n";

	echo "</font>\n";
		echo "<br>";
if ($addEditRowPerm == 1) {
	echo "[<a href='$PHP_SELF?action=addData&dbname=$dbname&tablename=$tablename'><b>Add data row</b></a>]\n";
}
}

function manageData( $cmd ) {
	global $mysqlHandle, $dbname, $tablename, $PHP_SELF;
        global $readTablePerm, $addEditRowPerm, $deleteRowPerm;

        tablePerms();
  
        if ($addEditRowPerm == 0) {
          badPerms();
          return;
        }

	if( $cmd == "add" )
		echo "<h1>Add row to \n";
	else if( $cmd == "edit" ) {
		echo "<h1>Edit row in \n";
		$pResult = mysql_list_fields( $dbname, $tablename );
		$num = mysql_num_fields( $pResult );
	
		$key = "";
		// XXX quotize $GLOBAL refs?
		for( $i = 0; $i < $num; $i++ ) {
			$field = mysql_fetch_field( $pResult, $i );
			if( $field->primary_key == 1 )
				if( $field->numeric == 1 )
					$key .= $field->name . "=" . $GLOBALS[$field->name] . " AND ";
				else
					$key .= $field->name . "='" . $GLOBALS[$field->name] . "' AND ";
		}
		$key = substr( $key, 0, strlen($key)-4 );

		mysql_select_db( $dbname, $mysqlHandle );
		$pResult = mysql_query( $queryStr =  "SELECT * FROM " . sanitizeSQLName($tablename) . " WHERE $key", $mysqlHandle );
		$data = mysql_fetch_array( $pResult );
	}

	echo pathString() . "</h1>\n";

	echo "<p><b>Note:</b> <i>\"NULL\" in a field (no quotes) will set that field to NULL.</i></p>\n";

	echo "<form action='$PHP_SELF' method=post>\n";
	if( $cmd == "add" )
		echo "<input type=hidden name=action value=addData_submit>\n";
	else if( $cmd == "edit" )
		echo "<input type=hidden name=action value=editData_submit>\n";
	echo "<input type=hidden name=dbname value=$dbname>\n";
	echo "<input type=hidden name=tablename value=$tablename>\n";
	echo "<table cellspacing=1 cellpadding=2>\n";
	echo "<tr>\n";
	echo "<th>Name</th>\n";
	echo "<th>Type</th>\n";
//	echo "<th>Function</th>\n";
	echo "<th>Data</th>\n";
	echo "</tr>\n";

	$pResult = mysql_db_query( $dbname, "SHOW fields FROM " . sanitizeSQLName($tablename) );
	$num = mysql_num_rows( $pResult );

	$pResultLen = mysql_list_fields( $dbname, $tablename );

	for( $i = 0; $i < $num; $i++ ) {
		$field = mysql_fetch_array( $pResult );
		$fieldname = $field["Field"];
		$fieldtype = $field["Type"];
		$len = mysql_field_len( $pResultLen, $i );

		echo "<tr>";
		echo "<td>$fieldname</td>";
		echo "<td>".$field["Type"]."</td>";
/*
		echo "<td>\n";
		echo "<select name=${fieldname}_function>\n";
		echo "<option>\n";
		echo "<option>ASCII\n";
		echo "<option>CHAR\n";
		echo "<option>SOUNDEX\n";
		echo "<option>CURDATE\n";
		echo "<option>CURTIME\n";
		echo "<option>FROM_DAYS\n";
		echo "<option>FROM_UNIXTIME\n";
		echo "<option>NOW\n";
		echo "<option>PASSWORD\n";
		echo "<option>PERIOD_ADD\n";
		echo "<option>PERIOD_DIFF\n";
		echo "<option>TO_DAYS\n";
		echo "<option>USER\n";
		echo "<option>WEEKDAY\n";
		echo "<option>RAND\n";
		echo "</select>\n";
		echo "</td>\n";
*/
		$value = htmlspecialchars($data[$i]);
		if( $cmd == "add" ) {
			$type = strtok( $fieldtype, " (,)\n" );
			if( $type == "enum" || $type == "set" ) {
				echo "<td>\n";
				if( $type == "enum" )
					echo "<select name=$fieldname>\n";
				else if( $type == "set" )
					echo "<select name=$fieldname size=4 multiple>\n";
				echo strtok( "'" );
				while( $str = strtok( "'" ) ) {
					echo "<option>$str\n";
					strtok( "'" );
				}
				echo "</select>\n";
				echo "</td>\n";
			} else {
				if( $len < 40 )
					echo "<td><input type=text size=40 maxlength=$len name=$fieldname></td>\n";
				else
					echo "<td><textarea cols=40 rows=3 maxlength=$len name=$fieldname></textarea>\n";
			}
		} else if( $cmd == "edit" ) {
			$type = strtok( $fieldtype, " (,)\n" );
			if( $type == "enum" || $type == "set" ) {
				echo "<td>\n";
				if( $type == "enum" )
					echo "<select name=$fieldname>\n";
				else if( $type == "set" )
					echo "<select name=$fieldname size=4 multiple>\n";
				echo strtok( "'" );
				while( $str = strtok( "'" ) ) {
					if( $value == $str )
						echo "<option selected>$str\n";
					else
						echo "<option>$str\n";
					strtok( "'" );
				}
				echo "</select>\n";
				echo "</td>\n";
			} else {
				if( $len < 40 )
					echo "<td><input type=text size=40 maxlength=$len name=$fieldname value=\"$value\"></td>\n";
				else
					echo "<td><textarea cols=40 rows=3 maxlength=$len name=$fieldname>$value</textarea>\n";
			}
		}
		echo "</tr>";
	}
	echo "</table><p>\n";
	if( $cmd == "add" )
		echo "<input type=submit value='Add Data'>\n";
	else if( $cmd == "edit" )
		echo "<input type=submit value='Edit Data'>\n";
	echo "<input type=button value='Cancel' onClick='history.back()'>\n";
	echo "</form>\n";
}

function manageData_submit( $cmd ) {
	global $mysqlHandle, $dbname, $tablename, $fieldname, $PHP_SELF, $queryStr, $errMsg;
        global $readTablePerm, $addEditRowPerm, $deleteRowPerm;
        tablePerms();
  
        if ($addEditRowPerm == 0) {
          badPerms();
          return;
        }

	$pResult = mysql_list_fields( $dbname, $tablename );
	$num = mysql_num_fields( $pResult );

	mysql_select_db( $dbname, $mysqlHandle );
	if( $cmd == "add" )
	  $queryStr = "INSERT INTO " . sanitizeSQLName( $tablename ) . " VALUES (";
	else if( $cmd == "edit" )
	  $queryStr = "REPLACE INTO " . sanitizeSQLName( $tablename ) . " VALUES (";
	for( $i = 0; $i < $num; $i++ ) {
		$field = mysql_fetch_field( $pResult );
		$func  = $GLOBALS[$field->name."_function"];
		$datum = $GLOBALS[$field->name];
                if ($datum != "NULL") {
			$datum = quoteForSQL( $datum );
		}
		if ($func != "") {
                        /// XXX sanitize $func
			echo "<pre>Error: Functions disabled.</pre><br>";
			//$datum = " $func(" . $datum . ")";
		}
		$queryStr .= $datum;
		if ($i != $num-1) {
		  $queryStr .= ",";
		}
	}
	$queryStr .= ")";

//	echo "<pre>Query is $queryStr.</pre>\n";
	mysql_query( $queryStr , $mysqlHandle );
	$errMsg = mysql_error();

	if ($errMsg != "") { echo "<h3>Error:</h3><br>\n<pre>$errMsg</pre>\n<hr>"; }

	viewData();
}

function deleteData() {
	global $mysqlHandle, $dbname, $tablename, $fieldname, $PHP_SELF, $queryStr, $errMsg;
        global $readTablePerm, $addEditRowPerm, $deleteRowPerm;

        tablePerms();
  
        if ($deleteRowPerm == 0) {
          badPerms();
          return;
        }

	$pResult = mysql_list_fields( $dbname, $tablename );
	$num = mysql_num_fields( $pResult );

	$key = "";
	// XXX sanitize keys?
	for( $i = 0; $i < $num; $i++ ) {
		$field = mysql_fetch_field( $pResult, $i );
		if( $field->primary_key == 1 )
			if( $field->numeric == 1 )
				$key .= $field->name . "=" . $GLOBALS[$field->name] . " AND ";
			else
				$key .= $field->name . "='" . $GLOBALS[$field->name] . "' AND ";
	}
	$key = substr( $key, 0, strlen($key)-4 );

	mysql_select_db( $dbname, $mysqlHandle );
	$queryStr =  "DELETE FROM " . sanitizeSQLName( $tablename ) . " WHERE $key";
	mysql_query( $queryStr, $mysqlHandle );
	$errMsg = mysql_error();

	viewData();
}

function header_html() {
	global $PHP_SELF;
	
	echo "<html>\n<head>\n";
	echo "<title>WebDB: " . pathStringNoLinks() . "</title>\n";
?>
<style type="text/css">
<!--
p.location {
	color: #11bb33;
	font-size: small;
}
body {
	background-color: #EEFFEE;
}
h1 {
	color: #004400;
}
th {
	background-color: #AABBAA;
	color: #000000;
	font-size: x-small;
}
td {
	background-color: #D4E5D4;
	font-size: x-small;
}
form {
	margin-top: 0;
	margin-bottom: 0;
}
a {
	text-decoration:none;
	color: #248200;
}
a:link {
}
a:hover {
	ba2ckground-color:#EEEFD5;
	color:#54A232;
	text-decoration:underline;               
}
//-->
</style>
</head>
<body>
<?
}

function undefinedcommand() {
	echo "<h1>There was a script error.</h1>
	      <h3>(undefined action type)</h3>\n";
}  

function footer_html() {
	global $mysqlHandle, $dbname, $tablename, $PHP_SELF, $USERNAME;

	echo "<hr>\n";
	echo "<font size=2>\n";
//	echo "This utility based on mysql.php3 by <a href='mailto:smkim76@icqmail.com'>SooMin Kim</a>.\n";
	echo "This utility based on mysql.php3 by SooMin Kim.\n";

	echo "</font>\n";
	echo "</body>\n";
	echo "</html>\n";
}

//------------------------------------------------------ MAIN

if( $action == "logon" || $action == "" || $action == "logout" )
	logon_submit();
else if( $action == "logon_submit" ) {
	logon_submit();
} else {
	while( list($var, $value) = each($HTTP_COOKIE_VARS) ) {
		if( $var == "mysql_web_admin_username" ) $USERNAME = $value;
		if( $var == "mysql_web_admin_password" ) $PASSWORD = $value;
	}
	echo "<!--";
	$mysqlHandle = mysql_pconnect( $HOSTNAME, $USERNAME, $PASSWORD );
	echo "-->";

	if( $mysqlHandle == false ) {
		echo "<html>\n";
		echo "<head>\n";
		echo "<title>WebDB<title>\n";
		echo "</head>\n";
		echo "<body>\n";
		echo "<h1>There was an error connecting to the database.</h1>";
		echo "<h3>(comments in page source may contain information.)</h3>"; 
                echo "</body>\n";
		echo "</html>\n";
	} else {
		header_html();
		if ($dbname != "" && strstr($dbname,"tbdb") == FALSE) {
		 echo "<h3>Sorry, due to security concerns," .
                      " you may only view databases with names" .
		      " containing the substring \"tbdb\";<br>" .
                      " Defaulting to \"tbdb\".</h3>\n";
		 $dbname = "tbdb";
                }

		if ($action == "view") {
		  if ($dbname == "") {
		    listDatabases();
		  } else {
                    if ($tablename == "") {
		      listTables();
		    } else {
		      viewData();	  
	            }
		  }
	        } else if( $action == "addData" )
			manageData( "add" );
		else if( $action == "addData_submit" )
			manageData_submit( "add" );
		else if( $action == "editData" )
			manageData( "edit" );
		else if( $action == "editData_submit" )
			manageData_submit( "edit" );
		else if( $action == "deleteData" )
			deleteData();
		else undefinedcommand();

		mysql_close( $mysqlHandle);
		footer_html();
	}
}

?>














