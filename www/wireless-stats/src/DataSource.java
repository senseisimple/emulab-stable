
import java.util.Vector;

public interface DataSource {
    
    /**
     * Get all possible "sources."  A source is a grouping of measurement 
     * "sets."
     */
    public String[] getSources();

    /**
     * Get all known sets for the specified source.
     */
    public String[] getSourceSetList(String src);
    
    /**
     * Refresh sources so that getSources() always returns the most recent list.
     */
    public void refreshSources();
    
    /**
     * Download the dataset.
     */
    public Dataset fetchData(String Source,String set);
    
    
}
