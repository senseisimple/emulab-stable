
import java.util.*;
import java.awt.Image;
import javax.swing.*;
import javax.swing.event.*;
import java.beans.*;

public class ControlPanel extends javax.swing.JPanel {
    
    private String currentModelName;
    private MapDataModel currentModel;
    private NodeMapPanel map;
    private Hashtable models;
    private Hashtable mapImages;
    private Vector modelNames;
    
    // this is the biggest sack of bullshit I have ever seen in my life.
    // you have to have "no subcomponents in the default instance"
    // this is the dumbest fucking thing I have ever encountered in an 
    // editor!  They're supposed to HELP you, not fuck you over for a
    // frigging hour!!!
    public ControlPanel() {
        super();
        //this(null,null,null);
    }
    
    public ControlPanel(Hashtable models,Hashtable mapImages,NodeMapPanel map) {
        super();
        
        // this only exists so we can tell the map about new models.
        this.map = map;
        this.models = models;
        this.mapImages = mapImages;
        this.modelNames = new Vector();
        this.currentModelName = null;
        this.currentModel = null;
        
        String tmpModelName = null;
        MapDataModel tmpModel = null;
        
        for (Enumeration e1 = models.keys(); e1.hasMoreElements(); ) {
            String modelName = (String)e1.nextElement();
            MapDataModel m = (MapDataModel)models.get(modelName);
            
            modelNames.add(modelName);
            if (tmpModelName == null) {
                tmpModelName = modelName;
                tmpModel = m;
            }
        }
        
        initComponents();
        
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
        map.setBackgroundImage((Image)mapImages.get(this.currentModelName));
        map.setModel(this.currentModel);
        // we tell it about this so that we can pass back and forth selection
        // info...
        map.setControlPanel(this);
        
        // start with power levels...
        int[] pLs = currentModel.getData().getPowerLevels();
        Arrays.sort(pLs);
        
        Integer pLIs[] = new Integer[pLs.length];
        Integer current = null;
        for (int i = 0; i < pLs.length; ++i) {
            Integer ni = new Integer(pLs[i]);
            pLIs[i] = ni;
            if (current == null) {
                current = ni;
            }
        }
        
        this.powerLevelComboBox.setModel(new DefaultComboBoxModel(pLIs));
        
        if (this.currentModel.getPowerLevel() > 0) {
            powerLevelComboBox.setSelectedItem(new Integer(this.currentModel.getPowerLevel()));
        }
        else {
            powerLevelComboBox.setSelectedItem(current);
            this.currentModel.setPowerLevel(current.intValue());
        }
        
        this.nodesList.clearSelection();
        String[] nodes = this.currentModel.getData().getNodes();
        Arrays.sort(nodes);
        this.nodesList.setListData(nodes);
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

        setLayout(new java.awt.GridBagLayout());

        setBorder(new javax.swing.border.TitledBorder("Options"));
        setPreferredSize(new java.awt.Dimension(250, 247));
        nodesList.setBorder(new javax.swing.border.LineBorder(new java.awt.Color(102, 102, 102)));
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
        gridBagConstraints.gridy = 18;
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
        gridBagConstraints.gridy = 17;
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
        gridBagConstraints.gridy = 12;
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
        gridBagConstraints.gridy = 5;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.insets = new java.awt.Insets(2, 16, 2, 2);
        add(modeAllRadioButton, gridBagConstraints);

        powerLevelLabel.setFont(new java.awt.Font("Dialog", 0, 12));
        powerLevelLabel.setText("Power level:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 2;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.insets = new java.awt.Insets(2, 2, 2, 2);
        add(powerLevelLabel, gridBagConstraints);

        powerLevelComboBox.setPreferredSize(new java.awt.Dimension(60, 24));
        powerLevelComboBox.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                powerLevelComboBoxActionPerformed(evt);
            }
        });

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 3;
        gridBagConstraints.fill = java.awt.GridBagConstraints.HORIZONTAL;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.weightx = 1.0;
        gridBagConstraints.insets = new java.awt.Insets(2, 16, 2, 2);
        add(powerLevelComboBox, gridBagConstraints);

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
        gridBagConstraints.gridy = 6;
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
        gridBagConstraints.gridy = 7;
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
        gridBagConstraints.gridy = 11;
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
        gridBagConstraints.gridy = 13;
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

        datasetComboBox.setFont(new java.awt.Font("Dialog", 0, 10));
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
        gridBagConstraints.gridy = 4;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.insets = new java.awt.Insets(2, 2, 2, 2);
        add(modeLabel, gridBagConstraints);

        limitLabel.setFont(new java.awt.Font("Dialog", 0, 12));
        limitLabel.setText("Limit by:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 9;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.insets = new java.awt.Insets(2, 2, 2, 2);
        add(limitLabel, gridBagConstraints);

        thresholdTextField.setFont(new java.awt.Font("Dialog", 0, 10));
        thresholdTextField.setText("0.70");
        thresholdTextField.setPreferredSize(new java.awt.Dimension(40, 17));
        thresholdTextField.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
                thresholdTextFieldActionPerformed(evt);
            }
        });

        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 14;
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
        gridBagConstraints.gridy = 10;
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
        gridBagConstraints.gridy = 8;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.insets = new java.awt.Insets(2, 16, 2, 2);
        add(MSTRadioButton, gridBagConstraints);

        otherOptionsLabel.setFont(new java.awt.Font("Dialog", 0, 12));
        otherOptionsLabel.setText("Other options:");
        gridBagConstraints = new java.awt.GridBagConstraints();
        gridBagConstraints.gridx = 0;
        gridBagConstraints.gridy = 15;
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
        gridBagConstraints.gridy = 16;
        gridBagConstraints.anchor = java.awt.GridBagConstraints.WEST;
        gridBagConstraints.insets = new java.awt.Insets(2, 16, 2, 2);
        add(noZeroLinksCheckBox, gridBagConstraints);

    }
    // </editor-fold>//GEN-END:initComponents

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
        float threshold = -1.0f;
        boolean error = false;
        try {
            threshold = Float.parseFloat(this.thresholdTextField.getText());
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
                "Please enter a positive floating point number greater than 0.0 and less than or equal to 1.0!",
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

    private void powerLevelComboBoxActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_powerLevelComboBoxActionPerformed
        Integer elm = (Integer)powerLevelComboBox.getSelectedItem();
        if (elm != null) {
            currentModel.setPowerLevel(elm.intValue());
        }
    }//GEN-LAST:event_powerLevelComboBoxActionPerformed

    private void datasetComboBoxActionPerformed(java.awt.event.ActionEvent evt) {//GEN-FIRST:event_datasetComboBoxActionPerformed
        String modelName = (String)datasetComboBox.getSelectedItem();
        MapDataModel model = (MapDataModel)models.get(modelName);
        setModel(modelName,model);
    }//GEN-LAST:event_datasetComboBoxActionPerformed
    
    
    // Variables declaration - do not modify//GEN-BEGIN:variables
    private javax.swing.JRadioButton MSTRadioButton;
    private javax.swing.ButtonGroup buttonGroup;
    private javax.swing.JComboBox datasetComboBox;
    private javax.swing.JLabel datasetLabel;
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
    private javax.swing.JRadioButton selectByDstRadioButton;
    private javax.swing.JRadioButton selectBySrcRadioButton;
    private javax.swing.JCheckBox thresholdCheckBox;
    private javax.swing.JTextField thresholdTextField;
    // End of variables declaration//GEN-END:variables
    
}
