/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

import java.util.*;
import java.lang.reflect.*;
import javax.sql.*;
import java.net.*;
import javax.net.ssl.*;
import java.io.*;
import java.security.*;

public class MoteLogger {

    public static int debug;

    private File classDir;
    private String[] classNames;
    private File aclDir;
    private String logTag;
    private String pid;
    private String eid;
    private String[] motes;
    private LinkedList packetQueue;
    private String database;
    private String dbUser;
    private Hashtable classes;
    private boolean connectReadOnly = false;
    private MoteThread moteThreads[];
    private static FileWriter logWriter;

    public static void main(String args[]) {
	//
	// Usage: java MoteLogger
	//   -M
	//   -m moteXXX,moteXXX,... (a list of /var/log/tiplogs/moteXXX.acl
	//        files we should look in to get tip connect info)
	//   -c directory (of class files)
	//   -p pid,eid
	//   -i id tag for this logging session
	//   - 

	parseArgsAndRun(args);
    }

    public static void globalDebug(int level,String msg) {
	if (MoteLogger.debug >= level) {
	    if (logWriter != null) {
		try {
		    logWriter.write("DEBUG: " + msg + "\n");
		    logWriter.flush();
		}
		catch (Exception e) {
		    System.err.println("Problem writing to logfile!!!");
		    e.printStackTrace();
		}
	    }
	    else {
		System.out.println("DEBUG: " + msg);
	    }
	}
    }

    public static void globalError(String msg) {
	if (logWriter != null) {
	    try {
		logWriter.write("ERROR: " + msg + "\n");
		logWriter.flush();
	    }
	    catch (Exception e) {
		System.err.println("Problem writing to logfile!!!");
		e.printStackTrace();
	    }
	}
	else {
	    System.err.println("ERROR: " + msg);
	}
    }

    public static void parseArgsAndRun(String args[]) {
	File classDir = null;
	String[] classNames = null;
	String tag = null;
	String pid = null;
	String eid = null;
	File aclDir = new File("/var/log/tiplogs");
	String[] motes = null;
	boolean connectReadOnly = false;
	File logFile = null;
	logWriter = null;
	String database = "test";
	String dbUser = "root";

	int i;
	for (i = 0; i < args.length; ++i) {
	    if (args[i].equals("-l")) {
		if (++i < args.length && !args[i].startsWith("-")) {
		    try {
			logFile = new File(args[i]);

			logWriter = new FileWriter(logFile);
		    }
		    catch (Exception e) {
			System.err.println("Could not open log file for writing, exiting.");
			e.printStackTrace();
			System.exit(-9);
		    }
		}
		else {
		    System.err.println("option '-l' must have an argument!");
		    usage();
		}
	    }
	    else if (args[i].equals("-r")) {
		connectReadOnly = true;
	    }
	    else if (args[i].equals("-D")) {
		if (++i < args.length && !args[i].startsWith("-")) {
		    database = args[i];
		}
		else {
		    System.err.println("option '-D' must have an argument!");
		    usage();
		}
	    }
            else if (args[i].equals("-U")) {
                if (++i < args.length && !args[i].startsWith("-")) {
                    dbUser = args[i];
                }
                else {
                    System.err.println("option '-U' must have an argument!");
                    usage();
                }
            }
	    else if (args[i].equals("-C")) {
		if (++i < args.length && !args[i].startsWith("-")) {
		    classDir = new File(args[i]);
		}
		else {
		    System.err.println("option '-C' must have an argument!");
		    usage();
		}
	    }
	    else if (args[i].equals("-c")) {
		if (++i < args.length && !args[i].startsWith("-")) {
		    classNames = args[i].split(",");
		}
		else {
		    System.err.println("option -c must have an argument!");
		    usage();
		}
	    }
	    else if (args[i].equals("-i")) {
		if (++i < args.length && !args[i].startsWith("-")) {
		    tag = args[i];
		}
		else {
		    System.err.println("option '-i' must have an argument!");
		    usage();
		}
	    }
	    else if (args[i].equals("-p")) {
		if (++i < args.length && !args[i].startsWith("-")) {
		    String[] sa = args[i].split(",");
		    if (sa.length == 2) {
			pid = sa[0];
			eid = sa[1];
		    }
		    else {
			System.err.println("argument to '-p' must be of the " +
					   "form 'pid,eid'!");
			usage();
		    }
		}
		else {
		    System.err.println("option '-p' must have an argument!");
		    usage();
		}
	    }
	    else if (args[i].equals("-M")) {
		if (++i < args.length && !args[i].startsWith("-")) {
		    aclDir = new File(args[i]);
		}
		else {
		    System.err.println("option '-i' must have an argument!");
		    usage();
		}
	    }
	    else if (args[i].equals("-d")) {
		++MoteLogger.debug;
	    }
	    else if (args[i].startsWith("-")) {
		System.err.println("Improper option '" + args[i] + "'!");
		usage();
	    }
	    else {
		// encountered a mote vnode name...
		break;
	    }
	}

	// parse the remainder args as mote vnode names
	motes = new String[args.length - i];
	int j = 0;
	while (i < args.length) {
	    motes[j] = args[i];
	    ++i;
	}

	//this.motes = motes;

	if (tag == null || tag.length() == 0) {
	    // gen a random num
	    tag = "" + (int)(Math.random()*9999);
	}

	// startup
	MoteLogger ml = new MoteLogger(classDir,classNames,aclDir,motes,
				       pid,eid,tag,connectReadOnly,
				       database,dbUser);
	ml.run();
    }

    public static void usage() {
	String usage = "" + 
	    "Usage: java MoteLogger -cipMd \n" +
	    "Options:\n" +
	    "\t-C <classdir>  Directory containing packet-matching " + 
	    "classfiles \n" +
	    "\t-c <classfile list>  Comma-separated list of fully-qualified " +
	    "classnames. \n" + 
	    "\t-i <idtag>  Alphanumeric tag for this logging set \n" +
	    "\t-p pid,eid \n" + 
	    //"\t-m <vname,vname,...>  (list of vnames present in acl dir) \n"+
	    "\t-M <acldir>  ACL directory \n" +
	    "\t-d..d  Stay in foreground and debug at level specified \n" +
	    "\t-r  Connect to capture read-only \n" + 
	    "\t-D <database>  Localhost database to drop packets in \n" + 
	    "\t-U <db user>  User to access the db as \n" + 
	    "";

	System.err.println(usage);
	System.exit(-1);
    }

    public MoteLogger(File classDir,String[] classNames,
		      File aclDir,String[] motes,String pid,
		      String eid,String tag,boolean connectReadOnly,
		      String database,String dbUser) {
	this.classDir = classDir;
	this.classNames = classNames;
	this.aclDir = aclDir;
	this.pid = pid;
	this.eid = eid;
	this.logTag = tag;
	this.motes = motes;
	this.classes = new Hashtable();
	this.connectReadOnly = connectReadOnly;
	this.moteThreads = null;
	this.database = database;
	this.dbUser = dbUser;
    }

    private void debug(int level,String msg) {
	MoteLogger.globalDebug(level,"MoteLogger: " + msg);
    }

    private void error(String msg) {
	MoteLogger.globalError("MoteLogger: " + msg);
    }
    
    // I know, not a thread, but who cares
    public void run() {
	if (motes == null || motes.length == 0) {
	    // gotta read any acl files we find in the specified dir:
	    File[] aclFiles = aclDir.listFiles( new FilenameFilter() {
		    public boolean accept(File dir, String name) {
			if (name != null && name.endsWith(".acl")) {
			    return true;
			}
			else {
			    return false;
			}
		    }
		});

	    if (aclFiles != null) {
		motes = new String[aclFiles.length];

		for (int i = 0; i < aclFiles.length; ++i) {
		    System.out.println(aclFiles[i].getName());
		    String[] sa = aclFiles[i].getName().split("\\.");
		    motes[i] = sa[0];
		}
	    }
	}

	if (motes == null || motes.length == 0) {
	    error("could not find any mote ACL files; exiting.");
	    System.exit(0);
	}

	// load classfiles and create Class objs so we can do instance objs
	// on incoming packet data
	// first, have to discover the classfiles, if they weren't specified.
	File[] classFiles = null;
	if (this.classNames == null || classNames.length == 0) {
	    // try to read the classFile directory and use the XXX.class
	    // names as the classes -- will fail if classes are in package.
	    classFiles = classDir.listFiles( new FilenameFilter() {
		    public boolean accept(File dir, String name) {
			if (name != null && name.endsWith(".class")) {
			    return true;
			}
			else {
			    return false;
			}
		    }
		});

	    if (classFiles != null) {
		this.classNames = new String[classFiles.length];

		for (int i = 0; i < classFiles.length; ++i) {
		    String[] sa = classFiles[i].getName().split("\\.class");
		    this.classNames[i] = sa[0];
		}
	    }
	}

	if (classFiles == null || classFiles.length == 0) {
	    error("could not find any classfiles; exiting.");
	    System.exit(0);
	}

	// second, actually load them.
	for (int i = 0; i < classNames.length; ++i) {
	    try {
		Class c = Class.forName(classNames[i]);

		// this call does the class.forName, redundant i know.
		classes.put(c,new SQLGenerator(classNames[i],
					       null,
					       this.logTag));
	    }
	    catch (Exception e) {
		error("problem loading class " + classNames[i] + 
				   ":");
		e.printStackTrace();
	    }
	}

	// get capture keys
	Hashtable acls = new Hashtable();
	for (int i = 0; i < motes.length; ++i) {
	    try {
		ElabACL ma = ElabACL.readACL(aclDir,motes[i]);
		acls.put(motes[i],ma);
	    }
	    catch (Exception e) {
		error("problem reading ACL for " + motes[i]);
		e.printStackTrace();
	    }
	}

	// setup queue
	packetQueue = new LinkedList();
	
	// connect to the database
	java.sql.Connection conn = null;
	try {
	    Class.forName("com.mysql.jdbc.Driver");
	    conn = java.sql.DriverManager.getConnection("jdbc:mysql:" + 
							"//localhost/" + 
							database + 
							"?user=" +
							dbUser);
	}
	catch (Exception e) {
	    error("FATAL -- couldn't connect to the database: " +
			       e.getMessage());
	    e.printStackTrace();

	    System.exit(-2);
	}

	// spawn connection threads
	moteThreads = new MoteThread[acls.size()];
	int k = 0;
	for (Enumeration e1 = acls.keys(); e1.hasMoreElements(); ++k) {
	    String vNN = (String)e1.nextElement();
	    ElabACL acl = (ElabACL)acls.get(vNN);
	    MoteThread mt = new MoteThread(vNN,acl,packetQueue,
					   connectReadOnly);
	    mt.start();
	    moteThreads[k] = mt;
	}

	// now, process the packet queue forever.
	while (true) {
	    LogPacket lp = null;

	    synchronized(packetQueue) {
		debug(1,"Waiting for packets...");
		
		while(packetQueue.size() == 0) {
		    try {
			packetQueue.wait(8);
		    }
		    catch (Exception e) {
			e.printStackTrace();
		    }

		    // check if all moteThreads are still cookin'
		    boolean allDead = true;
		    for (int j = 0; j < moteThreads.length; ++j) {
			if (moteThreads[j] != null 
			    && moteThreads[j].getRunStatus()) {
			    allDead = false;
			    break;
			}
		    }
		    
		    if (allDead && packetQueue.size() == 0) {
			error("all MoteThreads dead; exiting.");
			System.exit(-9);
		    }

		}

		
		    
		
		if (packetQueue.size() != 0) {
		    lp = (LogPacket)packetQueue.removeLast();
		}
	    }

	    debug(2,"Dumping packet to db:");
	    
	    // dump it out to database:
	    // this is the real work:

	    if (lp != null) {
		try {
		    // first match the packet:
		    SQLGenerator sq = null;
		    boolean foundMatch = false;
		    Object msgObj = null;
		    
		    for (Enumeration e1 = classes.keys(); 
			 !foundMatch && e1.hasMoreElements(); ) {
			
			Class cc = (Class)e1.nextElement();
			sq = (SQLGenerator)classes.get(cc);
			
			// Need to create a BaseTOSMsg
			// and extract the 'type' field... which is the am type
			// ... and then match this with the amType() method of
			// the invoked class.
			
			// XXX: note that this must be changed depending on the
			// architecture of the motes we're connecting to, in 
			// order to decode using the correct host byte order 
			// assumption.
			// i.e., net.tinyos.message.telos.*
			// 
			if (true) {
			    net.tinyos.message.avrmote.BaseTOSMsg btm = 
				new net.tinyos.message.avrmote.BaseTOSMsg(lp.getData());

			    debug(1,"lp data size = " + lp.getData().length);

			    // XXX: this is some nasty hack!!! the BaseTosMsg
			    // doesn't seem to want to let me call get_data.
			    // However, I want to remove the basic
			    // AM fields at the front... which are 5 bytes
			    // (at least for mica2!)
			    int PREBYTE_REMOVE = 5;
			    int realLen = lp.getData().length-PREBYTE_REMOVE;
			    byte[] realdata = new byte[realLen];
			    System.arraycopy(lp.getData(),PREBYTE_REMOVE,
					     realdata,0,realLen);

			    if (btm.get_type() == sq.getAMType()) {
				// class match
				// invoke the byte[] data constructor on the 
				// data from the basetosmsg:
				Constructor ccc = cc.getConstructor( new Class[] {
					byte[].class } );
				msgObj = ccc.newInstance( new Object[] {
					//btm.get_data() } );
				        //lp.getData() } );
					realdata } );
				
				lp.setMsgObject(msgObj);
				// ready to log!!!
				
				foundMatch = true;
				debug(2,"found packet type match with type " +
				      btm.get_type());
			    }
			    else {
				debug(2,"btm.amType=" + btm.get_type() + 
				      "; sq.getAMType=" + sq.getAMType());
			    }
			}
			
		    }
		    
		    // do the db insert:
		    if (foundMatch) {
			sq.storeMessage(lp,conn);
		    }
		    else {
			error("NO match for packet!");
		    }
		    
		    // that's all, folks!
		    
		}
		catch (Exception e) {
		    error("error while logging packet: ");
		    e.printStackTrace();
		}
	    }
	    else {
		error("got a null packet from the PacketReader!");
	    }
	}

	    
    }

    class MoteThread extends Thread {
	private String vNodeName;
	private ElabACL acl;
	private LinkedList q;
	private Socket sock;
	private SSLSocket ssock;
	private boolean runStatus;

	public MoteThread(String vNodeName,ElabACL acl,
			  LinkedList packetQueue,boolean connectReadOnly) {
	    this.vNodeName = vNodeName;
	    this.acl = acl;
	    this.q = packetQueue;
	    this.ssock = null;
	    this.runStatus = true;
	}
	
	private void debug(int level,String msg) {
	    MoteLogger.globalDebug(level,
				   "MoteThread (" + vNodeName + "): " + msg);
	}
	
	private void error(String msg) {
	    MoteLogger.globalError("MoteThread (" + vNodeName + "): " + msg);
	}

	public boolean getRunStatus() {
	    return runStatus;
	}
	
	public void run() {
	    sock = null;

	    try {
		// isn't java just wonderful
		sock = new Socket(acl.getHost(),acl.getPort());
		debug(1,"connected to " + 
		      sock.getInetAddress() + ":" + sock.getPort());
	    }
	    catch (Exception e) {
		error("couldn't connect to " + acl.getHost() +
		      ":" + acl.getPort());
		e.printStackTrace();
		this.runStatus = false;
		return;
	    }

	    int retval = 0;
	    String msg = null;

	    if (!connectReadOnly) {
		
		// authenticate to the full read/write port on capture.
		// send the 4-byte key len in host order :-(
		int keylen = acl.getKeyLen();
		byte[] len = new byte[4];
		len[3] = (byte)(0xff & (keylen >> 24));
		len[2] = (byte)(0xff & (keylen >> 16));
		len[1] = (byte)(0xff & (keylen >> 8));
		len[0] = (byte)(0xff & keylen);
		
		// now send the key (and pad its length out to 256 bytes)
		// aw heck, lets not pad it, who cares... capture won't...
		// oops, it does
		String key = acl.getKey();
		byte[] authBytes = new byte[260];
		byte[] oldbkey = null;
		try {
		    oldbkey = key.getBytes("ISO-8859-1");
		}
		catch (Exception e) {
		    error("could not convert key bytes!");
		    e.printStackTrace();
		    this.runStatus = false;
		    return;
		}
		System.arraycopy(len,0,authBytes,0,len.length);
		System.arraycopy(oldbkey,0,authBytes,4,oldbkey.length);
		for (int i = oldbkey.length + 4; i < 260; ++i) {
		    authBytes[i] = 0;
		}
		//bkey[0] = 0;
		//bkey[255] = 0;
		
		debug(4,"sending keylen " + keylen + " and key " + 
		      key + " bytearraylen = "+key.getBytes().length +
		      " \n  realkey = '"+new String(authBytes)+"'");
		
		try {
		    OutputStream out = sock.getOutputStream();
		    out.write(authBytes);
		    //out.write(bkey);
		    
		    InputStream in = sock.getInputStream();
		    byte[] rba = new byte[4];
		    retval = -1;
		    
		    // make sure we get them all...
		    retval = in.read(rba);
		    while (retval < 4) {
			retval = in.read(rba,retval,rba.length-retval);
		    }
		    
		    retval = rba[0];
		    retval |= rba[1] << 8;
		    retval |= rba[2] << 16;
		    retval |= rba[3] << 24;

		    
		}
		catch (Exception e) {
		    error("problem with plaintext auth for " + 
			  acl.getVnodeName());
		    e.printStackTrace();
		    this.runStatus = false;
		    return;
		}
	    }
	    else {
		String key = "MOTEREADER";
		int keylen = 10;
		byte[] len = new byte[4];
		len[3] = (byte)(0xff & (keylen >> 24));
		len[2] = (byte)(0xff & (keylen >> 16));
		len[1] = (byte)(0xff & (keylen >> 8));
		len[0] = (byte)(0xff & keylen);
		
		byte[] authBytes = new byte[260];
		byte[] oldbkey = null;
		try {
		    oldbkey = key.getBytes("ISO-8859-1");
		}
		catch (Exception e) {
		    error("could not convert key bytes!");
		    e.printStackTrace();
		    this.runStatus = false;
		    return;
		}
		System.arraycopy(len,0,authBytes,0,len.length);
		System.arraycopy(oldbkey,0,authBytes,4,oldbkey.length);
		for (int i = oldbkey.length + 4; i < 260; ++i) {
		    authBytes[i] = 0;
		}

		debug(4,"sending keylen " + keylen + " and key " + 
		      key + " bytearraylen = "+key.getBytes().length +
		      " \n  realkey = '"+new String(authBytes)+"'");
		
		try {
		    OutputStream out = sock.getOutputStream();
		    out.write(authBytes);
		    out.flush();
		}
		catch (Exception e) {
		    error("problem sending ssl auth hint");
		    e.printStackTrace();
		    this.runStatus = false;
		    return;
		}


		/* now send teh real key */
		key = acl.getKey();
		keylen = acl.getKeyLen();
		len = new byte[4];
		len[3] = (byte)(0xff & (keylen >> 24));
		len[2] = (byte)(0xff & (keylen >> 16));
		len[1] = (byte)(0xff & (keylen >> 8));
		len[0] = (byte)(0xff & keylen);

		try {
		    oldbkey = key.getBytes("ISO-8859-1");
		}
		catch (Exception e) {
		    error("could not convert key bytes!");
		    e.printStackTrace();
		    this.runStatus = false;
		    return;
		}
		System.arraycopy(len,0,authBytes,0,len.length);
		System.arraycopy(oldbkey,0,authBytes,4,oldbkey.length);
		for (int i = oldbkey.length + 4; i < 260; ++i) {
		    authBytes[i] = 0;
		}

		debug(4,"sending keylen " + keylen + " and key " + 
		      key + " bytearraylen = "+key.getBytes().length +
		      " \n  realkey = '"+new String(authBytes)+"'");
		
		try {
		    OutputStream out = sock.getOutputStream();
		    out.write(authBytes);
		    out.flush();

		    InputStream in = sock.getInputStream();
		    byte[] rba = new byte[4];
		    retval = -1;
		    
		    // make sure we get them all...
		    retval = in.read(rba);
		    while (retval < 4) {
			retval = in.read(rba,retval,rba.length-retval);
		    }
		    
		    retval = rba[0];
		    retval |= rba[1] << 8;
		    retval |= rba[2] << 16;
		    retval |= rba[3] << 24;
		}
		catch (Exception e) {
		    error("problem sending ssl auth hint");
		    e.printStackTrace();
		    this.runStatus = false;
		    return;
		}
		    
//  		// now start up ssl
//  		SecureRandom sr = new SecureRandom();
//  		sr.nextInt();
		
//  		try {
//  		    SSLContext sc = SSLContext.getInstance("SSL");
//  		    sc.init(null,new TrustManager[] { new ElabTrustManager() },sr);
		
//  		    SSLSocketFactory sslf = (SSLSocketFactory)sc.getSocketFactory();
//  		    ssock = (SSLSocket)sslf.createSocket(sock,acl.getHost(),
//  							 acl.getPort(),false);
//  		    //ssock.setUseClientMode(true);
		    
//  		    String[] suites = ssock.getEnabledCipherSuites();
//  		    for (int i = 0; i < suites.length; ++i) {
//  			debug(4,"ssl suites["+i+"] = "+suites[i]);
//  		    }
//  		    debug(4,"ssl needs client auth: "+ssock.getNeedClientAuth());
		    
//  		    ssock.startHandshake();
		    
//  		    System.exit(0);
//  		}
//  		catch (Exception e) {
//  		    error("problem while starting up ssl");
//  		    e.printStackTrace();
//  		}
		
		

	    }

	    msg = null;
	    if (retval == 0) {
		msg = "success.";
	    }
	    else if (retval == 1) {
		msg = "capture busy.";
	    }
	    else {
		msg = "failure (" + retval +").";
	    }
	    
	    debug(2,"result of authentication: " + msg);
	    
	    if (retval != 0) {
		error("could not connect to capture!");
		this.runStatus = false;
		return;
	    }
	    
//  	    catch (Exception e) {
//  		error("problem authenticating for " + 
//  		      acl.getVnodeName());
//  		e.printStackTrace();
//  		return;
//  	    }

	    // read packets forever:
	    LogPacket lp = null;
	    PacketReader pr = null;

	    try {
		pr = new PacketReader(vNodeName,sock.getInputStream());
	    }
	    catch (Exception e) {
		e.printStackTrace();
		this.runStatus = false;
		return;
	    }

	    while (true) {
		lp = null;
		try {
		    lp = pr.readPacket();
		    synchronized(q) {
			q.addFirst(lp);
			q.notifyAll();
		    }

		}
		catch (Exception e) {
		    error("problem while reading from " + 
			  acl.getVnodeName());
		    e.printStackTrace();
		}
	    }
	}
    }

}



