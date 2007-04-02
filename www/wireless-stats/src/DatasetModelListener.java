
public interface DatasetModelListener {
    
    public void newDataset(String dataset,DatasetModel model);
    
    public void datasetUnloaded(String dataset,DatasetModel model);
    
    public void currentDatasetChanged(String dataset,DatasetModel model);
    
}
