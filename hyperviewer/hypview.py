#!/usr/bin/env python
#
# EMULAB-COPYRIGHT
# Copyright (c) 2004 University of Utah and the Flux Group.
# All rights reserved.
#
# hypview - HyperViewer application.
# For description of script args, invoke with any dash arg or see the "usage:" message below.

##import pdb

import string
import sys

import hv
import exptToHv
from hvFrameUI import *
from OpenGL.GL import * 

from wxPython.wx import *
from wxPython.glcanvas import wxGLCanvas

# A wxPython application.
class hvApp(wxApp):
    
    ##
    # The app initialization.
    def OnInit(self):
	self.frame = hvFrame(None, -1, "wxHyperViewer", (100,0), (750,750))
	self.frame.app = self	# A back-reference here from the frame.
	self.openDialog = hvOpen(None, -1, "Open HyperViewer Data")
	self.openDialog.app = self
	self.usageDialog = UsageDialogUI(None, -1, "HyperViewer Usage")
	
	# Make it visible.
	self.frame.Show()
	self.SetTopWindow(self.frame)
	
	# Given command-line argument(s), attempt to read in a topology.
        filename = project = None
        # Any dash argument prints a usage message and exits.
	if len(sys.argv) == 2 and sys.argv[1][0] == '-': 
	    print '''Hyperviewer usage:
  No args - Starts up the GUI.  Use the File/Open menu item to read a topology.
  One arg - A .hyp file name.  Read it in and start the GUI, e.g.:
      ./hypview BigLan.hyp
  Two args - Project and experiment names in the database.
      Get the topology from XMLRPC, make a .hyp file, start as above.
      ./hypview testbed BigLan
  Three args - Project and experiment names, plus an optional root node name.
      ./hypview testbed BigLan clan'''
	    sys.exit()
            pass
        
        # One command-line argument: read from a .hyp file.
	elif len(sys.argv) == 2:
            filename = sys.argv[1]
	    print "Reading file:", filename
	    pass
        
        # Two args: read an experiment from the DB via XML-RPC, and make a .hyp file.
	elif len(sys.argv) == 3:
            project = sys.argv[1]
            experiment = sys.argv[2]
	    print "Getting project:", project + ", experiment:", experiment
	    filename = exptToHv.getExperiment(project, experiment)
	    pass

        # Three args: experiment from database, with optional graph root node.
	elif len(sys.argv) == 4:
            project = sys.argv[1]
            experiment = sys.argv[2]
            root = sys.argv[3]
	    print "Getting project:", project + ", experiment:", experiment \
		  + ", root node:", root
	    filename = exptToHv.getExperiment(project, experiment, root)
            pass

        if filename:
            if filename == 2:
                exptError = "There is no experiment " + project + "/" + experiment
                print exptError
                pass
            else:
                self.frame.ReadTopFile("wxHyperViewer", filename)
                pass
            pass
        elif project:
            print "Failed to get experiment from database."
            pass

	return True
    pass

# The semantics of the UI of the application.
class hvFrame(hvFrameUI):
    
    ##
    # Frame initialization.
    def __init__(self, *args, **kwds):
	# Set up the wxGlade part.
	hvFrameUI.__init__(self, *args, **kwds)
	
	self.vwr = None		# Load data under the File menu now.

	# Control events.  (HyperViewer events are connected after loading data.)
	EVT_CHECKBOX(self.DrawSphere, -1, self.OnDrawSphere)
	EVT_CHECKBOX(self.DrawNodes, -1, self.OnDrawNodes)
	EVT_CHECKBOX(self.DrawLinks, -1, self.OnDrawLinks)
	EVT_CHECKBOX(self.KeepAspect, -1, self.OnKeepAspect)
	EVT_CHECKBOX(self.LabelToRight, -1, self.OnLabelToRight)
	EVT_BUTTON(self.GoToTop, -1, self.OnGoToTop)
	EVT_BUTTON(self.ShowLinksIn, -1, self.OnShowLinksIn)
	EVT_BUTTON(self.HideLinksIn, -1, self.OnHideLinksIn)
	EVT_BUTTON(self.ShowLinksOut, -1, self.OnShowLinksOut)
	EVT_BUTTON(self.HideLinksOut, -1, self.OnHideLinksOut)
	EVT_BUTTON(self.HelpButton, -1, self.OnUsage)
	EVT_COMBOBOX(self.LabelsMode, -1, self.OnLabelsMode)
	EVT_MENU(self, 1, self.OnOpen)
	EVT_MENU(self, 2, self.OnExit)
	EVT_MENU(self, 3, self.OnUsage)
	EVT_SPINCTRL(self.CountGenNode, -1, self.OnCountGenNode)
	EVT_SPINCTRL(self.CountGenLink, -1, self.OnCountGenLink)
	
	# Other events.
	EVT_IDLE(self.hypView, self.OnIdle)
	EVT_PAINT(self.hypView, self.OnPaint)
	EVT_CLOSE(self, self.OnClose)
	
	pass
    
    ##
    # Read in a topology file and instantiate the C++ object.
    def ReadTopFile(self, name, file):
	self.SetTitle(name + " " + file)
	
	# Instantiate the SWIG'ed C++ object HypView object.
	self.hypView.SetCurrent()   # Select the OpenGL Graphics Context from the wxGLCanvas.
	self.vwr = hv.hvmain([name, file])	# Load the data into the graph.
	self.hypView.SwapBuffers()		# Make the sphere visible.
	self.DrawGL()				# Show the graph.
	
	# Don't connect up the mouse events before the HyperView data is loaded!
	EVT_LEFT_DOWN(self.hypView, self.OnClick)	
	EVT_LEFT_UP(self.hypView, self.OnClick)
	EVT_MIDDLE_DOWN(self.hypView, self.OnClick)
	EVT_MIDDLE_UP(self.hypView, self.OnClick)
	EVT_MOTION(self.hypView, self.OnMove)
	
	pass	
    
    ##
    # Draw the OpenGL content and make it visible.
    def DrawGL(self):
	##print "in DrawGL"
	self.vwr.drawFrame()
	self.hypView.SwapBuffers()
	pass	
    
    ##
    # The GUI displays information about the currently selected node.
    def SelectedNode(self, node):
	##print node
	if string.find(node, "|") == -1:	# Links are "node1|node2".
	    self.NodeName.Clear()
	    self.NodeName.WriteText(node)
	    self.ChildCount.SetLabel(str(self.vwr.getChildCount(node)))
	    self.LabelLinksIn.SetLabel(
		"    Non-tree Links in:  " + str(self.vwr.getIncomingCount(node)))
	    self.LabelLinksOut.SetLabel(
		"    Non-tree Links out:  " + str(self.vwr.getOutgoingCount(node)))
	pass
    
    ##
    # Event handling for the HyperViewer canvas.
    # Check boxes control boolean state.
    def OnDrawSphere(self, cmdEvent):
	self.vwr.setSphere(self.DrawSphere.IsChecked())
	self.DrawGL()
    def OnDrawNodes(self, cmdEvent):
	self.vwr.setDrawNodes(self.DrawNodes.IsChecked())
	self.DrawGL()
    def OnDrawLinks(self, cmdEvent):
	self.vwr.setDrawLinks(self.DrawLinks.IsChecked())
	self.DrawGL()
    def OnKeepAspect(self, cmdEvent):
	self.vwr.setKeepAspect(self.KeepAspect.IsChecked())
	self.DrawGL()
    def OnLabelToRight(self, cmdEvent):
	self.vwr.setLabelToRight(self.LabelToRight.IsChecked())
	self.DrawGL()
    
    ##
    # Buttons issue commands.
    def OnGoToTop(self, buttonEvent):
	self.vwr.gotoCenterNode(0)
	self.SelectedNode(hv.getSelected())
	self.DrawGL()
    def OnShowLinksIn(self, buttonEvent):
	self.vwr.setDrawBackTo(hv.getSelected(), 1, self.DescendLinksIn.IsChecked())
	self.DrawGL()
    def OnHideLinksIn(self, buttonEvent):
	self.vwr.setDrawBackTo(hv.getSelected(), 0, self.DescendLinksIn.IsChecked())
	self.DrawGL()
    def OnShowLinksOut(self, buttonEvent):
	self.vwr.setDrawBackFrom(hv.getSelected(), 1, self.DescendLinksOut.IsChecked())
	self.DrawGL()
    def OnHideLinksOut(self, buttonEvent):
	self.vwr.setDrawBackFrom(hv.getSelected(), 0, self.DescendLinksOut.IsChecked())
	self.DrawGL()
    
    ##
    # Combo boxes select between alternatives.
    def OnLabelsMode(self, cmdEvent):
	which = self.LabelsMode.GetSelection()
	if which != -1:
	    self.vwr.setLabels(which)
	    self.DrawGL()
	    pass
	pass
    
    ##
    # Spin controls set integer state.
    def OnCountGenNode(self, spinEvent):
	self.vwr.setGenerationNodeLimit(self.CountGenNode.GetValue())
	self.DrawGL()
    def OnCountGenLink(self, spinEvent):
	self.vwr.setGenerationLinkLimit(self.CountGenLink.GetValue())
	self.DrawGL()
    
    ##
    # Menu items issue commands from the menu bar.
    def OnExit(self, cmdEvent):
	self.app.ExitMainLoop()
    def OnOpen(self, cmdEvent):
	self.app.openDialog.Show()
    def OnUsage(self, cmdEvent):
	self.app.usageDialog.Show()
    
    ##
    # Mouse click events.
    def OnClick(self, mouseEvent):
	# Encode mouse button events for HypView.
	btnNum = -1
        
        # Left mouse button for X-Y motion of the hyperbolic center.
	if mouseEvent.LeftDown():
	    btnNum = 0
	    btnState = 0
	    pass
	elif mouseEvent.LeftUp():
	    btnNum = 0
	    btnState = 1
	    pass
        
        # Middle button for rotation of the hyperbolic space.
	elif mouseEvent.MiddleDown():
	    btnNum = 1
	    btnState = 0
	    pass
	elif mouseEvent.MiddleUp():
	    btnNum = 1
	    btnState = 1
	    pass
	
        # Left button with control or shift held down is also rotation.
        if btnNum == 0 and ( mouseEvent.ControlDown() or mouseEvent.ShiftDown() ):
            btnNum = 1
            pass
        
	# Handle mouse clicks in HypView.
	if btnNum != -1:
	    self.vwr.mouse(btnNum, btnState, mouseEvent.GetX(), mouseEvent.GetY(), 0, 0)
	    self.vwr.redraw()
	    self.hypView.SwapBuffers()
	    
	    # If a pick occurred, the current node name has changed.
	    self.SelectedNode(hv.getSelected())
	    pass
	pass
    
    ##
    # Mouse motion events.
    def OnMove(self, mouseEvent):
	# Hyperviewer calls motion when a mouse button is clicked "active"
	if mouseEvent.LeftIsDown() or mouseEvent.MiddleIsDown():
	    self.vwr.motion(mouseEvent.GetX(), mouseEvent.GetY(), 0, 0)
	    pass
	else:
	    # "passive" mouse motion is when there is no button clicked.
	    self.vwr.passive(mouseEvent.GetX(), mouseEvent.GetY(), 0, 0)
	    pass
	self.vwr.redraw()
	self.hypView.SwapBuffers()
	pass
    
    ##
    # Other events generated by the event toploop and window system interface.
    def OnIdle(self, idleEvent):
	if self.vwr:
	    self.vwr.idle()
	    pass
	self.hypView.SwapBuffers()
	###idleEvent.RequestMore()
	pass
    
    def OnPaint(self, paintEvent):
	if self.vwr:
	    self.vwr.redraw()
	    pass
	self.hypView.SwapBuffers()
	pass

    def OnClose(self, cmdEvent):
	self.app.ExitMainLoop()
    pass

class hvOpen(OpenDialogUI):
    ##
    # Open dialog frame initialization.
    def __init__(self, *args, **kwds):
	# Set up the wxGlade part.
	OpenDialogUI.__init__(self, *args, **kwds)
	    
	EVT_BUTTON(self.OpenFile, -1, self.OnOpenFile)
	EVT_BUTTON(self.OpenExperiment, -1, self.OnOpenExperiment)
	pass
    
    ##
    # Get topology data from a file.
    def OnOpenFile(self, cmdEvent):
	self.app.frame.ReadTopFile("wxHyperViewer", self.FileToOpen.GetLineText(0))
	self.Hide();
	pass
    
    ##
    # Get topology data for an experiment from the database via XML-RPC.
    def OnOpenExperiment(self, cmdEvent):
	project = self.ProjectName.GetLineText(0)
	experiment = self.ExperimentName.GetLineText(0)
	root = self.ExperimentRoot.GetLineText(0)
	hypfile = exptToHv.getExperiment(project, experiment, root)
	if hypfile:
            if hypfile == 2:
                exptError = "There is no experiment " + project + "/" + experiment
                self.ExperimentMsg.SetLabel(exptError)
                print exptError
                return
            
	    self.app.frame.ReadTopFile("wxHyperViewer", hypfile)
            self.ExperimentMsg.SetLabel(" ")
	    self.Hide();
	pass
    
    pass

app = hvApp(0)		# Create the wxPython application.
app.MainLoop()		# Handle events.
