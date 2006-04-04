
import java.util.*;

public class FieldInfo {
    
    public static int TYPE_NONE = 0;
    public static int TYPE_INT = 1;
    public static int TYPE_UNSIGNED = 2;
    public static int TYPE_FLOAT = 3;
    public static int TYPE_DOUBLE = 4;
    public static int TYPE_LONG_DOUBLE = 5;
    public static int TYPE_STRUCT = 6;
    public static int TYPE_UNION = 7;
    public static int TYPE_UNKNOWN = 8;

    public static String[] typeNames = {
	"none",
	"signed",
	"unsigned",
	"float",
	"double",
	"long double",
	"struct",
	"union",
	"unknown",
    };
    
    String name;
    String fullName;
    FieldInfo parent;
    Vector children;
    int type;
    int[] dim;    

    public FieldInfo() {
	this.name = null;
	this.fullName = null;
	this.parent = null;
	this.children = new Vector();
	this.type = TYPE_NONE;
	this.dim = null;

    }

    public static int getTypeForSpecString(String specType) {
	if (specType.equals("U")) {
	    return FieldInfo.TYPE_UNSIGNED;
	}
	else if (specType.equals("I")) {
	    return FieldInfo.TYPE_INT;
	}
	else if (specType.equals("F")) {
	    return FieldInfo.TYPE_FLOAT;
	}
	else if (specType.equals("D")) {
	    return FieldInfo.TYPE_DOUBLE;
	}
	else if (specType.equals("LD")) {
	    return FieldInfo.TYPE_LONG_DOUBLE;
	}
	else if (specType.equals("AS")) {
	    return FieldInfo.TYPE_STRUCT;
	}
	else if (specType.equals("AU")) {
	    return FieldInfo.TYPE_UNION;
	}
	else {
	    return FieldInfo.TYPE_UNKNOWN;
	}
    }

    public static String getTypeName(int type) {
	if (type > TYPE_NONE && type < TYPE_UNKNOWN) {
	    return typeNames[type];
	}
	else {
	    return typeNames[TYPE_UNKNOWN];
	}
    }

    public void addChild(FieldInfo child) {
	children.add(child);
    }

    public Enumeration getChildren() {
	return children.elements();
    }

    public String toString() {
	String retval = "" + this.name + "("+ getTypeName(this.type) + ")";
	if (isArray()) {
	    for (int i = 0; i < dim.length; ++i) {
		retval += "[" + dim[i] + "]";
	    }
	}

	return retval;
    }

    public FieldInfo getParent() {
	return this.parent;
    }

    public boolean isBasic() {
	if (this.type == FieldInfo.TYPE_INT 
	    || this.type == FieldInfo.TYPE_UNSIGNED 
	    || this.type == FieldInfo.TYPE_FLOAT 
	    || this.type == FieldInfo.TYPE_DOUBLE 
	    || this.type == FieldInfo.TYPE_LONG_DOUBLE) {

	    return true;
	}
	else {
	    return false;
	}
    }

    public boolean isStruct() {
	if (this.type == FieldInfo.TYPE_STRUCT) {
	    return true;
	}
	else {
	    return false;
	}
    }

    public boolean isUnion() {
	if (this.type == FieldInfo.TYPE_UNION) {
	    return true;
	}
	else {
	    return false;
	}
    }

    public boolean isArray() {
	if (this.dim != null && this.dim.length > 0) {
	    return true;
	}
	else {
	    return false;
	}
    }

    public void print(int depth) {
	for (int i = 0; i < depth; ++i) {
	    System.out.print("  ");
	}
	System.out.println(this.toString());

	for (Enumeration e = this.getChildren(); e.hasMoreElements(); ) {
	    FieldInfo fi = (FieldInfo)e.nextElement();
	    fi.print(depth+1);
	}
    }

}
