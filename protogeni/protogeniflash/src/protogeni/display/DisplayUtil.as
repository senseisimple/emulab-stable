/* GENIPUBLIC-COPYRIGHT
 * Copyright (c) 2009 University of Utah and the Flux Group.
 * All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software is hereby
 * granted provided that (1) source code retains these copyright, permission,
 * and disclaimer notices, and (2) redistributions including binaries
 * reproduce the notices in supporting documentation.
 *
 * THE UNIVERSITY OF UTAH ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  THE UNIVERSITY OF UTAH DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 */
 
 /* Common.as
 
    Functions used all around the project
*/
 
 package protogeni.display
{
	import flash.display.DisplayObject;
	import flash.events.MouseEvent;
	import flash.text.StyleSheet;
	
	import mx.collections.ArrayCollection;
	import mx.controls.Button;
	import mx.controls.Label;
	import mx.managers.PopUpManager;
	
	import protogeni.communication.Request;
	import protogeni.resources.ComponentManager;
	import protogeni.resources.PhysicalLink;
	import protogeni.resources.PhysicalLinkGroup;
	import protogeni.resources.PhysicalNode;
	import protogeni.resources.PhysicalNodeGroup;
	import protogeni.resources.PhysicalNodeInterface;
	import protogeni.resources.Slice;
	import protogeni.resources.VirtualLink;
	import protogeni.resources.VirtualNode;
	
	public class DisplayUtil
	{
		public static var successColor:String = "#0E8219";
		public static var failColor:String = "#FE0000";
		public static var waitColor:String = "#FF7F00";
		
		public static var hideColor:Object = 0xCCCCCC;
		public static var linkColor:Object = 0xFFCFD1;
		public static var linkBorderColor:Object = 0xFF00FF;
		public static var tunnelColor:uint = 0xFFAEAE;
		public static var tunnelBorderColor:Object = 0xFF0000;
		public static var nodeColor:Object = 0x092B9F;
		public static var nodeBorderColor:Object = 0xD2E1F0;
		
		public static var windowHeight:int = 400;
		public static var windowWidth:int = 700;
		
		// Embedded images used around the application
		[Bindable]
		[Embed(source="../../../images/chart_bar.png")]
		public static var statisticsIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/help.png")]
		public static var helpIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/user.png")]
		public static var userIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/user_gray.png")]
		public static var noUserIcon:Class;
		
		[Bindable]
        [Embed(source="../../../images/tick.png")]
        public static var availableIcon:Class;

        [Bindable]
        [Embed(source="../../../images/cross.png")]
        public static var crossIcon:Class;
        
        [Bindable]
        [Embed(source="../../../images/drive_network.png")]
        public static var ownedIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/drive_network.png")]
		public static var physicalNodeIcon:Class;
        
        [Bindable]
        [Embed(source="../../../images/link.png")]
        public static var linkIcon:Class;
        
        [Bindable]
        [Embed(source="../../../images/flag_green.png")]
        public static var flagGreenIcon:Class;
        
        [Bindable]
        [Embed(source="../../../images/flag_red.png")]
        public static var flagRedIcon:Class;
        
        [Bindable]
        [Embed(source="../../../images/flag_yellow.png")]
        public static var flagYellowIcon:Class;
        
        [Bindable]
        [Embed(source="../../../images/flag_orange.png")]
        public static var flagOrangeIcon:Class;
        
        [Bindable]
        [Embed(source="../../../images/error.png")]
        public static var errorIcon:Class;
        
        [Bindable]
        [Embed(source="../../../images/exclamation.png")]
        public static var exclamationIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/lightbulb.png")]
		public static var onIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/lightbulb_off.png")]
		public static var offIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/add.png")]
		public static var addIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/cancel.png")]
		public static var cancelIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/control_stop_blue.png")]
		public static var stopIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/control_play_blue.png")]
		public static var playIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/control_repeat_blue.png")]
		public static var repeatIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/delete.png")]
		public static var deleteIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/arrow_refresh.png")]
		public static var refreshIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/find.png")]
		public static var searchIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/map.png")]
		public static var mapIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/graph.png")]
		public static var graphIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/page.png")]
		public static var pageIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/page_code.png")]
		public static var pageCodeIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/page_white.png")]
		public static var pageWhiteIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/page_white_code.png")]
		public static var pageWhiteCodeIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/wand.png")]
		public static var actionIcon:Class;
		
		public static function getLabel(text:String):Label
		{
			var l:Label = new Label();
			l.text = text;
			return l;
		}

        // Gets an icon for a boolean value
        public static function assignIcon(val:Boolean):Class {
			if (val)
	            return availableIcon;
	        else
	            return crossIcon;
        }
        
        // Gets the icon for the given node
        public static function assignAvailabilityIcon(val:PhysicalNode):Class {
			if(val.virtualNodes != null && val.virtualNodes.length > 0)
				return ownedIcon;
            else
            {
	            if (val.available)
	                return availableIcon;
	            else
	                return cancelIcon;
            }
        }
        
        // Get's the CM icon
        public static function assignIconForComponentManager(val:ComponentManager):Class {
			if(val.Status == ComponentManager.VALID)
				return availableIcon;
			else
				return crossIcon;
        }
		
		public static function getLogMessageButton(msg:LogMessage):Button {
			var logButton:Button = new Button();
			logButton.label = msg.name;
			if(msg.isError)
			{
				logButton.setStyle("icon",DisplayUtil.errorIcon);
				logButton.styleName = "failedStyle";
			}
			else
				logButton.setStyle("icon",DisplayUtil.availableIcon);
			logButton.addEventListener(MouseEvent.CLICK,
				function openLog():void {
					var logw:LogMessageWindow = new LogMessageWindow();
					PopUpManager.addPopUp(logw, Main.Pgmap(), false);
					PopUpManager.centerPopUp(logw);
					logw.setMessage(msg);
				});
			return logButton;
		}
		
		public static function getRequestButton(r:Request):Button {
			var requestButton:Button = new Button();
			requestButton.label = r.name;
			requestButton.toolTip = r.details;
			requestButton.addEventListener(MouseEvent.CLICK,
				function openRequest(event:MouseEvent):void {
					viewRequest(r);
				}
			);
			return requestButton;
		}
		
		// Gets a button for the slice
		public static function getSliceButton(s:Slice):Button {
			var sButton:Button = new Button();
			if(s.hrn != null && s.hrn.length > 0)
				sButton.label = s.hrn;
			else
				sButton.label = s.urn;
			sButton.addEventListener(MouseEvent.CLICK,
				function openSlice(event:MouseEvent):void {
					viewSlice(s);
				}
			);
			return sButton;
		}
		
		// Gets a button for the component manager
		public static function getComponentManagerButton(cm:ComponentManager):Button {
			var cmButton:Button = new Button();
			cmButton.label = cm.Hrn;
			cmButton.toolTip = cm.Hrn + " at " + cm.Url;
			cmButton.setStyle("icon", DisplayUtil.assignIconForComponentManager(cm));
			cmButton.addEventListener(MouseEvent.CLICK,
				function openComponentManager(event:MouseEvent):void {
					viewComponentManager(cm);
				}
			);
			return cmButton;
		}
		
		// Gets a button for the physical node
		public static function getPhysicalNodeButton(n:PhysicalNode):Button {
			var nodeButton:Button = new Button();
			nodeButton.label = n.name;
			nodeButton.toolTip = n.name + " on " + n.manager.Hrn;
			nodeButton.setStyle("icon", DisplayUtil.assignAvailabilityIcon(n));
			nodeButton.addEventListener(MouseEvent.CLICK,
				function openNode(event:MouseEvent):void {
					viewPhysicalNode(n);
				}
			);
			return nodeButton;
		}
		
		// Gets a button for the physical node
		public static function getVirtualNodeButton(n:VirtualNode):Button {
			var nodeButton:Button = new Button();
			nodeButton.label = n.id;
			nodeButton.toolTip = n.id + " on " + n.manager.Hrn;
			nodeButton.addEventListener(MouseEvent.CLICK,
				function openNode(event:MouseEvent):void {
					viewVirtualNode(n);
				}
			);
			return nodeButton;
		}
		
		// Gets a button for a physical link
		public static function getPhysicalLinkWithInterfaceButton(ni:PhysicalNodeInterface, nl:PhysicalLink):Button {
			var linkButton:Button = new Button();
			linkButton.label = ni.id;
			linkButton.setStyle("icon", DisplayUtil.linkIcon);
			linkButton.addEventListener(MouseEvent.CLICK,
				function openLink(event:MouseEvent):void {
					viewPhysicalLink(nl);
				}
			);
			return linkButton;
		}
		
		// Gets a button for the virtual link
		public static function getVirtualLinkButton(pl:VirtualLink):Button {
			var linkButton:Button = new Button();
			linkButton.label = pl.id;
			linkButton.setStyle("icon", DisplayUtil.linkIcon);
			linkButton.addEventListener(MouseEvent.CLICK,
				function openLink(event:MouseEvent):void {
					viewVirtualLink(pl);
				}
			);
			return linkButton;
		}
		
		// Opens a virtual link window
		public static function viewVirtualLink(pl:VirtualLink):void {
	    	var plWindow:VirtualLinkAdvancedWindow = new VirtualLinkAdvancedWindow();
	    	plWindow.main = Main.Pgmap();
	    	PopUpManager.addPopUp(plWindow, Main.Pgmap(), false);
       		PopUpManager.centerPopUp(plWindow);
       		plWindow.loadPointLink(pl);
	    }
		
		// Opens a physical link window
		public static function viewPhysicalLink(l:PhysicalLink):void {
			var lgWindow:PhysicalLinkAdvancedWindow = new PhysicalLinkAdvancedWindow();
	    	lgWindow.main = Main.Pgmap();
	    	PopUpManager.addPopUp(lgWindow, Main.Pgmap(), false);
       		PopUpManager.centerPopUp(lgWindow);
       		lgWindow.loadLink(l);
		}
		
		// Opens a group of physical links
		public static function viewPhysicalLinkCollection(lc:ArrayCollection):void {
			if(lc.length == 1)
				viewPhysicalLink(lc[0]);
			else {
				var lgWindow:PhysicalLinkGroupAdvancedWindow = new PhysicalLinkGroupAdvancedWindow();
		    	lgWindow.main = Main.Pgmap();
		    	PopUpManager.addPopUp(lgWindow, Main.Pgmap(), false);
	       		PopUpManager.centerPopUp(lgWindow);
	       		lgWindow.loadCollection(lc);
			}
		}
		
		// Opens a group of physical links
		public static function viewPhysicalLinkGroup(lg:PhysicalLinkGroup):void {
			var lgWindow:PhysicalLinkGroupAdvancedWindow = new PhysicalLinkGroupAdvancedWindow();
	    	lgWindow.main = Main.Pgmap();
	    	PopUpManager.addPopUp(lgWindow, Main.Pgmap(), false);
       		PopUpManager.centerPopUp(lgWindow);
       		lgWindow.loadGroup(lg);
		}
		
		// Opens a component manager in a window
		public static function viewComponentManager(cm:ComponentManager):void {
			var cmWindow:ComponentManagerAdvancedWindow = new ComponentManagerAdvancedWindow();
	    	cmWindow.main = Main.Pgmap();
	    	PopUpManager.addPopUp(cmWindow, Main.Pgmap(), false);
       		PopUpManager.centerPopUp(cmWindow);
       		cmWindow.load(cm);
		}
		
		// Opens a physical node in a window
		public static function viewPhysicalNode(n:PhysicalNode):void {
			var ngWindow:PhysicalNodeAdvancedWindow = new PhysicalNodeAdvancedWindow();
	    	ngWindow.main = Main.Pgmap();
	    	PopUpManager.addPopUp(ngWindow, Main.Pgmap(), false);
       		PopUpManager.centerPopUp(ngWindow);
       		ngWindow.loadNode(n);
		}
		
		public static function viewVirtualNode(n:VirtualNode):void {
			var ngWindow:VirtualNodeAdvancedWindow = new VirtualNodeAdvancedWindow();
			PopUpManager.addPopUp(ngWindow, Main.Pgmap(), false);
			PopUpManager.centerPopUp(ngWindow);
			ngWindow.loadNode(n);
		}
		
		// Opens a group of physical nodes in a window
		public static function viewNodeGroup(ng:PhysicalNodeGroup):void {
			var ngWindow:PhysicalNodeGroupAdvancedWindow = new PhysicalNodeGroupAdvancedWindow();
	    	ngWindow.main = Main.Pgmap();
	    	PopUpManager.addPopUp(ngWindow, Main.Pgmap(), false);
       		PopUpManager.centerPopUp(ngWindow);
       		ngWindow.loadGroup(ng);
		}
		
		// Opens a group of physical nodes in a window
		public static function viewNodeCollection(nc:ArrayCollection):void {
			if(nc.length == 1)
				viewPhysicalNode(nc[0]);
			else {
				var ngWindow:PhysicalNodeGroupAdvancedWindow = new PhysicalNodeGroupAdvancedWindow();
		    	ngWindow.main = Main.Pgmap();
		    	PopUpManager.addPopUp(ngWindow, Main.Pgmap(), false);
	       		PopUpManager.centerPopUp(ngWindow);
	       		ngWindow.loadCollection(nc);
			}
		}
		
		// Opens a component manager in a window
		public static function viewSlice(s:Slice):void {
			var sWindow:SliceWindow = new SliceWindow();
			PopUpManager.addPopUp(sWindow, Main.Pgmap(), false);
			PopUpManager.centerPopUp(sWindow);
			sWindow.loadSlice(s);
		}
		
		public static function viewRequest(r:Request):void {
			var rWindow:RequestWindow = new RequestWindow();
			PopUpManager.addPopUp(rWindow, Main.Pgmap(), false);
			PopUpManager.centerPopUp(rWindow);
			rWindow.load(r);
		}
	}
}