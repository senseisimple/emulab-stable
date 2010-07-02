/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006-2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

public interface DatasetModelListener {
    
    public void newDataset(String dataset,DatasetModel model);
    
    public void datasetUnloaded(String dataset,DatasetModel model);
    
    public void currentDatasetChanged(String dataset,DatasetModel model);
    
}
