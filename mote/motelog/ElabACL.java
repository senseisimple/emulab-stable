
import java.io.*;

public class ElabACL {

    private String vnodeName;
    private String host;
    private short port;
    private int keylen;
    private String key;

    public static ElabACL readACL(File aclDir,String vnodeName) 
        throws FileNotFoundException {
	BufferedReader br = null;

	File f = new File(aclDir.toString() + File.separator + 
			  vnodeName + ".acl");
	if (!f.canRead()) {
	    throw new FileNotFoundException("could not read " + f.toString());
	}

	try {
	    br = new BufferedReader(new FileReader(f));

	    String line = null;
	    String h = null;
	    short p = 0;
	    int klen = 0;
	    String k = null;

	    while ((line = br.readLine()) != null) {
		if (line.startsWith("host:")) {
		    String[] sa = line.split(" ");
		    h = sa[1];
		}
		else if (line.startsWith("port:")) {
		    String[] sa = line.split(" ");
		    p = (short)Integer.parseInt(sa[1]);
		}
		else if (line.startsWith("keylen:")) {
		    String[] sa = line.split(" ");
		    klen = Integer.parseInt(sa[1]);
		}
		else if (line.startsWith("key:")) {
		    String[] sa = line.split(" ");
		    k = sa[1];
		}
		else {
		    ;
		}
	    }

	    return new ElabACL(vnodeName,h,p,klen,k);
	}
	catch (Exception e) {
	    System.err.println("Problem while reading ACL " + f.toString());
	    e.printStackTrace();
	}

	return null;
    }

    public ElabACL(String vname,String host,short port,int keylen,String key) {
	this.vnodeName = vname;
	this.host = host;
	this.port = port;
	this.keylen = keylen;
	this.key = key;
    }

    public String getVnodeName() {
	return this.vnodeName;
    }

    public String getHost() {
	return this.host;
    }

    public short getPort() {
	return this.port;
    }

    public int getKeyLen() {
	return this.keylen;
    }

    public String getKey() {
	return this.key;
    }

}
