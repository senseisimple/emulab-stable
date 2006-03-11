
import java.util.*;
import java.lang.reflect.*;
import javax.sql.*;
import java.net.*;
import java.io.*;

public class MoteLogger {

    private int debug;
    private File classDir;
    private String[] classNames;
    private File aclDir;
    private String logTag;
    private String pid;
    private String eid;
    private String[] motes;
    private SynchQueue packetQueue;
    private String dbURL = "jdbc.DriverMysql";
    private String dbUser = "root";
    private String dbPass = "";
    private Hashtable classes;

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

    public static void parseArgsAndRun(String args[]) {
	File classDir = null;
	String[] classNames = null;
	String tag = null;
	String pid = null;
	String eid = null;
	File aclDir = new File("/var/log/tiplogs");
	int debug = 0;
	String[] motes = null;

	int i;
	for (i = 0; i < args.length; ++i) {
	    if (args[i].equals("-C")) {
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
		++debug;
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

	// startup
	MoteLogger ml = new MoteLogger(classDir,classNames,aclDir,motes,
				       pid,eid,tag,debug);
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
	    "";

	System.err.println(usage);
	System.exit(-1);
    }

    public MoteLogger(File classDir,String[] classNames,
		      File aclDir,String[] motes,String pid,
		      String eid,String tag,int debug) {
	this.classDir = classDir;
	this.classNames = classNames;
	this.aclDir = aclDir;
	this.pid = pid;
	this.eid = eid;
	this.logTag = tag;
	this.debug = debug;
	this.motes = motes;
	this.classes = new Hashtable();
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
	    System.out.println("Could not find any mote ACL files; exiting.");
	    System.exit(0);
	}

	// load classfiles and create Class objs so we can do instance objs
	// on incoming packet data
	// first, have to discover the classfiles, if they weren't specified.
	if (this.classNames == null || classNames.length == 0) {
	    // try to read the classFile directory and use the XXX.class
	    // names as the classes -- will fail if classes are in package.
	    File[] classFiles = classDir.listFiles( new FilenameFilter() {
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
	    System.out.println("Could not find any classfiles; exiting.");
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
		System.err.println("Problem loading class " + classNames[i] + 
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
		System.err.println("Problem reading ACL for " + motes[i]);
		e.printStackTrace();
	    }
	}

	// setup queue
	packetQueue = new SynchQueue();
	
	// connect to the database
	Connection conn = null;
	try {
	    Class.forName("com.mysql.jdbc.Driver");
	    conn = DriverManager.getConnection("jdbc:mysql:" + 
					       "//localhost/test?user=root");
	}
	catch (Exception e) {
	    System.err.println("FATAL -- couldn't connect to the database: " +
			       e.getMessage());
	    e.printStackTrace();

	    System.exit(-2);
	}

	// spawn connection threads
	for (Enumeration e1 = acls.keys(); e1.hasMoreElements(); ) {
	    String vNN = (String)e1.nextElement();
	    ElabACL acl = (ElabACL)acls.get(vNN);
	    (new MoteThread(acl,packetQueue)).start();
	}

	// now, process the packet queue forever.
	while (true) {
	    LogPacket lp = null;

	    synchronized(packetQueue) {
		while(packetQueue.peek() == null) {
		    try {
			packetQueue.wait();
		    }
		    catch (Exception e) {
			e.printStackTrace();
		    }
		}

		// once we get a packet...
		lp = packetQueue.queueRemove();
	    }

	    // dump it out to database:
	    // this is the real work:

	    try {
		// first match the packet:
		SQLGenerator sq = null;
		boolean foundMatch = false;

		for (Enumeration e1 = classes.keys(); 
		     !foundMatch && e1.hasMoreElements(); ) {

		    Class cc = (Class)e1.nextElement();
		    sq = (SQLGenerator)classes.get(cc);

		    // Need to create a BaseTOSMsg
		    // and extract the 'type' field... which is the am type
		    // ... and then match this with the amType() method of the
		    // invoked class.

		    // XXX: note that this must be changed depending on the
		    // architecture of the motes we're connecting to, in order
		    // to decode using the correct host byte order assumption.
		    // i.e., net.tinyos.message.telos.*
		    // 
		    if (true) {
			net.tinyos.message.avrmote.BaseTOSMsg btm = 
			    new net.tinyos.message.avrmote.BaseTOSMsg(lp.getData());
			if (lp.amType() == sq.getAMType()) {
			    // class match
			    // invoke the byte[] data constructor on the data
			    // from the basetosmsg:
			    Constructor ccc = cc.getConstructor( new Class[] {
				byte[].class } );
			    Object msgObj = ccc.newInstance( new Object[] {
				btm.get_data() } );

			    lp.setMsgObject(msgObj);
			    // ready to log!!!

			    foundMatch = true;
			}
		    }

		}
	       
		// do the db insert:
		sq.storeMessage(msgObj,conn);

		// that's all, folks!
		
	    }
	    catch (Exception e) {
		System.err.println("Error while logging packet: ");
		e.printStackTrace();
	    }
	}

	    
    }

    class MoteThread extends Thread {
	private String vNodeName;
	private ElabACL acl;
	private SynchQueue q;
	private Socket sock;

	public MoteLogThread(String vNodeName,ElabACL acl,
			     SynchQueue packetQueue) {
	    this.vNodeName = vNodeName;
	    this.acl = acl;
	    this.q = packetQueue;
	}

	public void run() {
	    sock = null;

	    try {
		// isn't java just wonderful
		sock = new Socket(acl.getHost(),acl.getPort());
		System.out.println("Connected to " + sock.getInetAddress() + 
				   ":" + sock.getPort() + " for " + 
				   acl.getVnodeName());
	    }
	    catch (Exception e) {
		System.err.println("Couldn't connect to " + acl.getHost() +
				   ":" + acl.getPort());
		e.printStackTrace();
		return;
	    }

	    // authenticate
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
		e.printStackTrace();
	    }
	    System.arraycopy(len,0,authBytes,0,len.length);
	    System.arraycopy(oldbkey,0,authBytes,4,oldbkey.length);
	    for (int i = oldbkey.length + 4; i < 260; ++i) {
		authBytes[i] = 0;
	    }
	    //bkey[0] = 0;
	    //bkey[255] = 0;

	    System.out.println("Sending keylen " + keylen + " and key " + 
			       key + " bytearraylen = "+key.getBytes().length +
			       " \n  realkey = '"+new String(authBytes)+"'");

	    try {
		OutputStream out = sock.getOutputStream();
		out.write(authBytes);
		//out.write(bkey);
		
		InputStream in = sock.getInputStream();
		byte[] rba = new byte[4];
		int retval = 0;
		
		// make sure we get them all...
		retval = in.read(rba);
		while (retval < 4) {
		    retval = in.read(rba,retval,rba.length-retval);
		}

		retval = rba[0];
		retval |= rba[1] << 8;
		retval |= rba[2] << 16;
		retval |= rba[3] << 24;

		String msg = null;
		if (retval == 0) {
		    msg = "success.";
		}
		else if (retval == 1) {
		    msg = "capture busy.";
		}
		else {
		    msg = "failure (" + retval +").";
		}

		System.out.println("Result of authentication: " + msg);

		if (retval != 0) {
		    return;
		}
	    }
	    catch (Exception e) {
		System.err.println("Problem authenticating for " + 
				   acl.getVnodeName());
		e.printStackTrace();
		return;
	    }

	    // read packets forever:
	    LogPacket lp = null;
	    PacketReader pr = null;

	    try {
		pr = new PacketReader(vNodeName,sock.getInputStream());
	    }
	    catch (Exception e) {
		e.printStackTrace();
		return;
	    }

	    while (true) {
		lp = null;
		try {
		    lp = pr.readPacket();
		    synchronized(packetQueue) {
			packetQueue.queueAdd(lp);
			packetQueue.notifyAll();
		    }

		}
		catch (Exception e) {
		    System.err.println("Problem while reading from " + 
				       acl.getVnodeName());
		    e.printStackTrace();
		}
	    }
	}
    }

}



