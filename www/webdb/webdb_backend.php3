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

function sanitizeSQLName( $n ) {	
  // XXX fix.
  $r = strtr( $n, " '\"$!@#%^&()*\\[]{}<>,./?=+~`",
                  "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXX" );
  return $r;
}

function quoteForSQL( $l ) {
	return "'" . str_replace( "$", "\\$", addslashes( $l ) ) . "'";
//	return "'" . mysql_escape_string( $l ) . "'";  // not supported in current version. :(
}

function linkToViewDB( $text, $dbname ) {
  global $PHP_SELF;
  return "<a href=\"$PHP_SELF?dbname=$dbname\">$text</a>";
}

function linkToViewServer( $text ) {
  global $PHP_SELF;

  // use view action so we don't kick them into default database.
  return "<a href=\"$PHP_SELF?action=view\">$text</a>";
}

function linkToViewTable( $text, $dbname, $tablename ) {
  global $PHP_SELF;
  return "<a href=\"$PHP_SELF?dbname=$dbname&tablename=$tablename\">$text</a>";
}

function linkToViewTableExtra( $text, $extra, $dbname, $tablename ) {
  global $PHP_SELF;
  return "<a href=\"$PHP_SELF?dbname=$dbname&tablename=$tablename&$extra\">$text</a>";
}

function setError( $text ) {
  global $title_header, $body_header;

  $title_header = "Error";
  $body_header = "Error: $text";
}

function assertViewPermission() {
  global $readTablePerm, $tablename;

  if ($readTablePerm == 0) {
    setError( "Not allowed to view table '$tablename'" );
    return 0;
  } else {
    return 1;
  }
} 

function assertAddEditPermission() {
  global $addEditRowPerm, $tablename;

  if ($addEditRowPerm == 0) {
    setError( "Not allowed to add or edit rows in table '$tablename'" );
    return 0;
  } else {
    return 1;
  }
} 

function assertDeletePermission() {
  global $deleteRowPerm, $tablename;

  if ($deleteRowPerm == 0) {
    setError( "Not allowed to delete rows from table '$tablename'" );
    return 0;
  } else {
    return 1;
  }
} 

function encap( $c, $tag, $first ) {
  if ($first != "") {
    return "<$tag $first>$c</$tag>";
  } else {
    return "<$tag>$c</$tag>"; 
  }
}

function _form( $c ) { 
  global $PHP_SELF;
  return "<form action='$PHP_SELF' method=post>\n" . $c . "</form>\n";   
}
 
function _input_hidden( $var, $val ) {
  return "<input type=hidden name=$var value=\"$val\">\n";
}

function _input_text( $var, $val, $size ) {
  return "<input type=text name=$var value=\"$val\" size=$size>\n";
}

function _input_submit( $val ) {
  return "<input type=submit value=\"$val\" name=$val>\n";
}

function _table( $c ) { return encap( $c, "table", "cellspacing=0 cellpadding=5"); }  
function _tr( $c ) { return encap( $c, "tr", "align=left"); }
function _th( $c ) { return encap( $c, "th", "align=left"); }
function _th2( $c, $o ) { return encap( $c, "th", "align=left $o"); }
function _td( $c ) { return encap( $c, "td", ""); }
function _p( $c ) { return encap( $c, "p", ""); }
function _b( $c ) { return encap( $c, "b", ""); }
function _i( $c ) { return encap( $c, "i", ""); }
function _link( $c, $vars ) { 
  global $PHP_SELF;
  return encap( $c, "a", "href=\"$PHP_SELF?$vars\"" ); 
}
function _link2( $c, $vars, $o ) { 
  global $PHP_SELF;
  return encap( $c, "a", "href=\"$PHP_SELF?$vars\" $o" ); 
}

function listDatabases() {
	global $mysqlHandle, $body;
	
	$body .= _p( "Click to view tables in database. ") .
		 _p( _b("Note:") . 
                 _i("Due to security concerns, " .
                    "you may only view databases with names " .
		    "containing the substring \"tbdb\"." ));

        $table = _tr( _th("database name") );

	$pDB = mysql_list_dbs( $mysqlHandle );
	$num = mysql_num_rows( $pDB );
	for( $i = 0; $i < $num; $i++ ) {
		$dbname = mysql_dbname( $pDB, $i );
                $table .= _tr( _td( linkToViewDB( $dbname, $dbname ) ) );
	}

	$body .= _table( $table );
}


function listTables() {
	global $mysqlHandle, $dbname, $body;

	$body .= _p( "Click to view or edit table information." );

	$pResult = mysql_query( "SELECT * FROM comments WHERE column_name IS NULL" );
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
		setError( "$msg" ); 
		return;
	}

	$rows = mysql_num_rows( $pTable );

	$table = _tr(_th("table name") . _th("description"));

	for( $i = 0; $i < $rows; $i++ ) {
		$tablename = mysql_tablename( $pTable, $i );

		$row  = _td( _b( linkToViewTable( $tablename, $dbname, $tablename ) ) );
		if ($table_comment[$tablename] != "") {
	 	  $row .= _td( _i( $table_comment[$tablename] ) );
		}	
	
		$table .= _tr( $row );	
	}

	$body .= _table( $table );
}

function setTableDescription( $tablename, $description )
{
  global $mysqlhandle, $dbname;

  $a = quoteForSQL( $tablename );
  $c = quoteForSQL( $description );

  $q1 = "DELETE FROM comments WHERE table_name=$a AND column_name IS NULL";
  $q2 = "INSERT INTO comments VALUES ($a, NULL, $c)";

  mysql_query( $q1 );
  if ($description != "") {
    mysql_query( $q2 );
  }
}

function viewData() {
	global $mysqlHandle, $dbname, $tablename, $orderby, $body, $dliquid, $set, $cancel;
	global $description;
	global $addEditRowPerm, $deleteRowPerm;

        //// begin description crap

        if ($set != "" && $cancel == "") {
          setTableDescription( $tablename, $description );
        } 

        $query   = "SELECT * FROM comments WHERE column_name IS NULL AND table_name=" . quoteForSQL( $tablename );
	$pResult = mysql_query( $query );
        if ($pResult) {
	    $rows = mysql_num_rows( $pResult );
 	    if ($rows == 0) { 
              $description = "No description.";
              $realDescription = "";	 
            } else {
	      $row = mysql_fetch_array( $pResult );
              $description = $row["description"];
	      $realDescription = $description;
	    }

	    if ($dliquid == "") {
  	      $body .= _p( _b( "description:" ) . " " . _i( $description) .
	                   "[" . linkToViewTableExtra( "change", "dliquid=1", $dbname, $tablename) . "]" );
            } else {
		$body .= _form(
			   _b( "Description" ) .
			   _input_hidden( "dbname", $dbname ) . 
			   _input_hidden( "tablename", $tablename ) . 
			   _input_text( "description", $realDescription, 80 ) .
			   _input_submit( "set" ) .
			   _input_submit( "cancel" ) 
			);	  	
            }
      	} else {
	     $body .= _b( "no description table.\n");
	}


        //// end description crap

	$query = "SELECT * FROM $tablename";
        if($orderby != "") {
	  $query .= " ORDER BY $orderby";
        }

	$pResult = mysql_query( $query );
	
	if ($pResult == "") {
	  setError( mysql_error() );
	  return;
 	}

	$cols = mysql_num_fields( $pResult );

	$table = "";
        $row = "";

	for ( $i = 0; $i < $cols; $i++) {
	  $field = mysql_fetch_field( $pResult, $i );

          $name = $field->name;
	  $header = linkToViewTableExtra( $name, "orderby=$name", $dbname, $tablename );

	  if ( $field->primary_key == 1 ) {
            $header .= "*";
	  }

          $row .= _th( $header );
        }

	$row   .= _th2( "action", "colspan=2" );
	$table .= _tr( $row );

	while($rowArray = mysql_fetch_row( $pResult )) {
          $row = "";
	  $key = "";
	  for ($j = 0; $j < $cols; $j++) {
	    $data = $rowArray[ $j ];
	    $field = mysql_fetch_field( $pResult, $j );
	    if( $field->primary_key == 1 ) {
		$key .= "&" . "c_" . urlencode( $field->name ) . "=" . urlencode( $data );
	    } 
            $row .= _td( htmlspecialchars( $data ) );	       
          }

	  if ( $key == "" ) {
	    $row .= _td( "no primary key" );
          } else {
	    if ($addEditRowPerm == 1) {
		$row .= _td( "[" . _link( "Edit", "action=editData&dbname=$dbname&tablename=$tablename$key" ) . "]" );
	    }
	    if ($deleteRowPerm  == 1) {
		$row .= _td( "[" . _link2( "Delete", "action=deleteData&dbname=$dbname&tablename=$tablename$key",
			                   "onClick=\"return confirm('Delete Row?')\"" ) . "]" ); 
	    } 
	 }

          $table .= _tr( $row ); 	 
        }

	$body .= _table( $table );

	if ($addEditRowPerm == 1) {
		$body .= _p( "[" . _link( _b("Add data row"), "action=addData&dbname=$dbname&tablename=$tablename") . "]" );
	}
}
	

function manageData( $cmd ) {
	global $mysqlHandle, $dbname, $tablename;
	global $body, $body_header;

	if ( $cmd == "add") {
		$body_header = "Add row to " . $body_header;

 		$form  = _input_hidden( "action", "addData_submit" ); 	
        } else if ($cmd = "edit" ) {
		$body_header = "Edit row in " . $body_header;
         
		$pResult = mysql_list_fields( $dbname, $tablename );
		$num     = mysql_num_fields( $pResult );

		$key = "";
		for ($i = 0; $i < $num; $i++) {
			$field = mysql_Fetch_field( $pResult, $i );
			if ($field->primary_key == 1) {
				$key .= $field->name . "=" . quoteForSQL( $GLOBALS[ "c_" . $field->name ] ) . " AND ";
			}
		}

		$key = substr( $key, 0, strlen($key) - 4 ); // wipe away last " AND ".

		$query = "SELECT * FROM $tablename WHERE $key";

		$pResult = mysql_query( $query ); 
 		$data    = mysql_fetch_array( $pResult );

		$body .= _p( _b("Note:") . " " . _i("\"NULL\" in a field (no quotes) will set that field to NULL.") );
 		$form  = _input_hidden( "action", "editData_submit" ); 	
	} else {
		// this wont happen.
		seterror( "Unknown row management function \"$cmd\"" );
		return;
	}

	$form .= _input_hidden( "dbname", $dbname );
	$form .= _input_hidden( "tablename", $tablename );

	$table = _tr( _th("name") . _th("type") . _th("data") );

	$pResult = mysql_query("SHOW fields FROM $tablename" );
	$rows = mysql_num_rows( $pResult );
	$pResultFields = mysql_list_fields( $dbname, $tablename );	

	for( $i = 0; $i < $rows; $i++ ) {
		$field = mysql_fetch_array( $pResult );
		$fieldname  = $field["Field"];
		$fieldtype  = $field["Type"];

		if ($cmd == "edit") {
			$fieldvalueraw = $data[$i];
 		} else {
			$fieldvalueraw = "NULL";
		}

		$fieldvalue = htmlspecialchars( $fieldvalueraw ); 
	
		$len = mysql_field_len( $pResultFields, $i );

		$row = _td( $fieldname ) . _td( $fieldtype );
		
		$type = strtok( $fieldtype, " (,)\n" );
		if ($type == "enum" || $type == "set") {
			if ($type == "enum") {
  				$cell = "<select name=c_$fieldname>"; 	 
			} else {
				$cell = "<select name=c_$fieldname size=4 multiple>\n";
			}
			$cell .= strtok( "'" );
			while ($str = strtok( "'" ) ) {
				if ($fieldvalueraw == $str) {
					$cell .= "<option selected>$str\n";
				} else {	
					$cell .= "<option>$str\n";
				}
				strtok( "'" );
			}
			$cell .= "</select>";
		} else {
			if ($len < 40) {
				$cell = "<input type=text size=40 maxlength=$len name=c_$fieldname value=\"$fieldvalue\">";
			} else {
				$cell = "<textarea cols=40 rows=3 maxlength=$len name=c_$fieldname>$fieldvalue</textarea>";
			}
		}	
		$row .= _td( $cell );	

		$table .= _tr( $row );

	}

	
 	$form .= _table( $table );
	$form .= "<input type=submit value='$cmd data'>";

	$body .= _form( $form );
}

function deleteData() {
	global $mysqlHandle, $dbname, $tablename;

	$pResult = mysql_list_fields( $dbname, $tablename );

	if ($pResult == FALSE) {
		setError("Field listing failed.");
		return 0;
	}

	$num     = mysql_num_fields( $pResult );

	$query = "DELETE FROM $tablename WHERE ";

	while($field = mysql_fetch_field( $pResult )) {
		if ( $field->primary_key == 1 ) {
			$datum = $GLOBALS["c_".$field->name];
			if ($datum != "NULL") {
				$datum = quoteForSQL( $datum );
			}
			$query .= $field->name . "=$datum AND ";
		}
	} 

	$query = substr( $query, 0, strlen($key) - 4 ); // wipe away last " AND "		

	mysql_query( $query, $mysqlHandle );
	
	$errMsg = mysql_error();
	if ($errMsg != "") {
	  seterror( $errmsg );
	  return 0;
	}

//	echo "<pre>$query</pre>\n";
	return 1;
}

function manageData_submit( $cmd ) {
	global $mysqlHandle, $dbname, $tablename;

	if( $cmd == "add" ) {
	  $query = "INSERT INTO $tablename VALUES (";
        } else {
	  $query = "REPLACE INTO $tablename VALUES (";
  	}

	$pResult = mysql_list_fields( $dbname, $tablename );
	$num     = mysql_num_fields( $pResult );

	while($field = mysql_fetch_field( $pResult )) {
		$datum = $GLOBALS["c_".$field->name];
		if ($datum != "NULL") {
			$datum = quoteForSQL( $datum );
		}
		$query .= " $datum,";
	} 

	$query = substr( $query, 0, strlen($key) - 1 ) . " )"; // wipe away last "," and add ")"
	
	mysql_query( $query, $mysqlHandle );
	
	$errMsg = mysql_error();
	if ($errMsg != "") {
	  seterror( $errmsg );
	  return 0;
	}

//	echo "<pre>$query</pre>\n";
	return 1;
}


/////////////// MAIN

function webdb_backend_main() {
  global $body, $title_header, $body_header, $dbname, $action;
  global $readTablePerm, $addEditRowPerm, $deleteRowPerm;
  global $tablename, $orderby;
  global $mysqlHandle;

  $body = "";
  $title_header = "";
  $body_header  = "";

  if ($dbname == "") {
    // if there are no params, helpfully default them into tbdb. 
    if ($action == "") {
      $dbname = "tbdb";
    }
  } else { 
    $dbname = sanitizeSQLName( $dbname ); 
    if (strstr($dbname,"tbdb") == FALSE) {
	  $body .= "<h3>Sorry, due to security concerns," .
                   " you may only view databases with names" .
	  	   " containing the substring \"tbdb\";<br>" .
                   " Defaulting to \"tbdb\".</h3>\n";
	  $dbname = "tbdb";
    }
  } 

  if ($tablename != "") { $tablename = sanitizeSQLName( $tablename ); }
  if ($orderby   != "") { $orderby   = sanitizeSQLName( $orderby ); }

  $title_header = "MySQL";
  $body_header  = linkToViewServer("MySQL");
  if ($dbname) {
    $body_header  .= "::" . linkToViewDB( $dbname, $dbname );
    $title_header .= "::" . $dbname;
    if ($tablename) {
      $body_header  = "Table " . $body_header  . "::" . linkToViewTable( $tablename, $dbname, $tablename );
      $title_header = "Table " . $title_header . "::" . $tablename;
    } else {
      $body_header  = "Database " . $body_header;
      $title_header = "Database " . $title_header;
    }
  } else {
    $body_header  = "Server " . $body_header;
    $title_header = "Server " . $title_header;  
  }

  $HOSTNAME = "localhost";
  //echo "<!--";
  $mysqlHandle = mysql_pconnect( $HOSTNAME, $USERNAME, $PASSWORD );
  //echo "-->";
  if ($mysqlHandle == false) {
    seterror( "Couldn't connect to MySQL server." );
  } else {
    if ($dbname != "") {
      mysql_select_db( $dbname );
    }

    $readTablePerm  = 1;
    $addEditRowPerm = 0;
    $deleteRowPerm  = 0;

    // get permissions.
    $query   = "SELECT * FROM webdb_table_permissions WHERE table_name=" . quoteForSQL( $tablename );
    $pResult = mysql_query( $query ); 
    if ($pResult != FALSE && mysql_num_rows($pResult) > 0) {
      $field = mysql_fetch_array( $pResult );
      if ($field["allow_read"] != "1") { $readTablePerm = 0; }
      if ($field["allow_row_add_edit"] == "1") { $addEditRowPerm = 1; }
      if ($field["allow_row_delete"] == "1") { $deleteRowPerm = 1; } 
    } 

    if ($action == "" || $action == "view") {
      if ($dbname == "") {
        listDatabases(); 
      } else {
        if ($tablename == "") {
          if (assertViewPermission()) { listTables(); }
        } else {
          if (assertViewPermission()) { viewData(); }	  
        }
      }
    } else if( $action == "addData" ) {
      if (assertAddEditPermission()) { manageData( "add" ); }
    } else if( $action == "addData_submit" ) {
      if (assertAddEditPermission()) { 
  	  $success = manageData_submit( "add" );
	  if ($success == 1 && $readTablePerm != 0) {
  	    viewData();
	  }
      }
    } else if( $action == "editData" ) {
      if (assertAddEditPermission()) { manageData( "edit" ); }
    } else if( $action == "editData_submit" ) {
      if (assertAddEditPermission()) { 
	  $success = manageData_submit( "edit" );
	  if ($success == 1 && $readTablePerm != 0) {
	    viewData();
	  }
      }
    } else if( $action == "deleteData" ) {
      if (assertDeletePermission()) { 
	  $success = deleteData(); 
	  if ($success == 1 && $readTablePerm != 0) {
	    viewData();
	  }
      }
    } else {
      seterror("Undefined command '$action'");
    }
    mysql_close( $mysqlHandle);
  }
}








