
import net.tinyos.message.*;
import java.sql.*;
import java.util.*;
import java.lang.reflect.*;

public class SQLGenerator {

    //private Message m;

    // every message should have these fields...
    private Method[] tosMsgFields;
    // for now, even structures will be completely flattened out
    private Method[] simpleFields;
    // arrays are the only thing we will distinguish...
    // and heaven help us if there's a multidimensional one!
    // there could be more than one, hence the hash
    // keys:
    //private Hashtable arrayFields;
    private Method[] arrayFields;

    // table creation statements
    private String[] tableCreates;
    // view creates... reduces number of tables and data dup.
    private String[] viewCreates;

    // data insert preparedStatement strings
    private String[] inserts;
    // first dimension i is how to perform inserts[i]; 
    // each element in the arrayList is a String[] with the following data:
    //   [ getMethodName, returnType, SQLType ]
    private ArrayList[] insertInfo;

    private String className;
    private String msgSpec;
    private String tag;
    private int amType;

    private boolean tablesCreated;

    public SQLGenerator(String name,String[] spec,String tag) 
	throws ClassNotFoundException, Exception {
	
	// need to parse the methods via reflection:
	this.className = name;
	this.msgSpec = spec;
	this.tag = tag;

	Class c = Class.forName(className);

	// grab the am type first!
	Object msgObj = c.newInstance();
	Method am = c.getMethod("amType",null);
	Integer amt = (Integer)am.invoke(msgObj,null);
	this.amType = amt.intValue();
	
	// now figure out the methods.  Basically, we figure out which
	// get_* methods correspond to which arrays by finding the longest
	// common substring following the get_ part of the string.
	Methods[] m = c.getMethods();
	Vector flatFieldMethods = new Vector();
	Vector arrayFieldMethods = new Vector();

	for (int i = 0; i < m.length; ++i) {
	    String ms = m[i].getName();
	    String rts = m[i].getReturnType().getName();
	    
	    if (ms.startsWith("get_")) {
		if (rts.startsWith("[[")) {
		    // MULTIDIMENSIONAL ARRAYS NOT SUPPORTED YET
		    throw new Exception("multidimensional arrays unsupported");
		}
		else if (rts.startsWith("[")) {
		    arrayFieldMethods.add(m[i]);
		}
		else {
		    flatFieldMethods.add(m[i]);
		}
	    }
	}

	simpleFields = new Method[flatFieldMethods.size()];
	arrayFields = new Method[arrayFieldMethods.size()];

	int elm = 0;
	for (Enumeration e1 = flatFieldMethods.elements(); 
	     e1.hasMoreElements(); ) {
	    simpleFields[elm++] = (Method)e1.nextElement();
	}

	elm = 0;
	for (Enumeration e1 = arrayFieldMethods.elements(); 
	     e1.hasMoreElements(); ) {
	    arrayFields[elm++] = (Method)e1.nextElement();
	}
		    
	// so for now, we're just going to have to accept the reality of 
	// flattened everything.  The best we can do is create the main table,
	// with blobs for array data, and a secondary table, in which we change
	// the blobs to string representation of the array.

	// not true -- by asking for a bit more MIG output, we can do 
	// infinitely better -- because the whole type definition is right
	// there, along with variable names.  Just have to read the MIG src,
	// write a parser... then generate much, much more complex sql.

	// for now, just a few tables -- a main data table,
	// then several tables for array info.
	if (spec != null) {
	    // do the complex stuff...
	    ;
	}
	else {
	    // simple, one-table layout with two views.
	    int numTables = 1 + arrayFields.length;
	    int tableIndex = 1;

	    tableCreates = new String[numTables];
	    inserts = new String[numTables];
	    insertInfo = new ArrayList[numTables];
	    
	    viewCreates = new String[1];

	    // main table
	    String tableName = className + "__data_" + tag;
	    parentTableName = tableName; 
	    
	    tableCreates[0] = "" + 
		"CREATE TABLE " + tableName + " " +
		"(" + "id INT PRIMARY KEY AUTO_INCREMENT, " + 
		"time DATETIME, " + 
		"amType INT, " + 
		"srcMote VARCHAR(32)";
	    inserts[0] = "insert into " + tableName + 
		" values (NULL,?," + this.amType + ",?";

	    insertInfo[0] = new ArrayList();
	    insertInfo[0].add( new String[] { "date",null,null } );
	    // now get this statically.
	    //insertInfo[0].add( new String[] { "amType","I","INT" } );
	    insertInfo[0].add( new String[] { "srcMote",null,null } );
	    
	    // now the msg-specific stuff:
	    // we do this again here because we'd prefer the correct order...
	    Method ma[] = c.getMethods();
	    for (int i = 0; i < ma.length; ++i) {
		String mn = ma[i].getName();
		
		if (mn.startsWith("get_")) {
		    String mnSansGet = mn.subString(4,mnSansGet.length());

		    Method signMethod = null;
		    try {
			signMethod = c.getMethod("isSigned_" + mnSansGet);
		    }
		    catch (Exception e) {
			// no big deal
			;
		    }

		    boolean isArr = false;
		    String internalArraySQLType = "";
		    if (ma[i].getReturnType().getName().startsWith("[")) {
			isArr = true;
			internalArraySQLType = 
			    getSQLTypeStr(ma[i],signMethod,true);
		    }

		    if (!isArr) {
			tableCreates[0] += ", " + mnSansGet + " " + 
			    getSQLTypeStr(ma[i],signMethod,false);

			// add to the insert & insertInfo
			inserts[0] += ",?";
			insertInfo[0].add( new String[] {
			    ma[i].getName(),
			    ma[i].getReturnType().getName(),
			    getSQLTypeStr(ma[i],signMethod,false) } );
		    }
		    else {
			// now, IF we have an array, we create a separate 
			// table.  we only do this for arrays!  We let the 
			// structs get flattened out for now; we solve the 
			// hierarchy problem as best we can via views.
			
			// we add the table now:
			String aTableName = mnSansGet + "__data_" + tag;
			tableCreates[tableIdx] = "" +
			    "CREATE TABLE " + aTableName + " (" + 
			    "ppid INT PRIMARY KEY NOT NULL, " +
			    "idx INT NOT NULL, " +
			    "" + mnSansGet + " " + internalArraySQLType + 
			    ")";

			inserts[tableIdx] = "" + 
			    "insert into " + aTableName + "values (" + 
			    "?,?,?)";
			
			insertInfo[tableIdx] = new ArrayList();
			//   [ getMethodName, returnType, SQLType ]
			insertInfo[tableIdx].add( new String[] {
			    "ppid",null,null } );

			insertInfo[tableIdx].add( new String[] {
			    "idx",null,null } );
			    
			insertInfo[tableIdx].add( new String[] {
			    ma[i].getName(),
			    ma[i].getReturnType().getName(),
			    internalArraySQLType } );

			++tableIdx;

			// also add the field as a blob column, just in case
			tableCreates[0] += ", " + mnSansGet + " " + 
			    getSQLTypeStr(ma[i],signMethod,false);
			;
		    }

		}
	    }

	    // finish off the table create and insert:
	    tableCreates[0] += ")";
	    inserts[0] += ")";
	}

	tablesCreated = false;

    }

    public String[] getTableCreates() {
	return tableCreates;
    }

    public String[] getViewCreates() {
	return viewCreates;
    }

    // 
    public boolean storeMessage(LogPacket lp,Connection conn) 
	throws SQLException {
	
	// check if the tables are there; if not, create them.
	if (!tablesCreated) {
	    try {
		Statement s = conn.createStatement();
		
		for (int i = 0; i < tableCreates.length; ++i) {
		    s.execute(tableCreates[i]);
		}

		tablesCreated = true;
	    }
	    catch (SQLException e) {
		System.err.println("Problem while creating tables for " +
				   this.className + "!");
		e.printStackTrace();
	    }
	}

	// run the inserts:
	int errors = 0;
	for (int i = 0; i < inserts.length; ++i) {
	    ArrayList aInfo = insertInfo[i];

	    // setup the prepared stmt:
	    try {
		PreparedStatement ps = conn.prepareStatement(inserts[i]);
		int ppid = -1;

		for (int j = 0; j < aInfo.length(); ++j) {
		    String[] info = (String[])aInfo.get(j);

		    // now perform the ps.set op...
		    if (info[0].equals("date")) {
			// insert the date manually
			// note that we use j+1 since preparedStatement vars
			// start at index 1
			ps.setDate(j+1,lp.getDate());
		    }
		    else if (info[0].equals("srcMote")) {
			ps.setString(j+1,lp.getSrcMote());
		    }
		    else if (info[0].equals("ppid")) {
			ps.setInt(j+1,ppid);
		    }
		    else if (info[0].equals("idx")) {
			;
		    }
		    else {
			// "normal" field :-)

		    }

		    // now, if we were dumping into the main table (idx=0),
		    // grab the id, because it's the global packet id for
		    // this msg type.
		    if (j == 0) {
			// should be in the result set... ?
			
			// this only works with mysql 5.x and up, with
			// a recent version of the mysql jdbc drivers.
			// if this ever has to be ported to anything else
			// anytime soon, you'll need to try another technique
			// -- there are a couple.
			// i.e., select last_insert_id()
			ResultSet rs = stmt.getGeneratedKeys();

			if (rs.next()) {
			    ppid = rs.getInt(1);
			}
			else {
			    System.err.println("Could not get ppid from " +
					       "main table insert -- cannot " +
					       "add anything to array tables!");
			}

			rs.close();
		    }
		}
	    }
	    catch (Exception e) {
		System.err.println("Problem with insert for msg of type " + 
				   this.amType + ":");
		e.printStackTrace();

		++errors;

		//
		// XXX:
		// 
		// this is a PROBLEM -- we possibly have already inserted 
		// some packet data -- so we need to rollback those changes.
		// problem is, we're not using transactions yet!

	    }
	}

	if (errors > 0) {
	    return false;
	}
	else {
	    return true;
	}
	
    }

    private String getSQLTypeStr(Method m,Method signMethod,
				 boolean getArrayType) 
	throws Exception {
	
	String rt = ma.getReturnType().getName();
	boolean isSigned = false;
	
	if (signMethod != null) {
	    try {
		isSigned = ((Boolean)isSignedMethod.invoke()).getBoolValue();
	    }
	    catch (Exception e) {
		// do nothing; there just wasn't sign info...
		;
	    }
	}
	
	String retval = null;

	if (rt.startsWith("[")) {
	    // array, just do blob type for now.
 	    if (!getArrayType) {
		retval = "BLOB";
		return retval;
 	    }
 	    else {
		// strip the leading [
 		rt = rt.substring(1,rt.length());
 	    }
	}

	if (rt.equals("Z")) {
	    // boolean
	    retval = "BOOLEAN";
	}
	else if (rt.equals("B")) {
	    // byte
	    retval = "TINYINT";
	    
	    if (!isSigned) {
		retval += " UNSIGNED";
	    }
	}
	else if (rt.equals("C")) {
	    // char
	    retval = "CHAR(1)";
	}
	else if (rt.equals("D")) {
	    // double
	    retval = "DOUBLE";
	}
	else if (rt.equals("F")) {
	    // float
	    retval = "FLOAT";
	}
	else if (rt.equals("I")) {
	    // int 
	    retval = "INT";
	    
	    if (!isSigned) {
		retval += " UNSIGNED";
	    }
	}
	else if (rt.equals("J")) {
	    // long
	    retval = "BIGINT";
	    
	    if (!isSigned) {
		retval += " UNSIGNED";
	    }
	}
	else if (rt.equals("S")) {
	    // short
	    retval = "SMALLINT";
	    
	    if (!isSigned) {
		retval += " UNSIGNED";
	    }
	}
	else {
	    // unknown data type, probably an object
	    // (or a multidim array, but we ruled this out earlier)
	    
	    // XXX: should throw exception or something
	}
	
	return retval;
    }


    public int getAMType() {
	return this.amType;
    }

}

