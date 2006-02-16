
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
    private Hashtable mapImages;
    
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
        this.mapImages = new Hashtable();
        
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
        else if (datasetStr.indexOf(",") > -1) {
            fragDS = datasetStr.split(",");
        }
        else {
            fragDS = new String[1];
            fragDS[0] = datasetStr;
        }
        
        // now, for each dataset name, we have to grab the data and the appropriate image
        for (int i = 0; i < fragDS.length; ++i) {
            //
            // start with datafile -- may be zipped or plain text
            //
            URL datafile = null;
            InputStream data_is = null;
            try {
                datafile = new URL(base,
                                   "/dev/johnsond/wireless-stats/" + dataurl + 
                                   "&dataset=" + URLEncoder.encode(fragDS[i]) + 
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
                e.printStackTrace();
                System.exit(-1);
            }
            
            WirelessData data = null;
            if (data == null) {
                try {
                    // now try to parse the data
                    // every time we add a new kind of data file and an associated parser,
                    // we must add a try in here with it!
                    data = ILEStats.parseInputStream(data_is);
                    
                    int pLs[] = data.getPowerLevels();
                    System.out.print("power levels = ");
                    for (int j = 0; j < pLs.length; ++j) {
                        System.out.print(""+pLs[j]+",");
                    }
                    System.out.println();
                }
                catch (Exception e) {
                    e.printStackTrace();
                }
            }
            
            // in future, add more to process other datafiles...
            // XXX: add here:
            
            
            if (data == null) {
                System.err.println("Could not successfully parse datafile for dataset '" +
                                   fragDS[i]+"'!");
                System.exit(-4);
            }
            
            //
            // ok, now we need to grab the positions file:
            //
            URL positfile = null;
            InputStream posit_is = null;
            try {
                positfile = new URL(base,
                                    "/dev/johnsond/wireless-stats/" + positurl +
                                    "&dataset=" + URLEncoder.encode(fragDS[i]) + 
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
                e.printStackTrace();
                System.exit(-1);
            }
            
            NodePosition np_data = null;
            try {
                // now try to parse the data
                // every time we add a new kind of data file and an associated parser,
                // we must add a try in here with it!
                np_data = NodePositions.parseInputStream(posit_is);
            }
            catch (Exception e) {
                e.printStackTrace();
            }
            
            if (np_data == null) {
                System.err.println("Could not successfully parse positfile for dataset '" +
                                   fragDS[i]+"'!");
                System.exit(-4);
            }
            
            //
            // so, we've got the data -- what about the image?
            //
            URL imgfile = null;
            Image img = null;
            try {
                imgfile = new URL(base,
                                  "/dev/johnsond/wireless-stats/" + mapurl +
                                  "&dataset=" + URLEncoder.encode(fragDS[i]) + 
                                  "&nocookieuid=" + URLEncoder.encode(uid) +
                                  "&nocookieauth=" + URLEncoder.encode(auth)
                                 );
                img = getToolkit().getImage(imgfile);
                MediaTracker tracker = new MediaTracker(this);
                tracker.addImage(img,0);
                tracker.waitForID(0);
                System.out.println("got image for dataset '"+fragDS[i]+"'; w = "+img.getWidth(io)+", h = "+img.getHeight(io));
                if (tracker.isErrorID(0)) {
                    System.err.println("Errors with image!!!");
                }
                else {
                    System.out.println("Image had no errors!");
                }
            }
            catch (Exception e) {
                System.err.println("Could not download image for dataset '"+fragDS[i]+"'!");
                e.printStackTrace();
                System.exit(-5);
            }
            
            //
            // all this crap was downloaded successfully... move on!
            //
            MapDataModel model = new MapDataModel(data,np_data);
            datasets.put(fragDS[i],model);
            mapImages.put(fragDS[i],img);
            
        }
        
//        bgImage = Toolkit.getDefaultToolkit().getImage("/home/david/work/java/floormap.jpg");
//        mapImages.put("Floor4/WSN",bgImage);
//        
//        try {
//            MediaTracker tracker = new MediaTracker(this);
//            tracker.addImage(bgImage, 0);
//            tracker.waitForID(0);
//            //System.out.println("width = "+bgImage.getWidth(io));
//        }
//        catch (InterruptedException ex) {
//            ex.printStackTrace();
//        }
//        
//        // in the applet, we'll read in the possible datasets and 
//        
//        WirelessData defaultData = null;
//        NodePosition defaultPositions = null;
//        MapDataModel defaultModel = null;
//        String defaultDatasetName = "Floor4/WSN";
//        
//        try {
//            defaultData = ILEStats.parseDumpFile("/home/david/work/java/nn_client.log");
//            defaultPositions = NodePositions.parseFile("/home/david/work/java/mote_positions");
//            
//            defaultModel = new MapDataModel(defaultData,defaultPositions);
//            datasets.put(defaultDatasetName,defaultModel);
//        }
//        catch (Exception e) {
//            e.printStackTrace();
//            System.exit(-2);
//        }
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
        controlPanel = new ControlPanel(datasets,mapImages,nodeMapPanel);

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

    }
    // </editor-fold>//GEN-END:initComponents
    
    
    // Variables declaration - do not modify//GEN-BEGIN:variables
    private ControlPanel controlPanel;
    private NodeMapPanel nodeMapPanel;
    private javax.swing.JScrollPane nodeMapPanelScrollPane;
    // End of variables declaration//GEN-END:variables
    
}
