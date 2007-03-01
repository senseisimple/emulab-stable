/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006-2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

import java.util.*;
import java.util.zip.*;
import java.awt.Image;
import java.awt.image.*;
import java.net.URL;
import java.net.URLEncoder;
import java.net.URLConnection;
import java.net.MalformedURLException;
import java.io.IOException;
import java.io.*;
import java.security.MessageDigest;

/*
 * DataCache does exactly what it says it does.  It can retrieve dynamically-
 * generated content from the web and do a diff on the raw data to see if it
 * matches anything we've already retrieved.  You can "preload" items (i.e., 
 * add the url retrieval info, but not actually fetch), fetch on demand, set
 * an evict policy (i.e., to see if you're running short on system RAM and need
 * to evict before grabbing more).  If objects are the same, callers are given
 * refs to the one object saved.
 */
public class DataCache {
    
    public static final int EVICT_NEVER = 1;
    // eviction is based on an LRU policy, of course
    public static final int EVICT_ATNEED = 2;
    
    private static final int EVENT_TYPE_RETRIEVED = 1;
    private static final int EVENT_TYPE_RETRIEVAL_FAILURE = 2;
    private static final int EVENT_TYPE_EVICTED = 3;
    
    private java.applet.Applet applet;
    // URL :: CacheObject
    private Hashtable cache;
    // CacheObject :: Object (type determined by URLConnection.getContent())
    private Hashtable objMap;
    // Object (real one) :: md5(raw byte array)
    private Hashtable objMD5Map;
    // Object (real one) :: last access time
    private Hashtable objAccessTimes;
    // CacheObject :: Vector(DataCacheListener)
    private Hashtable dcls;
    
    private float minFreeThreshold;
    
    void evictLRUObject() {
        long ctime = System.currentTimeMillis();
        Vector cacheObjects = null;
        Object lruRealObject = null;
        long currentOldestAge = Long.MIN_VALUE;
        
        for (Enumeration e1 = objAccessTimes.keys(); e1.hasMoreElements(); ) {
            Object key = e1.nextElement();
            long atime = ((Long)objAccessTimes.get(key)).longValue();
            
            if ((ctime - atime) < currentOldestAge) {
                continue;
            }
            
            boolean canEvict = true;
            Vector tmpCacheObjects = new Vector();
            
            // now see if all CacheObjects pointing to the real object allow
            // eviction... note that this could produce a DOS on the memory 
            // system if a user came along and said no eviction for all objects
            // in the cache...
            for (Enumeration e2 = objMap.keys(); e2.hasMoreElements(); ) {
                // check through the current map to see if all CacheObjects that
                // map to this object all have evict policy set:
                DataCacheObject co = (DataCacheObject)e2.nextElement();
                Object realObj = objMap.get(co);
                if (realObj.equals(key)) {
                    if (co.getEvictPolicy() != EVICT_ATNEED) {
                        canEvict = false;
                        break;
                    }
                    else {
                        tmpCacheObjects.add(co);
                    }
                }
            }
            
            if (canEvict) {
                currentOldestAge = ctime - atime;
                lruRealObject = key;
                cacheObjects = tmpCacheObjects;
            }
        }
        
        if (lruRealObject != null) {
            // something to evict!
            for (Enumeration e1 = cacheObjects.elements(); e1.hasMoreElements(); ) {
                DataCacheObject dco = (DataCacheObject)e1.nextElement();
                objMap.remove(dco);
                notifyListeners(dco,new DataCacheEvent(this,dco),
                                this.EVENT_TYPE_EVICTED);
            }
            objMD5Map.remove(lruRealObject);
            objAccessTimes.remove(lruRealObject);
        }
    }
    
    public DataCache(java.applet.Applet applet) {
        this.applet = applet;
        
        this.minFreeThreshold = 0.05f;
        
        this.cache = new Hashtable();
        this.objMap = new Hashtable();
        this.objMD5Map = new Hashtable();
        this.objAccessTimes = new Hashtable();
        this.dcls = new Hashtable();
        
        // spin off a thread to monitor memory usage and warn/evict
        // as necessary
        (new MMThread(this)).start();
    }
    
    public DataCacheObject registerURL(URL u,int evict_policy) {
        for (Enumeration e1 = cache.keys(); e1.hasMoreElements(); ) {
            if (((URL)e1.nextElement()).equals(u)) {
                debug("while registering " + u + " found in cache");
                return (DataCacheObject)cache.get(u);
            }
        }
        DataCacheObject o = new DataCacheObject(this,u,evict_policy);
        cache.put(u,o);
        debug("registered u");
        
        return o;
    }
    
    private void debug(String s) {
        System.out.println("DEBUG: DataCache: " + s);
    }
    
    Object getDone(DataCacheObject o,Exception ex,
                   Object content,byte[] rawContent) {
        Object retval = null;
        DataCacheEvent ce = new DataCacheEvent(this,o);
        Exception lex = ex;
        
        // now check the cache and see if there's an identical object in there.
        if (ex == null) {
            byte[] newDigest = null;
            try {
                MessageDigest di = MessageDigest.getInstance("MD5");
                newDigest = di.digest(rawContent);
                
                debug("computed digest for " + o.getURL() + " to be " + newDigest.toString());
                
                Object realObj = null;
                for (Enumeration e1 = objMD5Map.keys(); e1.hasMoreElements(); ) {
                    Object key = e1.nextElement();
                    byte[] digest = (byte[])objMD5Map.get(key);
                    if (digest.length == newDigest.length) {
                        boolean eq = true;
                        for (int i = 0; i < digest.length; ++i) {
                            if (digest[i] != newDigest[i]) {
                                eq = false;
                                break;
                            }
                        }
                        if (eq) {
                            realObj = key;
                            break;
                        }
                    }
                }
                
                if (realObj != null) {
                    // need to associate this cacheobject with the 
                    // backing object
                    this.objMap.put(o,realObj);
                    
                    debug("getDone: using cached object " + 
                          ((realObj != null)?realObj.getClass().toString():"") + 
                          " for " + o.getURL());
                }
                else {
                    // need to save this object:
                    realObj = content;
                    this.objMap.put(o,realObj);
                    this.objMD5Map.put(realObj,newDigest);
                    
                    debug("getDone: using new object " + 
                          ((realObj != null)?realObj.getClass().toString():"") + 
                          " for " + o.getURL());
                }
                this.objAccessTimes.put(realObj,new Long(System.currentTimeMillis()));
                
                retval = realObj;
            }
            catch (Exception ex2) {
                ex2.printStackTrace();
                lex = ex2;
            }
        }
        
        ce.setException(lex);
        notifyListeners(o,ce,
                        (lex == null)?EVENT_TYPE_RETRIEVED:EVENT_TYPE_RETRIEVAL_FAILURE);
        
        return retval;
    }
    
    public Object getURL(DataCacheObject o) {
        Object retval = null;
        if ((retval = this.objMap.get(o)) != null) {
            this.objAccessTimes.put(retval,new Long(System.currentTimeMillis()));
            debug("getURL: cache hit");
            return retval;
        }
        else {
            Object contentObj = null;
            byte[] raw = null;
            Exception rete = null;
            try {
                URLConnection uconn = o.getURL().openConnection();
                debug("getURL: content type = '" + uconn.getContentType() + "'");
                contentObj = uconn.getContent();
//                debug("getURL: first contentObj = " + 
//                      ((contentObj != null)?contentObj.getClass().toString():""));
                
                InputStream is = uconn.getInputStream();
                //contentObj = uconn.getContent();
                byte[] tmpraw;
                raw = new byte[1024];
                int rc = 0;
                int total = 0;
                while ((rc = is.read(raw,total,raw.length - total)) != -1) {
                    total += rc;
                    if (total == raw.length) {
                        tmpraw = new byte[raw.length + 1024];
                        System.arraycopy(raw,0,tmpraw,0,raw.length);
                        raw = tmpraw;
                    }
                }
                tmpraw = new byte[total];
                System.arraycopy(raw,0,tmpraw,0,total);
                raw = tmpraw;
            }
            catch (Exception ex) {
                ex.printStackTrace();
                
                raw = null;
                contentObj = null;
                rete = ex;
            }
            
            // now, some trickery... we have to actually create the image from 
            // the content source... a silly sun! they're so silly.
            if (contentObj != null && 
                contentObj instanceof java.awt.image.ImageProducer) {
                contentObj = java.awt.Toolkit.getDefaultToolkit().createImage(raw);
//                debug("getURL: second contentObj = " + 
//                      ((contentObj != null)?contentObj.getClass().toString():""));
                  
//                java.awt.image.ImageObserver io = new java.awt.image.ImageObserver() {
//                    public boolean imageUpdate(Image img,int iflags,int x,int y,
//                                               int w,int h) {
//                        System.out.println("DEBUG: w="+w+",h="+h+" for img " + img);
//                        if (w > 1 && h > 1) {
//                            return false;
//                        }
//                        return true;
//                    }
//                };
//                ((java.awt.Image)contentObj).getHeight(io);
            }
            
            debug("getURL: downloaded object (" + 
                  ((contentObj != null)?contentObj.getClass().toString():"") + 
                  ")" + " for " + o.getURL());
            
            return getDone(o,rete,contentObj,raw);
        }
    }
    
    public void preloadURL(DataCacheObject o) {
        // first check cache --- if it's there, signal the dcl
        // before returning
        Object retval = this.objMap.get(o);
        if (retval != null) {
            // update timestamp?  no.  only usage.
            notifyListeners(o,new DataCacheEvent(this,o),
                            EVENT_TYPE_RETRIEVED);
            return;
        }
        else {
            final DataCache odc = this;
            final DataCacheObject oco = o;
            Thread gT = new Thread() {
                private final DataCache dc = odc;
                private final DataCacheObject co = oco;

                public void run() {
                    odc.getURL(co);
                }
            };
        }
    }
    
    public void addCacheListener(DataCacheObject o,DataCacheListener dcl) {
        Vector dclList = null;
        if ((dclList = (Vector)dcls.get(o)) == null) {
            dclList = new Vector();
            dcls.put(o,dclList);
        }
        if (!dclList.contains(dcl)) {
            dclList.add(dcl);
        }
    }
    
    public void removeCacheListener(DataCacheObject o,DataCacheListener dcl) {
        Vector dclList = null;
        if ((dclList = (Vector)dcls.get(o)) == null) {
            dclList.remove(dcl);
        }
    }
    
    void notifyListeners(DataCacheObject o,DataCacheEvent e,int evtType) {
        Vector dclList = (Vector)dcls.get(o);
        if (dclList != null) {
            for (Enumeration e1 = dclList.elements(); e1.hasMoreElements(); ) {
                switch(evtType) {
                    case EVENT_TYPE_RETRIEVED:
                        ((DataCacheListener)e1.nextElement()).dataRetrieved(e);
                        break;
                    case EVENT_TYPE_RETRIEVAL_FAILURE:
                        ((DataCacheListener)e1.nextElement()).dataRetrievalFailure(e);
                        break;
                    case EVENT_TYPE_EVICTED:
                        ((DataCacheListener)e1.nextElement()).dataEvicted(e);
                        break;
                    default:
                        e1.nextElement();
                        break;
                }
            }
        }
    }
    
    public void setMinFreeThreshold(float mft) {
        this.minFreeThreshold = mft;
    }
    
    public float getMinFreeThreshold() {
        return this.minFreeThreshold;
    }
    
    class MMThread extends Thread {
        private DataCache cache;
        
        public MMThread(DataCache cache) {
            this.cache = cache;
        }
        
        public void run() {
            while (true) {
                
                // if the remaining memory is below threshold, evict what
                // is both necessary and proper
                Runtime r = Runtime.getRuntime();
                r.gc();
                float currentFree = r.freeMemory() / r.totalMemory();
                
                if (currentFree < cache.getMinFreeThreshold()) {
                    // need to evict...
                    this.cache.evictLRUObject();
                }
                
                try {
                    Thread.currentThread().sleep(5000);
                }
                catch (Exception ex) {
                    ;
                }
            }
        }
    }
    
}
