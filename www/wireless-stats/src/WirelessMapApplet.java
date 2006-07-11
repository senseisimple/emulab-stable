
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
    
    //private Image bgImage;
    private java.awt.image.ImageObserver io;
    private Hashtable datasets;
    private Hashtable imageCache;
    
    public void init() {
        
        try {
            setup();
            
            javax.swing.SwingUtilities.invokeAndWait(new Runnable() {
                public void run() {
                    initComponents();
                }
            });
            
        } catch (Exception ex) {
            ex.printStackTrace();
        }
    }
    
    private void setup() {
        this.datasets = new Hashtable();
        this.imageCache = new Hashtable();
        
        io = new java.awt.Component() {
            public boolean updateImage(Image img, int infoflags, int x, int y, int width, int height) {
                System.out.println("w = "+width+",h = "+height);
                
                return true;
            }
        };
        
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
                    System.exit(-3);
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
                System.exit(-1);
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
                    System.exit(-3);
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
                System.exit(-1);
            }
            catch (IOException e) {
                e.printStackTrace();
                System.exit(-1);
            }
            catch (Exception e) {
                System.err.println("Couldn't parse data file!");
                e.printStackTrace();
                System.exit(-1);
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
                    
                    Image img = null;
                    if ((img = (Image)imageCache.get(tag)) == null) {
                        try {

                            URL imgfile = new URL(base,
                                              mapurl +
                                              "&dataset=" + URLEncoder.encode(dataset) + 
                                              "&building=" + URLEncoder.encode(building) + 
                                              "&floor=" + URLEncoder.encode(""+ds.floor[j]) + 
                                              "&scale=" + URLEncoder.encode(""+ds.scale[k]) + 
                                              "&nocookieuid=" + URLEncoder.encode(uid) +
                                              "&nocookieauth=" + URLEncoder.encode(auth)
                                             );
                            img = this.getImage(imgfile);
                            MediaTracker tracker = new MediaTracker(this);
                            tracker.addImage(img,0);
                            tracker.waitForID(0);
                            System.out.println("got image for dataset '"+dataset+"'; w = "+img.getWidth(io)+", h = "+img.getHeight(io));
                            if (tracker.isErrorID(0)) {
                                System.err.println("Errors with image!!!");
                                img = null;
                            }
                            else {
                                System.err.println("Image had no errors!");
                                imageCache.put(tag,img);
                            }
                        }
                        catch (Exception e) {
                            System.err.println("Could not download image for dataset '"+dataset+"'!");
                            e.printStackTrace();
                            System.exit(-5);
                        }
                    }
                    
                    //System.err.println("imageCache="+imageCache+",tag="+tag+",img="+img);
                    //imageCache.put(tag,img);
                    if (img != null) {
                        ds.addImage(img,ds.floor[j],ds.scale[k]);
                    }
                    
                    
                    System.err.println("imageRun; dataset="+ds.name+",tag="+tag+",img="+img);
                }
            }
            
            //
            // all this crap was downloaded successfully... move on!
            //
            ds.model = new MapDataModel(ds);
            datasets.put(dataset,ds);
            
        }
    }
    
    /** This method is called from within the init() method to
     * initialize the form.
     * WARNING: Do NOT modify this code. The content of this method is
     * always regenerated by the Form Editor.
     */
    // <editor-fold defaultstate="collapsed" desc=" Generated Code ">//GEN-BEGIN:initComponents
    private void initComponents() {
        java.awt.GridBagConstraints gridBagConstraints;

        nodeMapPanelScrollPane = new javax.swing.JScrollPane();
        nodeMapPanel = new NodeMapPanel();
        controlPanel = new ControlPanel(datasets,nodeMapPanel);

        getContentPane().setLayout(new java.awt.GridBagLayout());

        nodeMapPanelScrollPane.setViewportView(nodeMapPanel);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        getContentPane().add(nodeMapPanelScrollPane, gridBagConstraints);

        controlPanel.setPreferredSize(new java.awt.Dimension(200, 247));
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.VERTICAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        getContentPane().add(controlPanel, gridBagConstraints);

    }// </editor-fold>//GEN-END:initComponents
    
    
    // Variables declaration - do not modify//GEN-BEGIN:variables
    private ControlPanel controlPanel;
    private NodeMapPanel nodeMapPanel;
    private javax.swing.JScrollPane nodeMapPanelScrollPane;
    // End of variables declaration//GEN-END:variables
    
}
