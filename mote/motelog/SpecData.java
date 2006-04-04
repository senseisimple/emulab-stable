

public class SpecData {

    private FieldInfo root;
    private String specName;
    private int type;

    public SpecData(String specName,int specType,FieldInfo root) {
	this.root = root;
	this.specName = specName;
	this.type = specType;
    }

    public FieldInfo getRoot() {
	return this.root;
    }

    public String getSpecName() {
	return this.specName;
    }

    public int getType() {
	return this.type;
    }

}
