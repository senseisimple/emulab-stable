
import net.tinyos.message.*;
import java.sql.*;
import java.util.*;
import java.lang.reflect.*;
import java.io.ByteArrayInputStream;

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
    private String[][] arrayViewCreates;

    // data insert preparedStatement strings
    private String[] inserts;
    // first dimension i is how to perform inserts[i]; 
    // each element in the arrayList is a String[] with the following data:
    //   [ getMethodName, returnType, SQLType ]
    private ArrayList[] insertInfo;

    private String className;
    private Class classInfo;
    private String msgSpec[];
    private String tag;
    private int amType;

    private boolean tablesCreated;

    private SpecData spec;


    public SQLGenerator(String name,String[] spec,String tag) 
	throws ClassNotFoundException, Exception {
	
	// need to parse the methods via reflection:
	this.className = name;
	this.msgSpec = spec;
	this.tag = tag;

	Class c = Class.forName(className);
	this.classInfo = c;
	this.spec = null;

	if (msgSpec != null && msgSpec.length > 0) {
	    // see if we can parse it...
	    try {
		this.spec = NCCSpecParser.parseSpec(msgSpec);
	    }
	    catch (Exception e) {
		this.spec = null;
		error("problem parsing msg spec for class " + name);
		e.printStackTrace();
	    }
	}

	//debug(3,"sqlgen constructor for '"+name+"'");

	// the following are things we want to do whether using specs or not:

	// grab the am type first!
	Object msgObj = c.newInstance();
	Method am = c.getMethod("amType",null);
	Integer amt = (Integer)am.invoke(msgObj,null);
	this.amType = amt.intValue();
	
	// now figure out the methods.  Basically, we figure out which
	// get_* methods correspond to which arrays by finding the longest
	// common substring following the get_ part of the string.
	Method[] m = c.getMethods();
	Vector flatFieldMethods = new Vector();
	Vector arrayFieldMethods = new Vector();

	for (int i = 0; i < m.length; ++i) {
	    String ms = m[i].getName();
	    String rts = m[i].getReturnType().getName();
	    
	    if (ms.startsWith("get_")) {
		if (rts.startsWith("[[") && this.spec == null) {
		    // MULTIDIMENSIONAL ARRAYS NOT SUPPORTED YET
		    // only with specs to help us :-)
		    // XXX
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

	// to facilitate VIEW creation, we keep a global temp table number:
	int tempTableNum = 10;


	//if (spec != null) {
	    // first go through the FieldInfo tree and see how many tables
	    // we need:
	//    ;
	//}
	//else {
	if (true) {
	    // simple, n-table layout with two views; one master table.
	    int numTables = 1 + arrayFields.length;
	    int tableIdx = 1;
	    int arrayViewIdx = 0;

	    tableCreates = new String[numTables];
	    inserts = new String[numTables];
	    insertInfo = new ArrayList[numTables];
	    
	    // one for our view; one for motelab's
	    viewCreates = new String[2];

	    // main table
	    String tableName = className + "__data_" + tag;
	    String parentTableName = tableName; 
	    
	    tableCreates[0] = "" + 
		"CREATE TABLE IF NOT EXISTS " + tableName + " " +
		"(" + "id INT PRIMARY KEY AUTO_INCREMENT, " + 
		"time DATETIME, " + 
		"amType INT, " + 
		"srcMote VARCHAR(32)";
	    inserts[0] = "insert into " + tableName + 
		" values (NULL,?," + this.amType + ",?";

	    // XXX: for views, we don't have IF NOT EXISTS syntax;
	    // in fact, the create will silently fail if we don't specify
	    // OR REPLACE; if we do, then the view is replaced.
	    // frankly, I don't feel that we can replace a view without
	    // replacing the tables it depends on... thus, we need some
	    // code to check if a table that we are going to use again
	    // still matches the packet fields!!!

	    // do the motelab view:
	    viewCreates[0] = "" +
		"CREATE VIEW " + className + "_motelab_" + tag +
		" as SELECT * FROM " + tableName;

	    // start our better one!
	    viewCreates[1] = "" + 
		"CREATE VIEW " + className + "_" + tag +
		" as SELECT id,time,amType,srcMote";
	    // we have to build up a separate string of joins, unfortunately.
	    // this string will get appended to viewCreates[1] later.
	    String joinStr = " ";

	    // setup the other views:
	    // we don't know the dimensions of each array, so we simply 
	    // create a 2-d array.  The second dimension is a list of all
	    // the field-specific views we need for the hierarchy.
	    // we must always have one view named 
	    // className + "__" + fieldName + "__" + v1 + "_" + tag
	    // that has fields ppid and fieldName... so that we can
	    // left join the main table to all array tables on the ppid=id
	    // condition
	    arrayViewCreates = new String[arrayFields.length][];

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
		    String mnSansGet = mn.substring(4,mn.length());

		    Method signMethod = null;
		    try {
			signMethod = c.getMethod("isSigned_" + mnSansGet,null);
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

			// add this field to the main view create (the non-
			// motelab one)
			viewCreates[1] += "," + mnSansGet;
		    }
		    else {
			// now, IF we have an array, we create a separate 
			// table.  we only do this for arrays!  We let the 
			// structs get flattened out for now; we solve the 
			// hierarchy problem as best we can via views.
			
			// get number dims:
			String fieldName = ma[i].getName().replaceAll("get_","");
			Method dm = this.classInfo.getMethod("numDimensions_" +
							     fieldName,
							     null);

			int nDims = ((Integer)dm.invoke(null,null)).intValue();

			// we add the table now:
			String aTableName = className + "__" + mnSansGet + 
			    "__data_" + tag;
			tableCreates[tableIdx] = "" +
			    "CREATE TABLE IF NOT EXISTS " + aTableName + 
			    " (" + "id INT PRIMARY KEY AUTO_INCREMENT," +
			    "ppid INT NOT NULL";

			inserts[tableIdx] = "" + 
			    "insert into " + aTableName + " values (NULL," + 
			    "?";
			
			for (int k = 0; k < nDims; ++k) {
			    tableCreates[tableIdx] += ",idx" + (k+1) + 
				" INT NOT NULL";
			    inserts[tableIdx] += ",?";
			}

			tableCreates[tableIdx] += "," + mnSansGet + " " + 
			    internalArraySQLType + ")";
			inserts[tableIdx] += ",?)";

			insertInfo[tableIdx] = new ArrayList();
			//   [ getMethodName, returnType, SQLType ]
			insertInfo[tableIdx].add( new String[] {
			    "ppid",null,null } );

			for (int k = 0; k < nDims; ++k) {
			    insertInfo[tableIdx].add( new String[] {
				    "idx"+(k+1),null,null } );
			}

			insertInfo[tableIdx].add( new String[] {
			    ma[i].getName(),
			    ma[i].getReturnType().getName(),
			    internalArraySQLType } );


			// add to the viewCreates:
			// the top-level view for this array:
			String topLevelArrayViewName = "" + className + "__" + 
			    fieldName + "__" + "v1" + "_" + tag;

			viewCreates[1] += "," + topLevelArrayViewName + 
			    "." + fieldName;

			// add to the join string:
			joinStr += " left join " + topLevelArrayViewName + 
			    " on " +
			    "id="+topLevelArrayViewName+"." + "ppid";

			// now add a complete view hierarchy for this
			// array:
			arrayViewCreates[arrayViewIdx] = new String[nDims];

			String baseViewName = className + "__" + fieldName +
			    "__" + "v";
			String baseViewNameEnd = "_" + tag;

			// for the first view, we want to pull right out
			// of the main table; after that, we must pull 
			// from the most recently created VIEW
			String currentTableToSelectFromInView = aTableName;

			for (int k = nDims; k > 0; --k) {
			    String fullViewName = baseViewName + "" + k +
				baseViewNameEnd;
			    // this is the fields we select from the table:
			    String str1 = "ppid";
			    // this is for an order by inside the group_concat
			    String str2 = "ppid";
			    // this is for the group by stmt for the 
			    // group_concat; it is the field-reverse of
			    // str1
			    String str3;
			    if (nDims-1 == 0) {
				str3 = "";
			    }
			    else {
				str3 = "idx" + (nDims-1);
			    }

			    int k2;
			    for (k2 = 1; k2 < k; ++k2) {
				str1 += ",idx" + k2;
				str2 += ",idx" + k2;

				if (str3 == null) {
				    str3 = "idx" + (nDims-k2);
				}
				else {
				    str3 += ",idx" + (nDims-k2);
				}
			    }

			    // finish off str2; it has to be one idx longer
			    // to get the ordering correct in the group_concat
			    str2 += ",idx" + k2;

			    // finish off str3 with ppid:
			    if (str3.equals("")) {
				str3 = "ppid";
			    }
			    else {
				str3 += ",ppid";
			    }

			    // now create view at this level:
			    String tmp = "CREATE VIEW " + fullViewName + 
				" as SELECT " + str1 + ",concat('['," + 
				"group_concat(" + fieldName + " order by " +
				str2 + " separator ','),']') as " + fieldName +
				" from " + currentTableToSelectFromInView +
				" group by " + str3 + " order by " + str1;

			    // we create them backwards because the 
			    // highest-dim view must be created first
			    arrayViewCreates[arrayViewIdx][nDims-k] = tmp;

			    // change the `parent' table:
			    currentTableToSelectFromInView = fullViewName;
			}

			++arrayViewIdx;
			// done with viewCreates

			++tableIdx;
			
			// also add the field as a blob column, just in case
			tableCreates[0] += ", " + mnSansGet + " " + 
			    getSQLTypeStr(ma[i],signMethod,false);
			// add to the insert & insertInfo
			inserts[0] += ",?";
			insertInfo[0].add( new String[] {
			    ma[i].getName(),
			    ma[i].getReturnType().getName(),
			    getSQLTypeStr(ma[i],signMethod,false) } );
		    }

		}
	    }

	    // finish off the table create and insert:
	    tableCreates[0] += ")";
	    //tableCreates[1] += ")";
	    inserts[0] += ")";

	    viewCreates[1] += " FROM " + tableName + " " + joinStr;

	}

	for (int k = 0; k < tableCreates.length; ++k) {
	    debug(3,"tableCreates["+k+"] = '" + tableCreates[k] + "'.");
	    debug(3,"inserts["+k+"] = '" + inserts[k] + "'.");
	}

	for (int k = 0; k < viewCreates.length; ++k) {
	    debug(3,"viewCreates["+k+"] = '" + viewCreates[k] + "'.");
	}

	if (arrayViewCreates != null) {
	    for (int k = 0; k < arrayViewCreates.length; ++k) {
		if (arrayViewCreates[k] != null) {
		    for (int j = 0; j < arrayViewCreates[k].length; ++j) {
			debug(3,
			      "arrayViewCreates["+k+"]["+j+"] = '" + 
			      arrayViewCreates[k][j] + "'.");
		    }
		}
	    }
	}

	tablesCreated = false;

    }

    private void debug(int level,String msg) {
	MoteLogger.globalDebug(level,"SQLGenerator (" + className + "): "+msg);
    }

    private void error(String msg) {
	MoteLogger.globalError("SQLGenerator (" + className + "): " + msg);
    }

    public String[] getTableCreates() {
	return tableCreates;
    }

    public String[] getViewCreates() {
	return viewCreates;
    }

//     private void setPreparedStmtVal(PreparedStatement ps,int idx,
// 				    Object msgObject,Method m,String[] info) {
// 	if (info[1] != null && 
// 	    (info[1].equals("short") 
// 	     || info[1].equals("S"))) {
	    
// 	    int retval = 0;
	    
// 	    Short s = (Short)m.invoke(msgObject,null);
	    
// 	    ps.setInt(j+1,(int)s.shortValue());
	    
// 	}
// 	else if (info[1] != null && 
// 		 (info[1].equals("I") 
// 		  || info[1].equals("int"))) {
	    
// 	}
//     }

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

		// create array views:
		// we have to do these before the viewCreates because the
		// viewCreates[1] depend on the arrayViewCreates
		for (int i = 0; i < arrayViewCreates.length; ++i) {
		    for (int j = 0; j < arrayViewCreates[i].length; ++j) {
			s.execute(arrayViewCreates[i][j]);
		    }
		}

		// create views too:
		for (int i = 0; i < viewCreates.length; ++i) {
		    s.execute(viewCreates[i]);
		}

		tablesCreated = true;
	    }
	    catch (SQLException e) {
		error("problem while creating tables!");
		e.printStackTrace();
	    }
	}

	// run the inserts:
	int errors = 0;
	// the first table is always the master parent table...
	int ppid = -1;

	for (int i = 0; i < inserts.length; ++i) {
	    ArrayList aInfo = insertInfo[i];


	    // each element in the arrayList is a String[] with the 
	    // following data:
	    //   [ getMethodName, returnType, SQLType ]
	    //private ArrayList[] insertInfo;


	    // setup the prepared stmt:
	    try {
		PreparedStatement ps = conn.prepareStatement(inserts[i]);

		// if not an array insert (i.e., is main table insert):
		if (i == 0) {
		    
		    for (int j = 0; j < aInfo.size(); ++j) {
			String[] info = (String[])aInfo.get(j);
			
			// now perform the ps.set op...
			if (info[0].equals("date")) {
			    // insert the date manually
			    // note that we use j+1 since preparedStatement 
			    // vars start at index 1
			    ps.setTimestamp(j+1,(new java.sql.Timestamp(lp.getTimeStamp().getTime())));
			}
			else if (info[0].equals("srcMote")) {
			    ps.setString(j+1,lp.getSrcVnodeName());
			}
			else if (info[0].equals("ppid")) {
			    ps.setInt(j+1,ppid);
			}
// 			else if (info[0].equals("idx")) {
// 			    //ps.setInt();
// 			}
			else {
			    // "normal" field :-)
			    Object msgObject = lp.getMsgObject();
			    Method m = this.classInfo.getMethod(info[0],
								null);

			    fillPreparedStmt(ps,j+1,m,null,
					     msgObject,lp.getData());
			    //setPreparedStatementVal(ps,j+1,msgObject,m,info);

			}
		    }

		    // dump the stmt...
		    ps.executeUpdate();

		    // now, if we were dumping into the main table (idx=0),
		    // grab the id, because it's the global packet id for
		    // this msg type.
		    if (i == 0) {
			// should be in the result set... ?
			
			// this only works with mysql 5.x and up, with
			// a recent version of the mysql jdbc drivers.
			// if this ever has to be ported to anything else
			// anytime soon, you'll need to try another technique
			// -- there are a couple.
			// i.e., select last_insert_id()
			ResultSet rs = ps.getGeneratedKeys();

			if (rs.next()) {
			    ppid = rs.getInt(1);
			}
			else {
			    error("Could not get ppid from " +
				  "main table insert -- cannot " +
				  "add anything to array tables!");
			}

			rs.close();
		    }
		}
		else {
		    // XXXX we have an array insert... need to do 
		    // multiple inserts on this prepared stmt, using ppid
		    // as the backref:

		    // set it now; will never change:
		    ps.setInt(1,ppid);

		    // so, we have to loop over all the dims, setting 
		    // each idx val and any other array values each time.

		    // get the dims
		    // of course, we rely on our consistent naming here ;)
		    int dimCount = 0;
		    int valCount = 0;
		    
		    // we start with idx 1 because the first col is ppid.
		    // also want the value column count because if more than
		    // one value, we MUST figure out the right method calls.
		    for (int j = 1; j < aInfo.size(); ++j) {
			String[] info = (String[])aInfo.get(j);

			if (info[0].startsWith("idx")) {
			    ++dimCount;
			}
			else if (i > 0) {
			    ++valCount;
			}
		    }

		    // assume that ALL valObjects are arrays, possibly 
		    // multidimensional arrays, WITH THE SAME INTERNAL 
		    // DIMENSIONS!!!

		    Object msgObject = lp.getMsgObject();
		    
		    Method[] valElmMethods = new Method[valCount];
		    int dimSizes[] = null;
		    // now, we have to get the arrays for the vals in this
		    // table.
		    int lpc = 0;
		    for (int j = 1+dimCount; j < (1+dimCount+valCount); ++j) {
			String[] info = (String[])aInfo.get(j);
			
			// grab the dimensions
			String fieldName = info[0].replaceAll("get_","");
			Method dm = this.classInfo.getMethod("numDimensions_"+
							     fieldName,
							     null);
			int nDims = ((Integer)dm.invoke(null,null)).intValue();
			dimSizes = new int[nDims];

			Method dsm = this.classInfo.getMethod("numElements_"+
							      fieldName,
							      new Class[] {
								  Integer.TYPE
							      });
			for (int k = 0; k < nDims; ++k) {
			    dimSizes[k] = ((Integer)dsm.invoke(null,
							       new Object[] {
								   new Integer(k)
							       })).intValue();
			}

			// now grab the method to use for getting individual
			// elements:
			Class argTypes[] = new Class[nDims];
			for (int k = 0; k < argTypes.length; ++k) {
			    argTypes[k] = Integer.TYPE;
			}

			Method gm = this.classInfo.getMethod("getElement_" +
							     fieldName,
							     argTypes);
			valElmMethods[lpc++] = gm;
		    }

		    // 

		    // ok, now we have the data and the data dimensions, now
		    // do a whole mess of prepared statements:

		    // now, loop over all values and do a bunch of inserts:
		    int currentDims[] = new int[dimCount];
		    for (int p = 0; p < dimCount; ++p) {
			currentDims[p] = 0;
			// necessary so that we can increment the most deeply-
			// nested idx right away in the while loop below:
			if (p == dimCount - 1) {
			    currentDims[p] = -1;
			}
		    }
		    
		    boolean keepGoing = true;
		    while (keepGoing) {
			// here's what we do: we loop over each dimension, 
			// checking to see if currentDims[i] == dimSizes[i];
			// if so, bump currentDims[i] to 0 and do 
			// currentDims[i-1]++ .  Now, if we get to the point
			// where currentDims[0] == dimSizes[0], we're done.
			// heh, in case it's not obvious; this technique is 
			// SLOW!!!
			for (int p = dimSizes.length-1; p > -1; --p) {
			    // have to do this right away so the following 
			    // checks don't fail:
			    ++(currentDims[p]);

			    if (p == 0 && currentDims[0] == dimSizes[0]) {
				keepGoing = false;
				break;
			    }
			    else if (currentDims[p] == dimSizes[p]) {
				currentDims[p] = 0;
				++(currentDims[p-1]);
			    }
			    //else {
			    //++(currentDims[p]);
			    //}
			}

			if (!keepGoing) {
			    continue;
			}

			// prepare the index args:
			Object idxArgs[] = new Object[currentDims.length];
			for (int p = 0; p < currentDims.length; ++p) {
			    idxArgs[p] = new Integer(currentDims[p]);
			    ps.setInt(2+p,currentDims[p]);
			}

			// now, for each value method, dump the element 
			// specified by currentDims into the prepared stmt;
			// then execute it!
			for (int p = 0; p < valElmMethods.length; ++p) {
			    fillPreparedStmt(ps, // the prepared stmt
					     1+dimCount+1+p, // the ps index
					     valElmMethods[p], // the method to
					                       // get val from
					     idxArgs, // the args to the method
					     msgObject,
					     lp.getData()
					     );
			}

			ps.executeUpdate();
		    }
			
		}
	    }
	    catch (Exception e) {
		error("problem with insert for msg of type " + 
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

    // the point of this is to 
    private void fillPreparedStmt(PreparedStatement ps,int idx,
				  Method m,Object[] methodArgs,
				  Object msgObject,byte[] msgBytes) 
	throws Exception {
	
	String retType = m.getReturnType().getName();

	if (retType.equals("S") || retType.equals("short")) {

	    //debug(0,"method = '"+m+"'; retType = '"+retType+"'");
	    //debug(0,"methodArgs = '"+methodArgs+"'");

	    //debug(0,"methodArgs = ");
	    //for (int i = 0; i < methodArgs.length; ++i) {
	    //debug(0,""+((Integer)methodArgs[i]).intValue());
	    //}

	    Short s = (Short)m.invoke(msgObject,methodArgs);

	    ps.setShort(idx,s.shortValue());
	}
	else if (retType.equals("I") || retType.equals("int")) {
	    Integer i = (Integer)m.invoke(msgObject,methodArgs);
	    ps.setInt(idx,i.intValue());
	}
	else if (retType.equals("J") || retType.equals("long")) {
	    Long l = (Long)m.invoke(msgObject,methodArgs);
	    ps.setLong(idx,l.longValue());
	}
	else if (retType.equals("Z") || retType.equals("boolean")) {
	    Boolean b = (Boolean)m.invoke(msgObject,methodArgs);
	    ps.setBoolean(idx,b.booleanValue());
	}
	else if (retType.equals("B") || retType.equals("byte")) {
	    Byte b = (Byte)m.invoke(msgObject,methodArgs);
	    ps.setByte(idx,b.byteValue());
	}
	else if (retType.equals("C") || retType.equals("char")) {
	    Character c = (Character)m.invoke(msgObject,methodArgs);
	    ps.setString(idx,new String(""+c.charValue()));
	}
	else if (retType.equals("D") || retType.equals("double")) {
	    Double d = (Double)m.invoke(msgObject,methodArgs);
	    ps.setDouble(idx,d.doubleValue());
	}
	else if (retType.equals("F") || retType.equals("float")) {
	    Float f = (Float)m.invoke(msgObject,methodArgs);
	    ps.setFloat(idx,f.floatValue());
	}
	else if (retType.startsWith("[")) {
	    // XXX: need to dump any arbitrary array to a single-dimension
	    // array of bytes.
	    //ps.setNull(idx,java.sql.Types.BLOB);
	    //return;
	    // XXX: done below!
	    
	    // for this, we need to get the offset and length info for this
	    // array, then grab the original bytes out of the raw msg:
	    // we need three methods: 
	    //   int numDimensions_NAME    and
	    //   int offset_NAME(indices)  and 
	    //   int totalSize_NAME

	    String fieldName = m.getName().replaceAll("get_","");
	    Method dm = this.classInfo.getMethod("numDimensions_" + fieldName,
						 null);

	    // set it up so that we can find the offset_NAME method...
	    Integer tmpI = new Integer(0);

	    int nDims = ((Integer)dm.invoke(null,null)).intValue();
	    Class omArgTypes[] = new Class[nDims];
	    Object omArgs[] = new Object[nDims];

	    for (int i = 0; i < nDims; ++i) {
		omArgTypes[i] = Integer.TYPE;
		omArgs[i] = tmpI;
	    }

	    Method om = this.classInfo.getMethod("offset_" + fieldName,
						 omArgTypes);

	    Method tsm = this.classInfo.getMethod("totalSize_" + fieldName,
						  null);

	    // ok, now call everybody and get the values!
	    int offset = ((Integer)om.invoke(null,omArgs)).intValue();
	    int length = ((Integer)tsm.invoke(null,null)).intValue();

	    // now copy it into an array from the msg raw data array:
	    byte psData[] = new byte[length];
	    System.arraycopy(msgBytes,offset,psData,0,length);

	    debug(0,"doing the setBytes for "+className+", length"+length);

	    // now do the ps set:
	    ps.setBytes(idx,psData);
	    //psData = new byte[] { 0,1,2,3,4,5,6,7,8,9 };
	    //ByteArrayInputStream bais = new ByteArrayInputStream(psData);
	    //ps.setBinaryStream(idx,bais,10);
	    //ps.setString(idx,new String(psData));
	}
	else {
	    throw new Exception("fillPreparedStatement: unknown return type");
	}
    }
    
    private String getSQLTypeStr(Method m,Method signMethod,
				 boolean getArrayType) 
	throws Exception {
	
	String rt = m.getReturnType().getName();
	boolean isSigned = false;
	
	if (signMethod != null) {
	    try {
		Object msgObj = classInfo.newInstance();
		isSigned = ((Boolean)signMethod.invoke(msgObj,null)).booleanValue();
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
		// strip the leading ['s
 		rt = rt.replaceAll("\\[","");
 	    }
	}

	if (rt.equals("Z") || rt.equals("boolean")) {
	    // boolean
	    retval = "BOOLEAN";
	}
	else if (rt.equals("B") || rt.equals("byte")) {
	    // byte
	    retval = "TINYINT";
	    
	    if (!isSigned) {
		retval += " UNSIGNED";
	    }
	}
	else if (rt.equals("C") || rt.equals("char")) {
	    // char
	    retval = "CHAR(1)";
	}
	else if (rt.equals("D") || rt.equals("double")) {
	    // double
	    retval = "DOUBLE";
	}
	else if (rt.equals("F") || rt.equals("float")) {
	    // float
	    retval = "FLOAT";
	}
	else if (rt.equals("I") || rt.equals("int")) {
	    // int 
	    retval = "INT";
	    
	    if (!isSigned) {
		retval += " UNSIGNED";
	    }
	}
	else if (rt.equals("J") || rt.equals("long")) {
	    // long
	    retval = "BIGINT";
	    
	    if (!isSigned) {
		retval += " UNSIGNED";
	    }
	}
	else if (rt.equals("S") || rt.equals("short")) {
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

	    error("unrecognized return type '" + rt +"'!");
	}
	
	return retval;
    }


    public int getAMType() {
	return this.amType;
    }

}

