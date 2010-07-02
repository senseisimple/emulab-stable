/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2006-2007 University of Utah and the Flux Group.
 * All rights reserved.
 */

import javax.swing.*;
import javax.swing.event.*;
import javax.swing.border.*;
import java.awt.*;
import java.awt.event.*;
import java.util.*;

public class CollapsablePanelContainer extends javax.swing.JPanel {

    // title :: panel
    Hashtable childPanels;
    // title :: GridBagConstraints
    Hashtable childConstraints;
    // title :: state (false == uncollapsed)
    Hashtable childPanelState;
    // title :: int (index)
    Hashtable childPanelIndex;
    // title :: JButton
    Hashtable childPanelButtons;
    
    public CollapsablePanelContainer() {
        childPanels = new Hashtable();
        childConstraints = new Hashtable();
        childPanelState = new Hashtable();
        childPanelIndex = new Hashtable();
        childPanelButtons = new Hashtable();
        initComponents();
        this.removeAll();
    }
    
    public void add(JComponent c,Object la) {
        if (c instanceof CollapsablePanel) {
            appendChildPanel(((CollapsablePanel)c).getTitle(),c,la);
        }
        else {
            appendChildPanel("unk",c,la);
        }
    }
    
    void appendChildPanel(String title,JComponent ccp,Object layoutArg) {
        Set cpNames = childPanels.keySet();
        if (cpNames == null || cpNames.contains(title)) {
            return;
        }
        
        int y = childPanels.size() * 2;
        
        childPanelIndex.put(title,new Integer(y));
        
        childPanels.put(title,ccp);
        childPanelState.put(title,new Boolean(false));
        
        GridBagConstraints gc = new GridBagConstraints();
        gc.anchor = GridBagConstraints.NORTHWEST;
        gc.gridx = 0;
        gc.gridy = y;
        //gc.weighty = 1.0;
        
        JButton ocButton = new javax.swing.JButton();
        //ocButton.setFont(new java.awt.Font("Dialog", 1, 10));
        ocButton.setIcon(new ImageIcon(getClass().getResource("/open.gif")));
        ocButton.setBorderPainted(false);
        ocButton.setMargin(new java.awt.Insets(0, 0, 0, 0));
        
        childPanelButtons.put(title,ocButton);
        
        final CollapsablePanelContainer cpc = this;
        final String lTitle = title;
        
        ocButton.addActionListener(new ActionListener() {
            final String myTitle = lTitle;
            final CollapsablePanelContainer myCPC = cpc;
            
            public void actionPerformed(ActionEvent evt) {
                myCPC.invertState(myTitle);
            }
        });
        
        super.add(ocButton,gc);
        
        gc.gridx = 1;
        gc.anchor = GridBagConstraints.NORTHWEST;
        //gc.weightx = 1.0;
        
        JLabel ocLabel = new JLabel(title);
        ocLabel.setFont(new java.awt.Font("Dialog", 1, 12));
        
        super.add(ocLabel,gc);
        
//        Border pBorder = BorderFactory.createTitledBorder(null,"",
//                javax.swing.border.TitledBorder.DEFAULT_JUSTIFICATION,
//                javax.swing.border.TitledBorder.DEFAULT_POSITION,
//                new java.awt.Font("Dialog", 0, 10));
        Border pBorder = BorderFactory.createLineBorder(Color.GRAY,1);
        ccp.setBorder(pBorder);
        
        gc.anchor = GridBagConstraints.NORTHWEST;
        gc.gridy = y + 1;
        gc.gridx = 0;
        gc.gridwidth = 2;
        
        if (layoutArg instanceof GridBagConstraints) {
            GridBagConstraints igc = (GridBagConstraints)layoutArg;
            // just need to set the grid args; leave the rest intact.
            //igc.anchor = GridBagConstraints.NORTHWEST;
            //igc.fill = GridBagConstraints.BOTH;
            //igc.weightx = 1.0;
            //igc.weighty = 1.0;
            igc.gridy = y + 1;
            igc.gridx = 0;
            igc.gridwidth = 2;
            this.childConstraints.put(title,igc);
            super.add(ccp,igc);
        }
        else {
            this.childConstraints.put(title,gc);
            super.add(ccp,gc);
        }
        
        this.revalidate();
        this.repaint();
    }
    
    public void invertState(String title) {
        Boolean pstate = (Boolean)childPanelState.get(title);
        if (pstate == null) {
            return;
        }
        setCollapsed(title,!pstate.booleanValue());
    }
    
    public void setCollapsed(String title,boolean collapsed) {
        Boolean pstate = (Boolean)childPanelState.get(title);
        if (pstate == null || pstate.booleanValue() == collapsed) {
            return;
        }
        else {
            childPanelState.put(title,new Boolean(collapsed));
            
            if (collapsed) {
                JComponent oc = (JComponent)childPanels.get(title);
                super.remove(oc);
                JButton ocButton = (JButton)childPanelButtons.get(title);
                ocButton.setIcon(new ImageIcon(getClass().getResource("/closed.gif")));
                this.revalidate();
                this.repaint();
            }
            else {
                int y = ((Integer)childPanelIndex.get(title)).intValue();
                GridBagConstraints gc = (GridBagConstraints)childConstraints.get(title);
//                gc.anchor = GridBagConstraints.NORTHWEST;
//                gc.gridx = 0;
//                gc.gridy = y + 1;
//                gc.gridwidth = 2;
                
                super.add((Component)childPanels.get(title),gc);
                JButton ocButton = (JButton)childPanelButtons.get(title);
                ocButton.setIcon(new ImageIcon(getClass().getResource("/open.gif")));
                this.revalidate();
                this.repaint();
            }
            
        }
    }
    
    /** This method is called from within the constructor to
     * initialize the form.
     * WARNING: Do NOT modify this code. The content of this method is
     * always regenerated by the Form Editor.
     */
    // <editor-fold defaultstate="collapsed" desc=" Generated Code ">//GEN-BEGIN:initComponents
    private void initComponents() {
        jButton1 = new javax.swing.JButton();

        setLayout(new java.awt.GridBagLayout());

        jButton1.setFont(new java.awt.Font("Dialog", 1, 10));
        jButton1.setIcon(new javax.swing.ImageIcon(getClass().getResource("/open.gif")));
        jButton1.setText("jButton1");
        add(jButton1, new java.awt.GridBagConstraints());

    }// </editor-fold>//GEN-END:initComponents
    
    
    // Variables declaration - do not modify//GEN-BEGIN:variables
    private javax.swing.JButton jButton1;
    // End of variables declaration//GEN-END:variables
    
}
