
import java.util.*;
import java.net.URL;

public class EmulabURLDataSource implements DataSource {
    
    // String uid = this.getParameter("uid");
    // String auth = this.getParameter("auth");
    // String mapurl = this.getParameter("mapurl");
    // String dataurl = this.getParameter("dataurl");
    // String positurl = this.getParameter("positurl");
    // String datasetStr = this.getParameter("datasets");
    
    
    private URL base;
    private String stdargs;
    private String srcarg;
    private String maparg;
    private String dataarg;
    private String positarg;
    
    Vector dsListeners;
    
    public EmulabURLDataSource(DataCache cache,URL codebase,String stdargs) {
        this.base = codebase;
        this.stdargs = stdargs;
        
        this.dsListeners = new Vector();
    }
    
    public void refreshSources() {
        
    }
    
    public String[] getSources() {
        return null;
    }

    public String[] getSourceSetList(String src) {
        return null;
    }

    public Dataset fetchData(String Source,String set) {
        return null;
    }
    
    public void addDataSourceListener(DataSourceListener dsl) {
        if (!dsListeners.contains(dsl)) {
            dsListeners.add(dsl);
        }
    }
    
    public void removeDataSourceListener(DataSourceListener dsl) {
        if (dsListeners.contains(dsl)) {
            dsListeners.remove(dsl);
        }
    }
    
    void notifyDataSourceListeners() {
        
    }
    
}
