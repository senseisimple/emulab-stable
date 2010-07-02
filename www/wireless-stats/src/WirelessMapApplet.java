/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006-2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
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

public class WirelessMapApplet extends javax.swing.JApplet {
    
    private DatasetModel model;
    private DataCache cache;
    private String chosenFirstDataset;
    
    public void init() {
        
        this.model = new DatasetModel();
        
        try {
            setup();
            
            final String tdsn = chosenFirstDataset;
            final DatasetModel tdm = model;
            javax.swing.SwingUtilities.invokeLater(new Runnable() {
                public void run() {
                    initComponents();
                    
                    nodeMapPanel.setContainingScrollPane(nodeMapPanelScrollPane);
                    
                    tdm.setCurrentModel(tdsn);
                    
                }
            });
        }
        catch (Exception ex) {
            ex.printStackTrace();
        }
    }
    
    private void setup() {
        this.cache = new DataCache(this);
        
        this.cache.setMinFreeThreshold(0.10f);
        
        chosenFirstDataset = null;
        
        // grab our params...
        // this is so cool, check out the cascading text!
        String uid = this.getParameter("uid");
        String auth = this.getParameter("auth");
        String mapurl = this.getParameter("mapurl");
        String dataurl = this.getParameter("dataurl");
        String positurl = this.getParameter("positurl");
        String datasetStr = this.getParameter("datasets");
        String fragDS[] = null;
        URL base = this.getCodeBase();
        
        if (datasetStr == null || datasetStr.length() == 0) {
            System.err.println("No datasets!");
            System.exit(-1);
        }
        else if (datasetStr.indexOf(";") > -1) {
            fragDS = datasetStr.split(";");
        }
        else {
            fragDS = new String[1];
            fragDS[0] = datasetStr;
        }
        
        // now, for each dataset name, we have to grab the data and the appropriate image
        for (int i = 0; i < fragDS.length; ++i) {
            String[] sa = fragDS[i].split(",");
            // sa[i] should look like 'Wifi-MEB,MEB,3,1:2:3:4:5,1:1.5:2:3:4,7.180:10.770:14.360:17.950:21.540;'
            if (sa.length != 6) {
                System.err.println("bad dataset spec '"+sa[i]+"'!");
                continue;
            }
            
            String dataset = sa[0];
            String building = sa[1];
            String floor = sa[2];
            String[] scales = sa[3].split(":");
            String[] scaleFactors = sa[4].split(":");
            String[] ppms = sa[5].split(":");
        
            Dataset ds = new Dataset();
            ds.name = dataset;
            ds.building = building;
            ds.floor = new int[1];
            ds.floor[0] = Integer.parseInt(floor);
            ds.scale = new int[scales.length];
            for (int j = 0; j < scales.length; ++j) {
                ds.scale[j] = Integer.parseInt(scales[j]);
            }
            ds.scaleFactor = new float[scaleFactors.length];
            for (int j = 0; j < scaleFactors.length; ++j) {
                ds.scaleFactor[j] = Float.parseFloat(scaleFactors[j]);
            }
            ds.ppm = new float[ppms.length];
            for (int j = 0; j < ppms.length; ++j) {
                ds.ppm[j] = Float.parseFloat(ppms[j]);
            }
            
            //
            // first grab the positions file:
            //
            try {
                InputStream posit_is = null;
                System.err.println("base = "+base.toString());
                URL positfile = new URL(base,
                                    positurl +
                                    "&dataset=" + URLEncoder.encode(dataset) + 
                                    "&nocookieuid=" + URLEncoder.encode(uid) +
                                    "&nocookieauth=" + URLEncoder.encode(auth)
                                   );
                URLConnection uc = positfile.openConnection();
                Object content = uc.getContent();
                
                if (uc.getContentType().equalsIgnoreCase("text/plain")) {
                    // in this case, content should be of type
                    //  sun.net.www.content.text.PlainTextInputStream
                    // ...
                    posit_is = (InputStream)content;
                }
                else {
                    System.err.println("Data type ("+uc.getContentType()+") not plain text!");
                    continue;
                }
                
                // now try to parse the data
                // every time we add a new kind of data file and an associated parser,
                // we must add a try in here with it!
                ds.positions = NodePositions.parseInputStream(posit_is);
                
                // now, check over the positions file and see if there are more floors
                // than just the primary floor we received from the webserver in the params:
                for (Enumeration e1 = ds.positions.getNodeList(); e1.hasMoreElements(); ) {
                    String nn = (String)e1.nextElement();
                    ds.addFloor(ds.positions.getPosition(nn).floor);
                }
                
                System.err.println("Successfully parsed posit for dataset '"+dataset+"'");
                
            }
//            catch (MalformedURLException e) {
//                e.printStackTrace();
//                System.exit(-1);
//            }
//            catch (IOException e) {
//                e.printStackTrace();
//                System.exit(-1);
//            }
            catch (Exception e) {
                System.err.println("Could not successfully parse positfile for dataset '" +
                                   dataset+"'!");
                e.printStackTrace();
                continue;
            }
            
            //
            // do datafile -- may be zipped or plain text
            //
            try {
                InputStream data_is = null;
                URL datafile = new URL(base,
                                   dataurl + 
                                   "&dataset=" + URLEncoder.encode(dataset) + 
                                   "&nocookieuid=" + URLEncoder.encode(uid) + 
                                   "&nocookieauth=" + URLEncoder.encode(auth)
                                  );
                URLConnection uc = datafile.openConnection();
                Object content = uc.getContent();
                
                if (uc.getContentType().equalsIgnoreCase("application/zip")) {
                    // grab the zip data
                    // in this case, content should be of type
                    //  sun.net.www.protocol.http.HttpURLConnection$HttpInputStream
                    // ...
                    ZipInputStream zis = new ZipInputStream((InputStream)content);
                    //ZipEntry ze = zis.getNextEntry();
                    data_is = zis;
                }
                else if (uc.getContentType().equalsIgnoreCase("text/plain")) {
                    // in this case, content should be of type
                    //  sun.net.www.content.text.PlainTextInputStream
                    // ...
                    data_is = (InputStream)content;
                }
                else {
                    System.err.println("Data type not zip or plain text!");
                    continue;
                }
                
                // now try to parse the data
                // every time we add a new kind of data file and an associated parser,
                // we must add a try in here with it!
                ds.data = GenericStats.parseInputStream(data_is);
                System.err.println("Successfully parsed datafile for dataset '" +
                                   dataset+"'");
                
            }
            catch (MalformedURLException e) {
                e.printStackTrace();
                continue;
            }
            catch (IOException e) {
                e.printStackTrace();
                continue;
            }
            catch (Exception e) {
                System.err.println("Couldn't parse data file!");
                e.printStackTrace();
                continue;
            }
            
            //
            // so, we've got the data -- what about the image?
            // we now actually grab ALL the images we might need.
            // for each tuple <floor,scale> we grab the image for this
            // building.
            // of course, we cache globally.  :-)
            //
            for (int j = 0; j < ds.floor.length; ++j) {
                for (int k = 0; k < ds.scale.length; ++k) {
                    
                    String tag = ds.building + "-" + ds.floor[j] + "-" + ds.scale[k];
                    
                    DataCacheObject img = null;
                    URL imgfile = null;
                    try {
                        imgfile = new URL(base,
                                          mapurl +
                                          "&dataset=" + URLEncoder.encode(dataset) + 
                                          "&building=" + URLEncoder.encode(building) + 
                                          "&floor=" + URLEncoder.encode(""+ds.floor[j]) + 
                                          "&scale=" + URLEncoder.encode(""+ds.scale[k]) + 
                                          "&nocookieuid=" + URLEncoder.encode(uid) +
                                          "&nocookieauth=" + URLEncoder.encode(auth)
                                         );
                    }
                    catch (MalformedURLException ex) {
                        System.err.println("WARNING: bad url while trying to grab img " + tag);
                        ex.printStackTrace();
                        continue;
                    }
                    
                    // preload if scale is 1 so that we have something to look
                    // at right away...
                    if (ds.scale[k] == 1) {
                        img = cache.registerURL(imgfile,DataCache.EVICT_NEVER);
                        cache.getURL(img);
                        System.out.println("Retrieved img " + tag);
                    }
                    else {
                        img = cache.registerURL(imgfile,DataCache.EVICT_ATNEED);
                        // do nothing right now...
                        System.out.println("Registered img " + tag + " but did not download");
                    }
                    
                    ds.addImage(img,ds.floor[j],ds.scale[k]);
                }
            }
            
            //
            // all this crap was downloaded successfully... move on!
            //
            ds.model = new MapDataModel(ds);
            this.model.addDataset(ds.name,ds);
            if (this.chosenFirstDataset == null) {
                this.chosenFirstDataset = ds.name;
            }
            if (ds.name.equalsIgnoreCase("Wifi-MEB")) {
                this.chosenFirstDataset = ds.name;
            }
            
        }
        
        // this is the ugliest hack in the book, but the images won't display
        // right away unless we sleep and let them settle, then reset the model.
        // The reason that this seems to happen is that createImage won't give
        // us a BufferedImage until the applet leaves the "headless" state, and
        // at this point, the init method in the JApplet override hasn't 
        // completed.  Lots of fun.
//        final DatasetModel tdm = this.model;
//        final String tdsn = chosenFirstDataset;
//        final WirelessMapApplet tapp = this;
//        javax.swing.SwingUtilities.invokeLater(new Runnable() {
//            public void run() {
//                while (tapp.getNMP() != null && tapp.getNMP().createImage(5,5) == null) {
//                    try {
//                        Thread.currentThread().sleep(100);
//                    }
//                    catch (Exception ex) {
//                        ex.printStackTrace();
//                    }
//                }
//                System.out.println("DEBUG: setting current dataset in dataset model to " + tdsn);
//                tdm.setCurrentModel(tdsn);
//            }
//        });
    }
    
    protected NodeMapPanel getNMP() {
        return this.nodeMapPanel;
    }
    
    // <editor-fold defaultstate="collapsed" desc=" Generated Code ">//GEN-BEGIN:initComponents
    private void initComponents() {
        java.awt.GridBagConstraints gridBagConstraints;

        jDialog1 = new javax.swing.JDialog();
        nodeMapPanelScrollPane = new javax.swing.JScrollPane();
        nodeMapPanel = new NodeMapPanel(model);
        controlPanel = new ControlPanel(model);

        getContentPane().setLayout(new java.awt.GridBagLayout());

        nodeMapPanelScrollPane.setViewportView(nodeMapPanel);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        getContentPane().add(nodeMapPanelScrollPane, gridBagConstraints);

        controlPanel.setPreferredSize(new java.awt.Dimension(200, 247));
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 1;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.VERTICAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.NORTH;
        gridBagConstraints.weighty = 1.0;
        getContentPane().add(controlPanel, gridBagConstraints);

    }// </editor-fold>//GEN-END:initComponents
    
    
    // Variables declaration - do not modify//GEN-BEGIN:variables
    private ControlPanel controlPanel;
    private javax.swing.JDialog jDialog1;
    private NodeMapPanel nodeMapPanel;
    private javax.swing.JScrollPane nodeMapPanelScrollPane;
    // End of variables declaration//GEN-END:variables
    
}
