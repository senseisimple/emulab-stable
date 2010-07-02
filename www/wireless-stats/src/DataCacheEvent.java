/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

public class DataCacheEvent {
    private DataCache src;
    private DataCacheObject obj;
    private Exception exception;

    public DataCacheEvent(DataCache cache,DataCacheObject o) {
        this.src = cache;
        this.obj = o;
        this.exception = null;
    }

    void setException(Exception ex) {
        this.exception = ex;
    }

    public DataCache getSource() {
        return this.src;
    }

    public DataCacheObject getCacheObject() {
        return this.obj;
    }

    public Exception getException() {
        return this.exception;
    }

    public boolean isSuccess() {
        return (this.exception == null)?true:false;
    }
}
