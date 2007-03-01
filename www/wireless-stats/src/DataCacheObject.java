/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006-2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

import java.net.URL;

/*
 * Wraps a data object so that the user never has to keep a reference
 * to the object.  This is necessary so that the cache can remove objects
 * from memory... and it will only work if the user NEVER keeps a reference
 * to a cached object --- they use it and discard their ref.
 */
class DataCacheObject {

    private URL url;
    private DataCache cache;
    private int evictPolicy;

    public DataCacheObject(DataCache cache,URL u,int evictPolicy) {
        this.cache = cache;
        this.url = u;
        this.evictPolicy = evictPolicy;
    }

    public URL getURL() {
        return url;
    }

    public DataCache getCache() {
        return cache;
    }

    public int getEvictPolicy() {
        return this.evictPolicy;
    }
    
    /*
     * This method blocks until the cache delivers the object; returns
     * null if the cache could not deliver (i.e., OutOfMemory).
     */
    public Object getObject() {
        return this.cache.getURL(this);
    }

    /*
     * Here, we basically trigger off a "fetch" request; the idea is that
     * a call to getObjectAsync can be followed directly with a call to
     * getObject.   We can't return the object in the listener event, 
     * because that would violate the cache policy of never letting anybody
     * but the user directly use the cached object.  Thus, we accept the 
     * slight race.  Quite frankly, there won't be much of a race if 
     * the user has setup their listener correctly---they will get a 
     * dataEvicted message before they call getObject.
     *
     * A little hysteresis reduces the probability of this race, since we
     * keep objects around for at least 10s before evicting them (LRU also
     * minimizes the odds).
     */
    public void getObjectAsync() {
        this.cache.preloadURL(this);
    }

}
