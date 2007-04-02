
public interface DataSourceListener {
    
    public void fetchingDatasetItem(Dataset ds,int ithItem,int totalItems);
    
    public void datasetFetched(Dataset ds);
    
    public void datasetRemoved(Dataset ds);
    
    public void newDatasetSource(String srcName);
    
}
