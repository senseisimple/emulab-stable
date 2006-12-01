/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005-2006 University of Utah and the Flux Group.
 * All rights reserved.
 */

import java.util.*;
import java.awt.Image;
import javax.swing.*;
import javax.swing.event.*;
import java.awt.*;
import java.awt.event.*;
import java.beans.*;

public class ControlPanel extends javax.swing.JPanel {
    
    private String currentModelName;
    private MapDataModel currentModel;
    private NodeMapPanel map;
    private Hashtable datasets;
    private Vector modelNames;
    // a map of model name to a Vector (containing JComponents -- generally
    // (label,jcombobox) pairs... all of which get added to the indexPanel
    // when their model is swapped in.
    private Hashtable modelIndexComponents;
    
    // ummm, a gentler comment: this is needed to preserve the component's
    // identity as a bean... so I can add it to the appropriate DND panel
    // in netbeans.
    public ControlPanel() {
        super();
        //this(null,null,null);
    }
    
    public ControlPanel(Hashtable datasets,NodeMapPanel map) {
        super();
        
        // this only exists so we can tell the map about new datasets.
        this.map = map;
        this.datasets = datasets;
        this.modelNames = new Vector();
        this.currentModelName = null;
        this.currentModel = null;
        
        this.modelIndexComponents = new Hashtable();
        
        String tmpModelName = null;
        MapDataModel tmpModel = null;
        
        for (Enumeration e1 = datasets.keys(); e1.hasMoreElements(); ) {
            String modelName = (String)e1.nextElement();
            Dataset ds = (Dataset)datasets.get(modelName);
            
            modelNames.add(modelName);
            if (tmpModelName == null) {
                tmpModelName = modelName;
                tmpModel = ds.model;
            }
            System.err.println("considered model '"+modelName+"'");
        }
        
        initComponents();
        
        System.err.println("setting init model '"+tmpModelName+"' with model="+tmpModel);
        setModel(tmpModelName,tmpModel);
    }
    
    protected void mapSetSelectedNodes(Vector nodes) {
        // the idea is that the nodemap call this to
        // determine the selection based on map clicks:
        
        // first, tell the list about each item:
        nodesList.clearSelection();
        
        if (nodes != null && nodes.size() > 0) {
            String[] data = this.currentModel.getData().getNodes();
            Arrays.sort(data);
            int selected[] = new int[nodes.size()];

            int count = 0;
            for (int i = 0; i < data.length; ++i) {
                if (nodes.contains(data[i])) {
                    selected[count] = i;
                    ++count;
                }
            }

            this.nodesList.setSelectedIndices(selected);
        }
    }
    
    private void setModel(String modelName,MapDataModel m) {
        this.currentModel = m;
        this.currentModelName = modelName;
        
        // don't want to reset control in the model, just our buttons...  We 
        // can figure out what the settings for the
        // comboboxes and buttons, etc should be by what's set in the model.
        // last: tell the map about the new model:
        // no, do this first!  then the new listener in the map updates 
        // right away and redraws
        Dataset ds = (Dataset)datasets.get(this.currentModelName);
        map.setModel(this.currentModel);
        // we tell it about this so that we can pass back and forth selection
        // info...
        map.setControlPanel(this);
        
        // a bit of dynamic trickery: remove all components from indexPanel,
        // and add the necessary ones specified by this model.  If this model
        // hasn't been swapped in yet, create them and a single listener for
        // all comboboxes, then add them in descending order to the indexPanel.
        indexPanel.removeAll();
        
        Vector tv = (Vector)modelIndexComponents.get(modelName);
        if (tv == null) {
            tv = new Vector();
            modelIndexComponents.put(modelName,tv);
            
            // populate the vector...
            String[] indices = m.getData().getIndices();
            final JComboBox[] jcArray = new JComboBox[indices.length];
            
            for (int i = 0; i < indices.length; ++i) {
                JLabel jl = new JLabel();
                jl.setText(indices[i] + ":");
                jl.setFont(new java.awt.Font("Dialog",0,10));
                
                JComboBox jc = new JComboBox();
                // bookkeep for later, so that we can use a single listener
                // for all n comboboxen...
                jcArray[i] = jc;
                jc.setModel(new javax.swing.DefaultComboBoxModel(m.getData().getIndexValues(indices[i])));
                jc.setFont(new java.awt.Font("Dialog",1,10));
                
                // add both to the vector:
                tv.add(jl);
                tv.add(jc);
            }
            
            // now create a single listener for these n comboboxen:
            //modelIndexComponents
            ActionListener tal = new java.awt.event.ActionListener() {
                JComboBox[] ijcArray = jcArray;
                
                public void actionPerformed(java.awt.event.ActionEvent evt) {
                    String[] newIndexValues = new String[ijcArray.length];
                    for (int i = 0; i < newIndexValues.length; ++i) {
                        newIndexValues[i] = (String)ijcArray[i].getSelectedItem();
                    }
                    
                    // now change the selection:
                    // actually using the value of currentModel is a bit of a race
                    // condition for fast clicks... but shouldn't be bad ever.
                    currentModel.setIndexValues(newIndexValues);
                }
            };
            
            for (int i = 0; i < indices.length; ++i) {
                jcArray[i].addActionListener(tal);
            }
            
            
        }
        // now add everything in the vector to indexPanel:
        int cy = 0;
        GridBagConstraints gc = new GridBagConstraints();
        gc.gridx = 0;
        gc.insets = new Insets(2,16,2,2);
        gc.anchor = GridBagConstraints.NORTHWEST;
        gc.fill = GridBagConstraints.HORIZONTAL;
        gc.weightx = 1.0;
        
        for (Enumeration e1 = tv.elements(); e1.hasMoreElements(); ) {
            JComponent jc = (JComponent)e1.nextElement();
            
            gc.gridy = cy++;
            indexPanel.add(jc,gc);
        }
        
        // set the property list:
        Vector tpv = m.getData().getProperties();
        this.primaryDisplayPropertyComboBox.setModel(new DefaultComboBoxModel(tpv));
        this.primaryDisplayPropertyComboBox.setSelectedItem(m.getCurrentProperty());
        
        
        this.nodesList.clearSelection();
        String[] nodes = this.currentModel.getData().getNodes();
        Arrays.sort(nodes);
        this.nodesList.setListData(nodes);
        //this.nodesList.setS
        
        // set up the floor combobox:
        Integer[] td = new Integer[ds.floor.length];
        for (int i = 0; i < ds.floor.length; ++i) {
            td[i] = new Integer(ds.floor[i]);
        }
        this.floorComboBox.setModel(new DefaultComboBoxModel(td));
        this.floorComboBox.setSelectedItem(new Integer(m.getFloor()));
        
        // set up the scale slider:
        this.scaleSlider.setMinimum(m.getMinScale());
        this.scaleSlider.setMaximum(m.getMaxScale());
        this.scaleSlider.setValue(m.getScale());
        if (m.getMinScale() == m.getMaxScale()) {
            this.scaleSlider.setEnabled(false);
        }
        else {
            this.scaleSlider.setEnabled(true);
        }
        
        // finally, set the background
        map.setBackgroundImage(ds.getImage(m.getFloor(),m.getScale()));
    }
    
    // <editor-fold defaultstate="collapsed" desc=" Generated Code ">//GEN-BEGIN:initComponents
    private void initComponents() {
        java.awt.GridBagConstraints gridBagConstraints;

        buttonGroup = new javax.swing.ButtonGroup();
        limitButtonGroup = new javax.swing.ButtonGroup();
        nodesList = new javax.swing.JList();
        nodesLabel = new javax.swing.JLabel();
        nnTextField = new javax.swing.JTextField();
        modeAllRadioButton = new javax.swing.JRadioButton();
        indexPanel = new javax.swing.JPanel();
        powerLevelLabel = new javax.swing.JLabel();
        powerLevelComboBox = new javax.swing.JComboBox();
        selectBySrcRadioButton = new javax.swing.JRadioButton();
        selectByDstRadioButton = new javax.swing.JRadioButton();
        kBestNeighborsCheckBox = new javax.swing.JCheckBox();
        thresholdCheckBox = new javax.swing.JCheckBox();
        datasetLabel = new javax.swing.JLabel();
        datasetComboBox = new JComboBox(new DefaultComboBoxModel(modelNames));
        modeLabel = new javax.swing.JLabel();
        limitLabel = new javax.swing.JLabel();
        thresholdTextField = new javax.swing.JTextField();
        noneCheckBox = new javax.swing.JCheckBox();
        MSTRadioButton = new javax.swing.JRadioButton();
        otherOptionsLabel = new javax.swing.JLabel();
        noZeroLinksCheckBox = new javax.swing.JCheckBox();
        datasetParametersLabel = new javax.swing.JLabel();
        primaryDisplayPropertyLabel = new javax.swing.JLabel();
        primaryDisplayPropertyComboBox = new javax.swing.JComboBox();
        floorLabel = new javax.swing.JLabel();
        floorComboBox = new javax.swing.JComboBox();
        scaleLabel = new javax.swing.JLabel();
        scaleSlider = new javax.swing.JSlider();

        setLayout(new java.awt.GridBagLayout());

        setBorder(javax.swing.BorderFactory.createTitledBorder(null, "Options", javax.swing.border.TitledBorder.DEFAULT_JUSTIFICATION, javax.swing.border.TitledBorder.DEFAULT_POSITION, new java.awt.Font("Dialog", 0, 12)));
        setPreferredSize(new java.awt.Dimension(250, 247));
        nodesList.setBorder(javax.swing.BorderFactory.createLineBorder(new java.awt.Color(102, 102, 102)));
        nodesList.setFont(new java.awt.Font("Dialog", 0, 10));
        nodesList.setEnabled(false);
        nodesList.setMinimumSize(new java.awt.Dimension(160, 100));
        nodesList.setPreferredSize(new java.awt.Dimension(90, 120));
        nodesList.addListSelectionListener(new javax.swing.event.ListSelectionListener() {
            public void valueChanged(javax.swing.event.ListSelectionEvent evt) {
                nodesListValueChanged(evt);
            }
        });

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 24;
        gridBagConstraints.fill = java.awt.GridBagConstraints.BOTH;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.NORTHWEST;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.weighty = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(2, 16, 2, 2);
        add(nodesList, gridBagConstraints);

        nodesLabel.setFont(new java.awt.Font("Dialog", 0, 12));
        nodesLabel.setText("Select nodes:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 23;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.NORTHWEST;
        gridBagConstraints.insets = new java.awt.Insets(2, 2, 2, 2);
        add(nodesLabel, gridBagConstraints);

        nnTextField.setFont(new java.awt.Font("Dialog", 0, 10));
        nnTextField.setText("3");
        nnTextField.setMaximumSize(new java.awt.Dimension(35, 17));
        nnTextField.setPreferredSize(new java.awt.Dimension(40, 17));
        nnTextField.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                nnTextFieldActionPerformed(evt);
            }
        });

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 18;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.insets = new java.awt.Insets(2, 28, 2, 2);
        add(nnTextField, gridBagConstraints);

        buttonGroup.add(modeAllRadioButton);
        modeAllRadioButton.setFont(new java.awt.Font("Dialog", 0, 10));
        modeAllRadioButton.setMnemonic('a');
        modeAllRadioButton.setSelected(true);
        modeAllRadioButton.setLabel("Show all links");
        modeAllRadioButton.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                modeAllRadioButtonActionPerformed(evt);
            }
        });

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 11;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.insets = new java.awt.Insets(2, 16, 2, 2);
        add(modeAllRadioButton, gridBagConstraints);

        indexPanel.setLayout(new java.awt.GridBagLayout());

        powerLevelLabel.setFont(new java.awt.Font("Dialog", 0, 10));
        powerLevelLabel.setText("Power level:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.insets = new java.awt.Insets(2, 16, 2, 2);
        indexPanel.add(powerLevelLabel, gridBagConstraints);

        powerLevelComboBox.setFont(new java.awt.Font("Dialog", 1, 10));
        powerLevelComboBox.setPreferredSize(new java.awt.Dimension(60, 24));
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(2, 16, 2, 2);
        indexPanel.add(powerLevelComboBox, gridBagConstraints);

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 7;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        add(indexPanel, gridBagConstraints);

        buttonGroup.add(selectBySrcRadioButton);
        selectBySrcRadioButton.setFont(new java.awt.Font("Dialog", 0, 10));
        selectBySrcRadioButton.setMnemonic('s');
        selectBySrcRadioButton.setText("Select by source");
        selectBySrcRadioButton.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                selectBySrcRadioButtonActionPerformed(evt);
            }
        });

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 12;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.insets = new java.awt.Insets(2, 16, 2, 2);
        add(selectBySrcRadioButton, gridBagConstraints);

        buttonGroup.add(selectByDstRadioButton);
        selectByDstRadioButton.setFont(new java.awt.Font("Dialog", 0, 10));
        selectByDstRadioButton.setMnemonic('r');
        selectByDstRadioButton.setText("Select by receiver");
        selectByDstRadioButton.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                selectByDstRadioButtonActionPerformed(evt);
            }
        });

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 13;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.insets = new java.awt.Insets(2, 16, 2, 2);
        add(selectByDstRadioButton, gridBagConstraints);

        limitButtonGroup.add(kBestNeighborsCheckBox);
        kBestNeighborsCheckBox.setFont(new java.awt.Font("Dialog", 0, 10));
        kBestNeighborsCheckBox.setText("k best neighbors");
        kBestNeighborsCheckBox.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                kBestNeighborsCheckBoxActionPerformed(evt);
            }
        });

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 17;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.insets = new java.awt.Insets(2, 16, 2, 2);
        add(kBestNeighborsCheckBox, gridBagConstraints);

        limitButtonGroup.add(thresholdCheckBox);
        thresholdCheckBox.setFont(new java.awt.Font("Dialog", 0, 10));
        thresholdCheckBox.setText("all links above threshold");
        thresholdCheckBox.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                thresholdCheckBoxActionPerformed(evt);
            }
        });

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 19;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.insets = new java.awt.Insets(2, 16, 2, 2);
        add(thresholdCheckBox, gridBagConstraints);

        datasetLabel.setFont(new java.awt.Font("Dialog", 0, 12));
        datasetLabel.setText("Change dataset:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 0;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.insets = new java.awt.Insets(2, 2, 2, 2);
        add(datasetLabel, gridBagConstraints);

        datasetComboBox.setFont(new java.awt.Font("Dialog", 1, 10));
        datasetComboBox.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                datasetComboBoxActionPerformed(evt);
            }
        });

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 1;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(2, 16, 2, 2);
        add(datasetComboBox, gridBagConstraints);

        modeLabel.setFont(new java.awt.Font("Dialog", 0, 12));
        modeLabel.setText("Mode:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 10;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.insets = new java.awt.Insets(2, 2, 2, 2);
        add(modeLabel, gridBagConstraints);

        limitLabel.setFont(new java.awt.Font("Dialog", 0, 12));
        limitLabel.setText("Limit by:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 15;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.insets = new java.awt.Insets(2, 2, 2, 2);
        add(limitLabel, gridBagConstraints);

        thresholdTextField.setFont(new java.awt.Font("Dialog", 0, 10));
        thresholdTextField.setText("70");
        thresholdTextField.setPreferredSize(new java.awt.Dimension(40, 17));
        thresholdTextField.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                thresholdTextFieldActionPerformed(evt);
            }
        });

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 20;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.insets = new java.awt.Insets(2, 28, 2, 2);
        add(thresholdTextField, gridBagConstraints);

        limitButtonGroup.add(noneCheckBox);
        noneCheckBox.setFont(new java.awt.Font("Dialog", 0, 10));
        noneCheckBox.setSelected(true);
        noneCheckBox.setText("None");
        noneCheckBox.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                noneCheckBoxActionPerformed(evt);
            }
        });

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 16;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.insets = new java.awt.Insets(2, 16, 2, 2);
        add(noneCheckBox, gridBagConstraints);

        buttonGroup.add(MSTRadioButton);
        MSTRadioButton.setFont(new java.awt.Font("Dialog", 0, 10));
        MSTRadioButton.setText("Min Spanning Tree");
        MSTRadioButton.setEnabled(false);
        MSTRadioButton.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                MSTRadioButtonActionPerformed(evt);
            }
        });

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 14;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.insets = new java.awt.Insets(2, 16, 2, 2);
        add(MSTRadioButton, gridBagConstraints);

        otherOptionsLabel.setFont(new java.awt.Font("Dialog", 0, 12));
        otherOptionsLabel.setText("Other options:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 21;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.insets = new java.awt.Insets(2, 2, 2, 2);
        add(otherOptionsLabel, gridBagConstraints);

        noZeroLinksCheckBox.setFont(new java.awt.Font("Dialog", 0, 10));
        noZeroLinksCheckBox.setSelected(true);
        noZeroLinksCheckBox.setText("Never show 0% links");
        noZeroLinksCheckBox.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                noZeroLinksCheckBoxActionPerformed(evt);
            }
        });

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 22;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.insets = new java.awt.Insets(2, 16, 2, 2);
        add(noZeroLinksCheckBox, gridBagConstraints);

        datasetParametersLabel.setFont(new java.awt.Font("Dialog", 0, 12));
        datasetParametersLabel.setText("Dataset parameters:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 6;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.insets = new java.awt.Insets(2, 2, 2, 2);
        add(datasetParametersLabel, gridBagConstraints);

        primaryDisplayPropertyLabel.setFont(new java.awt.Font("Dialog", 0, 12));
        primaryDisplayPropertyLabel.setText("Primary display property:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 8;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.insets = new java.awt.Insets(2, 2, 2, 2);
        add(primaryDisplayPropertyLabel, gridBagConstraints);

        primaryDisplayPropertyComboBox.setFont(new java.awt.Font("Dialog", 1, 10));
        primaryDisplayPropertyComboBox.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                primaryDisplayPropertyComboBoxActionPerformed(evt);
            }
        });

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 9;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(2, 16, 2, 2);
        add(primaryDisplayPropertyComboBox, gridBagConstraints);

        floorLabel.setFont(new java.awt.Font("Dialog", 0, 12));
        floorLabel.setText("Change floor:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.insets = new java.awt.Insets(2, 2, 2, 2);
        add(floorLabel, gridBagConstraints);

        floorComboBox.setFont(new java.awt.Font("Dialog", 1, 10));
        floorComboBox.setPreferredSize(new java.awt.Dimension(60, 24));
        floorComboBox.addItemListener(new java.awt.event.ItemListener() {
            public void itemStateChanged(java.awt.event.ItemEvent evt) {
                floorComboBoxItemStateChanged(evt);
            }
        });

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(2, 16, 2, 2);
        add(floorComboBox, gridBagConstraints);

        scaleLabel.setFont(new java.awt.Font("Dialog", 0, 12));
        scaleLabel.setText("Zoom:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 4;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.insets = new java.awt.Insets(2, 2, 2, 2);
        add(scaleLabel, gridBagConstraints);

        scaleSlider.setMaximum(5);
        scaleSlider.setValue(3);
        scaleSlider.addChangeListener(new javax.swing.event.ChangeListener() {
            public void stateChanged(javax.swing.event.ChangeEvent evt) {
                scaleSliderStateChanged(evt);
            }
        });

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 5;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.insets = new java.awt.Insets(2, 16, 2, 2);
        add(scaleSlider, gridBagConstraints);

    }// </editor-fold>//GEN-END:initComponents

    private void floorComboBoxItemStateChanged(java.awt.event.ItemEvent evt) {//GEN-FIRST:event_floorComboBoxItemStateChanged
        this.currentModel.setFloor(((Integer)floorComboBox.getSelectedItem()).intValue());
        // have to cheat here...
        map.setBackgroundImage(this.currentModel.getDataset().getImage(this.currentModel.getFloor(),this.currentModel.getScale()));
    }//GEN-LAST:event_floorComboBoxItemStateChanged

    private void scaleSliderStateChanged(javax.swing.event.ChangeEvent evt) {//GEN-FIRST:event_scaleSliderStateChanged
        this.currentModel.setScale(scaleSlider.getValue());
        map.setBackgroundImage(this.currentModel.getDataset().getImage(this.currentModel.getFloor(),this.currentModel.getScale()));
    }//GEN-LAST:event_scaleSliderStateChanged

    private void primaryDisplayPropertyComboBoxActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_primaryDisplayPropertyComboBoxActionPerformed
        this.currentModel.setCurrentProperty((String)this.primaryDisplayPropertyComboBox.getSelectedItem());
    }//GEN-LAST:event_primaryDisplayPropertyComboBoxActionPerformed

    private void noZeroLinksCheckBoxActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_noZeroLinksCheckBoxActionPerformed
        boolean cval = this.currentModel.getOption(MapDataModel.OPTION_NO_ZERO_LINKS);
        if (noZeroLinksCheckBox.isSelected()) {
            if (!cval) {
                this.currentModel.setOption(MapDataModel.OPTION_NO_ZERO_LINKS, MapDataModel.OPTION_SET);
            }
        }
        else {
            if (cval) {
                this.currentModel.setOption(MapDataModel.OPTION_NO_ZERO_LINKS, MapDataModel.OPTION_UNSET);
            }
        }
    }//GEN-LAST:event_noZeroLinksCheckBoxActionPerformed

    private void MSTRadioButtonActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_MSTRadioButtonActionPerformed
        if (MSTRadioButton.isSelected() && !(currentModel.getMode() == MapDataModel.MODE_MST)) {
            currentModel.setMode(MapDataModel.MODE_MST);
            
            nodesList.setEnabled(false);
            // also want to disable all the limit checkboxes, since limits are meaningless here
            kBestNeighborsCheckBox.setEnabled(false);
            noneCheckBox.setEnabled(false);
            thresholdCheckBox.setEnabled(false);
            
            // ehhhh...
            //nnTextField.setEnabled(false);
            //thresholdTextField.setEnabled(false);
        }
    }//GEN-LAST:event_MSTRadioButtonActionPerformed

    private void nodesListValueChanged(javax.swing.event.ListSelectionEvent evt) {//GEN-FIRST:event_nodesListValueChanged
        // check if the selection changed;
        if (!nodesList.getValueIsAdjusting()) {
            // safe
            Object objs[] = nodesList.getSelectedValues();
            Vector tmp = new Vector();
            for (int i = 0; i < objs.length; ++i) {
                tmp.add(objs[i]);
            }
            this.currentModel.setSelection(tmp);
            this.map.controlPanelSetSelectedNodes(tmp);
        }
    }//GEN-LAST:event_nodesListValueChanged

    private void thresholdTextFieldActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_thresholdTextFieldActionPerformed
        Float threshold = null;
        boolean error = false;
        try {
            threshold = new Float(Float.parseFloat(this.thresholdTextField.getText()));
        }
        catch (NumberFormatException e) {
            //e.printStackTrace();
            error = true;
        }
        
        if (!currentModel.setThreshold(threshold)) {
            error = true;
        }
        
        if (error) {
            JOptionPane.showMessageDialog(this.getParent(),
                "Please enter a floating point number!",
                "Number Format Error",
                JOptionPane.ERROR_MESSAGE);
            thresholdTextField.setText(""+currentModel.getThreshold());
        }
    }//GEN-LAST:event_thresholdTextFieldActionPerformed

    private void thresholdCheckBoxActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_thresholdCheckBoxActionPerformed
        if (thresholdCheckBox.isSelected() && !(currentModel.getLimit() == MapDataModel.LIMIT_THRESHOLD)) {
            currentModel.setLimit(MapDataModel.LIMIT_THRESHOLD);
            
            nnTextField.setEnabled(false);
            thresholdTextField.setEnabled(true);
        }
    }//GEN-LAST:event_thresholdCheckBoxActionPerformed

    private void nnTextFieldActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_nnTextFieldActionPerformed
        int neighbors = -1;
        boolean error = false;
        try {
            neighbors = Integer.parseInt(this.nnTextField.getText());
        }
        catch (NumberFormatException e) {
            //e.printStackTrace();
            error = true;
        }
        
        if (!currentModel.setNeighborCount(neighbors)) {
            error = true;
        }
        
        if (error) {
            JOptionPane.showMessageDialog(this.getParent(),
                "Please enter a positive integer greater than zero!",
                "Number Format Error",
                JOptionPane.ERROR_MESSAGE);
            nnTextField.setText(""+currentModel.getNeighborCount());
        }
    }//GEN-LAST:event_nnTextFieldActionPerformed

    private void kBestNeighborsCheckBoxActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_kBestNeighborsCheckBoxActionPerformed
        if (kBestNeighborsCheckBox.isSelected() && !(currentModel.getLimit() == MapDataModel.LIMIT_N_BEST_NEIGHBOR)) {
            currentModel.setLimit(MapDataModel.LIMIT_N_BEST_NEIGHBOR);
            
            nnTextField.setEnabled(true);
            thresholdTextField.setEnabled(false);
        }
    }//GEN-LAST:event_kBestNeighborsCheckBoxActionPerformed

    private void noneCheckBoxActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_noneCheckBoxActionPerformed
        if (noneCheckBox.isSelected() && !(currentModel.getLimit() == MapDataModel.LIMIT_NONE)) {
            currentModel.setLimit(MapDataModel.LIMIT_NONE);
            
            nnTextField.setEnabled(false);
            thresholdTextField.setEnabled(false);
        }
    }//GEN-LAST:event_noneCheckBoxActionPerformed

    private void selectByDstRadioButtonActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_selectByDstRadioButtonActionPerformed
        if (selectByDstRadioButton.isSelected() && !(currentModel.getMode() == MapDataModel.MODE_SELECT_DST)) {
            currentModel.setMode(MapDataModel.MODE_SELECT_DST);
            
            nodesList.setEnabled(true);
            
            kBestNeighborsCheckBox.setEnabled(true);
            noneCheckBox.setEnabled(true);
            thresholdCheckBox.setEnabled(true);
            
            map.controlPanelSetSelectEnabled(true);
        }
    }//GEN-LAST:event_selectByDstRadioButtonActionPerformed

    private void selectBySrcRadioButtonActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_selectBySrcRadioButtonActionPerformed
        if (selectBySrcRadioButton.isSelected() && !(currentModel.getMode() == MapDataModel.MODE_SELECT_SRC)) {
            currentModel.setMode(MapDataModel.MODE_SELECT_SRC);
            
            nodesList.setEnabled(true);
            
            kBestNeighborsCheckBox.setEnabled(true);
            noneCheckBox.setEnabled(true);
            thresholdCheckBox.setEnabled(true);
            
            map.controlPanelSetSelectEnabled(true);
        }
    }//GEN-LAST:event_selectBySrcRadioButtonActionPerformed

    private void modeAllRadioButtonActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_modeAllRadioButtonActionPerformed
        if (modeAllRadioButton.isSelected() && !(currentModel.getMode() == MapDataModel.MODE_ALL)) {
            currentModel.setMode(MapDataModel.MODE_ALL);
            
            nodesList.setEnabled(false);
            
            kBestNeighborsCheckBox.setEnabled(true);
            noneCheckBox.setEnabled(true);
            thresholdCheckBox.setEnabled(true);
            
            map.controlPanelSetSelectEnabled(false);
        }
    }//GEN-LAST:event_modeAllRadioButtonActionPerformed

    private void datasetComboBoxActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_datasetComboBoxActionPerformed
        String modelName = (String)datasetComboBox.getSelectedItem();
        Dataset ds = (Dataset)datasets.get(modelName);
        setModel(modelName,ds.model);
    }//GEN-LAST:event_datasetComboBoxActionPerformed
    
    
    // Variables declaration - do not modify//GEN-BEGIN:variables
    private javax.swing.JRadioButton MSTRadioButton;
    private javax.swing.ButtonGroup buttonGroup;
    private javax.swing.JComboBox datasetComboBox;
    private javax.swing.JLabel datasetLabel;
    private javax.swing.JLabel datasetParametersLabel;
    private javax.swing.JComboBox floorComboBox;
    private javax.swing.JLabel floorLabel;
    private javax.swing.JPanel indexPanel;
    private javax.swing.JCheckBox kBestNeighborsCheckBox;
    private javax.swing.ButtonGroup limitButtonGroup;
    private javax.swing.JLabel limitLabel;
    private javax.swing.JRadioButton modeAllRadioButton;
    private javax.swing.JLabel modeLabel;
    private javax.swing.JTextField nnTextField;
    private javax.swing.JCheckBox noZeroLinksCheckBox;
    private javax.swing.JLabel nodesLabel;
    private javax.swing.JList nodesList;
    private javax.swing.JCheckBox noneCheckBox;
    private javax.swing.JLabel otherOptionsLabel;
    private javax.swing.JComboBox powerLevelComboBox;
    private javax.swing.JLabel powerLevelLabel;
    private javax.swing.JComboBox primaryDisplayPropertyComboBox;
    private javax.swing.JLabel primaryDisplayPropertyLabel;
    private javax.swing.JLabel scaleLabel;
    private javax.swing.JSlider scaleSlider;
    private javax.swing.JRadioButton selectByDstRadioButton;
    private javax.swing.JRadioButton selectBySrcRadioButton;
    private javax.swing.JCheckBox thresholdCheckBox;
    private javax.swing.JTextField thresholdTextField;
    // End of variables declaration//GEN-END:variables
    
}
