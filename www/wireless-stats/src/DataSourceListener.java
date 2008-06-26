/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006-2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

public interface DataSourceListener {
    
    public void fetchingDatasetItem(Dataset ds,int ithItem,int totalItems);
    
    public void datasetFetched(Dataset ds);
    
    public void datasetRemoved(Dataset ds);
    
    public void newDatasetSource(String srcName);
    
}
