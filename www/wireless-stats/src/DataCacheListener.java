/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006-2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

public interface DataCacheListener {
    
    public void dataRetrieved(DataCacheEvent evt);
    public void dataRetrievalFailure(DataCacheEvent evt);
    public void dataEvicted(DataCacheEvent evt);
    
}
