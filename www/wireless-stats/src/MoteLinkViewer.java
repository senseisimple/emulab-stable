
import javax.swing.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;

//class ImageObserverImpl implements java.awt.image.ImageObserver {
//    public boolean updateImage(Image img, int infoflags, int x, int y, int width, int height) {
//        System.out.println("w = "+width+",h = "+height);
//    }
//}

public class MoteLinkViewer extends javax.swing.JFrame {

    private Image bgImage;
    private java.awt.image.ImageObserver io;
    private Hashtable datasets;
    private Hashtable mapImages;
    
    public MoteLinkViewer() {
        this.datasets = new Hashtable();
        this.mapImages = new Hashtable();
        
        io = new java.awt.Component() {
            public boolean updateImage(Image img, int infoflags, int x, int y, int width, int height) {
                System.out.println("w = "+width+",h = "+height);
                
                return true;
            }
        };
        
        bgImage = Toolkit.getDefaultToolkit().getImage("/home/david/from_cvs/wireless-stats/floormap.jpg");
        mapImages.put("Floor4/WSN",bgImage);
        
        try {
            MediaTracker tracker = new MediaTracker(this);
            tracker.addImage(bgImage, 0);
            tracker.waitForID(0);
            //System.out.println("width = "+bgImage.getWidth(io));
        }
        catch (InterruptedException ex) {
            ex.printStackTrace();
        }
        
        // in the applet, we'll read in the possible datasets and 
        
        GenericWirelessData defaultData = null;
        NodePosition defaultPositions = null;
        MapDataModel defaultModel = null;
        String defaultDatasetName = "Floor4/WSN";
        
        try {
            defaultData = GenericStats.parseDumpFile("/home/david/from_cvs/wireless-stats/nn_client.log");
            defaultPositions = NodePositions.parseFile("/home/david/from_cvs/wireless-stats/mote_positions");
            
            defaultModel = new MapDataModel(defaultData,defaultPositions);
            datasets.put(defaultDatasetName,defaultModel);
        }
        catch (Exception e) {
            e.printStackTrace();
            System.exit(-2);
        }
        
        initComponents();
        
        
        
        ;
        
    }
    
    // <editor-fold defaultstate="collapsed" desc=" Generated Code ">//GEN-BEGIN:initComponents
    private void initComponents() {
        java.awt.GridBagConstraints gridBagConstraints;

        nodeMapScrollPane = new javax.swing.JScrollPane();
        nodeMapPanel = new NodeMapPanel();
        //nodeMapPanel.setBackgroundImage(bgImage);
        //nodeMapPanel.setPositions(positions);
        //nodeMapPanel.setILEStats(model);
        controlPanel = new ControlPanel(datasets,mapImages,nodeMapPanel);

        getContentPane().setLayout(new java.awt.GridBagLayout());

        setDefaultCloseOperation(javax.swing.WindowConstants.EXIT_ON_CLOSE);
        nodeMapScrollPane.setBackground(new java.awt.Color(255, 255, 255));
        nodeMapScrollPane.setViewportView(nodeMapPanel);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        getContentPane().add(nodeMapScrollPane, gridBagConstraints);

        controlPanel.setPreferredSize(new java.awt.Dimension(200, 247));
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.fill = java.awt.GridBagConstraints.VERTICAL;
        getContentPane().add(controlPanel, gridBagConstraints);

        pack();
    }
    // </editor-fold>//GEN-END:initComponents
        
    public static void main(final String args[]) {
        java.awt.EventQueue.invokeLater(new Runnable() {
            public void run() {
                new MoteLinkViewer().setVisible(true);
            }
        });
    }
    
    // Variables declaration - do not modify//GEN-BEGIN:variables
    private ControlPanel controlPanel;
    private NodeMapPanel nodeMapPanel;
    private javax.swing.JScrollPane nodeMapScrollPane;
    // End of variables declaration//GEN-END:variables
    
}
