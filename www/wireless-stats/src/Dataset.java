/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006-2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

import java.util.*;

public class Dataset {
    public String name;
    public String building;
    public int[] floor;
    public int[] scale;
    public float[] scaleFactor;
    public String image_path;
    public float[] ppm;
    public String positionFile;
    public String dataFile;
    
    public MapDataModel model;
    // "<floor>-<scale>" :: DataCacheObject
    public Hashtable bgImages;
    public GenericWirelessData data;
    public NodePosition positions;
    

    public Dataset(String name,String positionFile,String dataFile,
                   String building,int floor,String image_path,float ppm) {
        this.name = name;
        this.building = building;
        this.floor = new int[1];
        this.floor[0] = floor;
        this.image_path = image_path;
        this.ppm = new float[1];
        this.ppm[0] = ppm;
        this.scale = new int[1];
        this.scale[0] = 1;
        this.scaleFactor = new float[1];
        this.scaleFactor[0] = 1;
        this.positionFile = positionFile;
        this.dataFile = dataFile;
        this.bgImages = new Hashtable();
    }
    
    public Dataset() {
        this.name = null;
        this.building = null;
        this.floor = null;
        this.image_path = null;
        this.scale = null;
        this.scaleFactor = null;
        this.ppm = null;
        this.positionFile = null;
        this.dataFile = null;
        this.bgImages = new Hashtable();
    }
    
    // We only don't take the building argument because I postulate that
    // it will never be the case that we have statistics from between 
    // buildings.  For me, one building per dataset is good enough for our
    // needs into the foreseeable future... just like 64k.  :-)
    public java.awt.Image getImage(int floor,int scale) {
        System.err.println("getting cache image for building="+this.building+",floor="+floor+",scale="+scale);
        String tag = "" + floor + "-" + scale;
        return (java.awt.Image)((DataCacheObject)this.bgImages.get(tag)).getObject();
    }
    
    public void addImage(DataCacheObject dco,int floor,int scale) {
        System.err.println("adding cache image for building="+this.building+",floor="+floor+",scale="+scale);
        String tag = "" + floor + "-" + scale;
        this.bgImages.put(tag,dco);
    }
    
    public float getScaleFactor(int scale) {
        if (this.scale != null) {
            int i;
            for (i = 0; i < this.scale.length; ++i) {
                if (this.scale[i] == scale) {
                    break;
                }
            }
            
            if (i != this.scale.length) {
                return this.scaleFactor[i];
            }
        }
        return 1.0f;
    }
    
    public void addFloor(int f) {
        int[] tmp;
        if (this.floor == null) {
            tmp = new int[1];
            tmp[1] = f;
            this.floor = tmp;
        }
        else {
            // check and see if it's already in the array:
            for (int i = 0; i < this.floor.length; ++i) {
                if (this.floor[i] == f) {
                    return;
                }
            }
            
            tmp = new int[this.floor.length+1];
            System.arraycopy(this.floor,0,tmp,0,this.floor.length);
            tmp[this.floor.length] = f;
            this.floor = tmp;
        }
    }
    
    public void addScale(int f) {
        int[] tmp;
        if (this.scale == null) {
            tmp = new int[1];
            tmp[1] = f;
            this.scale = tmp;
        }
        else {
            tmp = new int[this.scale.length+1];
            System.arraycopy(this.scale,0,tmp,0,this.scale.length);
            tmp[this.scale.length] = f;
            this.scale = tmp;
        }
    }
    
    public int[] getScales() {
        return this.scale;
    }
    
    public boolean isScale(int f) {
        if (this.scale == null) {
            return false;
        }
        for (int i = 0; i < this.scale.length; ++i) {
            if (this.scale[i] == f) {
                return true;
            }
        }
        return false;
    }
    
}
