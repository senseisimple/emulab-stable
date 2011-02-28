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
	import flash.events.MouseEvent;
	
	import mx.collections.ArrayCollection;
	import mx.controls.Button;
	import mx.controls.ButtonLabelPlacement;
	import mx.managers.PopUpManager;
	
	import protogeni.communication.Request;
	import protogeni.resources.GeniManager;
	import protogeni.resources.PhysicalLink;
	import protogeni.resources.PhysicalLinkGroup;
	import protogeni.resources.PhysicalNode;
	import protogeni.resources.PhysicalNodeGroup;
	import protogeni.resources.PhysicalNodeInterface;
	import protogeni.resources.PlanetlabAggregateManager;
	import protogeni.resources.ProtogeniComponentManager;
	import protogeni.resources.Site;
	import protogeni.resources.Slice;
	import protogeni.resources.VirtualLink;
	import protogeni.resources.VirtualNode;
	
	import spark.components.Label;
	
	public class DisplayUtil
	{
		public static var successColorString:String = "#0E8219";
		public static var failColorString:String = "#FE0000";
		public static var waitColorString:String = "#FF7F00";
		
		public static var successColor:uint = 0x0E8219;
		public static var failColor:uint = 0xFE0000;
		public static var waitColor:uint = 0xFF7F00;
		
		public static var hideColor:Object = 0xCCCCCC;
		public static var linkColor:Object = 0xFFCFD1;
		public static var linkBorderColor:Object = 0xFF00FF;
		public static var tunnelColor:uint = 0xFFAEAE;
		public static var tunnelBorderColor:Object = 0xFF0000;
		public static var nodeColor:Object = 0x092B9F;
		public static var nodeBorderColor:Object = 0xD2E1F0;
		
		public static var windowHeight:int = 400;
		public static var windowWidth:int = 700;
		public static var buttonHeight:int = 24;
		public static var buttonWidth:int = 24;
		
		// Embedded images used around the application
		[Bindable]
		[Embed(source="../../../images/chart_bar.png")]
		public static var statisticsIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/help.png")]
		public static var helpIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/arrow_left.png")]
		public static var leftIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/arrow_right.png")]
		public static var rightIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/arrow_up.png")]
		public static var upIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/arrow_down.png")]
		public static var downIcon:Class;
		
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
		[Embed(source="../../../images/control_pause_blue.png")]
		public static var pauseIcon:Class;
		
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
		public static var findIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/magnifier.png")]
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
		
		[Bindable]
		[Embed(source="../../../images/ssl_certificates.png")]
		public static var sslIcon:Class;
		
		[Bindable]
		[Embed(source="../../../images/keyboard.png")]
		public static var keyboardIcon:Class;
		
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
        public static function assignIconForGeniManager(val:GeniManager):Class {
			if(val.Status == GeniManager.STATUS_VALID)
				return availableIcon;
			else
				return crossIcon;
        }
		
		public static function getLogMessageButton(msg:LogMessage):Button {
			var logButton:Button = getButton();
			logButton.label = msg.name;
			logButton.toolTip = msg.groupId;
			if(msg.isError)
			{
				logButton.setStyle("icon",DisplayUtil.errorIcon);
				logButton.styleName = "failedStyle";
			}
			else
			{
				if(msg.type == LogMessage.TYPE_START) {
					logButton.setStyle("icon",DisplayUtil.rightIcon);
					logButton.labelPlacement = ButtonLabelPlacement.LEFT;
				}
				else if(msg.type == LogMessage.TYPE_END) {
					logButton.setStyle("icon",DisplayUtil.rightIcon);
					logButton.labelPlacement = ButtonLabelPlacement.RIGHT;
				}
				else {
					logButton.setStyle("icon",DisplayUtil.availableIcon);
				}
			}
			logButton.addEventListener(MouseEvent.CLICK,
				function openLog():void {
					var logw:LogMessageWindow = new LogMessageWindow();
					logw.showWindow();
					logw.setMessage(msg);
				});
			return logButton;
		}
		
		public static function getRequestButton(r:Request):Button {
			var requestButton:Button = getButton();
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
			var sButton:Button = getButton();
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
		public static function getGeniManagerButton(gm:GeniManager):Button {
			var cmButton:Button = getButton(DisplayUtil.assignIconForGeniManager(gm));
			cmButton.label = gm.Hrn;
			cmButton.toolTip = gm.Hrn + " at " + gm.Url;
			cmButton.addEventListener(MouseEvent.CLICK,
				function openGeniManager(event:MouseEvent):void {
					DisplayUtil.viewGeniManager(gm);
				}
			);
			return cmButton;
		}
		
		// Gets a button for the component manager
		public static function getSiteButton(s:Site):Button {
			var cmButton:Button = getButton();
			cmButton.label = s.id + " (" + s.name + ")";
			cmButton.addEventListener(MouseEvent.CLICK,
				function openGeniManager(event:MouseEvent):void {
					DisplayUtil.viewNodeCollection(s.nodes);
				}
			);
			return cmButton;
		}
		
		// Gets a button for the physical node
		public static function getPhysicalNodeButton(n:PhysicalNode):Button {
			var nodeButton:Button = getButton(DisplayUtil.assignAvailabilityIcon(n));
			nodeButton.label = n.name;
			nodeButton.toolTip = n.name + " on " + n.manager.Hrn;
			nodeButton.addEventListener(MouseEvent.CLICK,
				function openNode(event:MouseEvent):void {
					DisplayUtil.viewPhysicalNode(n);
				}
			);
			return nodeButton;
		}
		
		// Gets a button for the physical node
		public static function getVirtualNodeButton(n:VirtualNode):Button {
			var nodeButton:Button = getButton();
			nodeButton.label = n.id;
			nodeButton.toolTip = n.id + " on " + n.manager.Hrn;
			nodeButton.addEventListener(MouseEvent.CLICK,
				function openNode(event:MouseEvent):void {
					DisplayUtil.viewVirtualNode(n);
				}
			);
			return nodeButton;
		}
		
		// Gets a button for a physical link
		public static function getPhysicalLinkWithInterfaceButton(ni:PhysicalNodeInterface, nl:PhysicalLink):Button {
			var linkButton:Button = getButton(DisplayUtil.linkIcon);
			linkButton.label = ni.id;
			linkButton.addEventListener(MouseEvent.CLICK,
				function openLink(event:MouseEvent):void {
					DisplayUtil.viewPhysicalLink(nl);
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
					DisplayUtil.viewVirtualLink(pl);
				}
			);
			return linkButton;
		}
		
		// Opens a virtual link window
		public static function viewVirtualLink(pl:VirtualLink):void {
	    	var plWindow:VirtualLinkWindow = new VirtualLinkWindow();
			plWindow.showWindow();
       		plWindow.loadPointLink(pl);
	    }
		
		// Opens a physical link window
		public static function viewPhysicalLink(l:PhysicalLink):void {
			var lgWindow:PhysicalLinkWindow = new PhysicalLinkWindow();
			lgWindow.showWindow();
       		lgWindow.loadLink(l);
		}
		
		// Opens a group of physical links
		public static function viewPhysicalLinkCollection(lc:ArrayCollection):void {
			if(lc.length == 1)
				viewPhysicalLink(lc[0]);
			else {
				var lgWindow:PhysicalLinkGroupWindow = new PhysicalLinkGroupWindow();
				lgWindow.showWindow();
	       		lgWindow.loadCollection(lc);
			}
		}
		
		// Opens a group of physical links
		public static function viewPhysicalLinkGroup(lg:PhysicalLinkGroup):void {
			var lgWindow:PhysicalLinkGroupWindow = new PhysicalLinkGroupWindow();
			lgWindow.showWindow();
       		lgWindow.loadGroup(lg);
		}
		
		// Opens a component manager in a window
		public static function viewGeniManager(gm:GeniManager):void {
			if(gm is ProtogeniComponentManager) {
				var cmWindow:ProtogeniManagerWindow = new ProtogeniManagerWindow();
				cmWindow.showWindow();
				cmWindow.load(gm as ProtogeniComponentManager);
			} else if (gm is PlanetlabAggregateManager) {
				var plmWindow:PlanetlabManagerWindow = new PlanetlabManagerWindow();
				plmWindow.showWindow();
				plmWindow.load(gm as PlanetlabAggregateManager);
			}
		}
		
		// Opens a physical node in a window
		public static function viewPhysicalNode(n:PhysicalNode):void {
			var ngWindow:PhysicalNodeWindow = new PhysicalNodeWindow();
			ngWindow.showWindow();
       		ngWindow.loadNode(n);
		}
		
		public static function viewVirtualNode(n:VirtualNode):void {
			var ngWindow:VirtualNodeWindow = new VirtualNodeWindow();
			ngWindow.showWindow();
			ngWindow.loadNode(n);
		}
		
		// Opens a group of physical nodes in a window
		public static function viewNodeGroup(ng:PhysicalNodeGroup):void {
			var ngWindow:PhysicalNodeGroupWindow = new PhysicalNodeGroupWindow();
			ngWindow.showWindow();
       		ngWindow.loadGroup(ng);
		}
		
		// Opens a group of physical nodes in a window
		public static function viewNodeCollection(nc:ArrayCollection):void {
			if(nc.length == 1)
				viewPhysicalNode(nc[0]);
			else {
				var ngWindow:PhysicalNodeGroupWindow = new PhysicalNodeGroupWindow();
				ngWindow.showWindow();
	       		ngWindow.loadCollection(nc);
			}
		}
		
		// Opens a component manager in a window
		public static function viewSlice(s:Slice):void {
			var sWindow:SliceWindow = new SliceWindow();
			sWindow.showWindow();
			sWindow.loadSlice(s);
		}
		
		public static function viewRequest(r:Request):void {
			var rWindow:RequestWindow = new RequestWindow();
			rWindow.showWindow();
			rWindow.load(r);
		}
		
		public static function viewSearchWindow():void {
			var searchWindow:SearchWindow = new SearchWindow();
			searchWindow.showWindow();
		}
		
		public static function viewAboutWindow():void {
			var aboutWindow:AboutWindow = new AboutWindow();
			PopUpManager.addPopUp(aboutWindow, Main.Application(), false);
			PopUpManager.centerPopUp(aboutWindow);
		}
		
		public static function viewSlicesWindow():void {
			var slicesWindow:SlicesWindow = new SlicesWindow();
			slicesWindow.showWindow();
		}
		
		public static function viewUserWindow():void {
			var userWindow:UserWindow = new UserWindow();
			userWindow.showWindow();
		}
		
		public static function viewManagersWindow():void {
			var managersWindow:GeniManagersWindow = new GeniManagersWindow();
			managersWindow.showWindow();
		}
		
		public static function getButton(img:Class = null, imgOnly:Boolean = false):Button {
			var b:Button = new Button();
			if(imgOnly)
				b.width = buttonWidth;
			b.height = buttonHeight;
			if(img != null)
				b.setStyle("icon", img);
			return b;
		}
	}
}