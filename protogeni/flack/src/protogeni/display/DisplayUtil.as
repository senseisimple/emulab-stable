/* GENIPUBLIC-COPYRIGHT
 * Copyright (c) 2008-2011 University of Utah and the Flux Group.
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
 
package protogeni.display
{
	import mx.collections.ArrayCollection;
	import mx.managers.PopUpManager;
	
	import protogeni.StringUtil;
	import protogeni.communication.Request;
	import protogeni.display.components.DataButton;
	import protogeni.display.components.XmlWindow;
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
	
	import spark.components.Button;
	import spark.components.Label;
	
	/**
	 * Common functions for GUI stuff
	 * 
	 * @author mstrum
	 * 
	 */
	public final class DisplayUtil
	{
		public static const windowHeight:int = 400;
		public static const windowWidth:int = 700;
		public static const minComponentHeight:int = 24;
		public static const minComponentWidth:int = 24;
		
		public static function getLabel(text:String, bold:Boolean = false):Label
		{
			var l:Label = new Label();
			l.text = text;
			if(bold)
				l.setStyle("fontWeight", "bold");
			return l;
		}

        // Gets an icon for a boolean value
        public static function assignIcon(val:Boolean):Class {
			if (val)
	            return ImageUtil.availableIcon;
	        else
	            return ImageUtil.crossIcon;
        }
        
        // Gets the icon for the given node
        public static function assignAvailabilityIcon(val:PhysicalNode):Class {
			if(val.virtualNodes != null && val.virtualNodes.length > 0)
				return ImageUtil.ownedIcon;
            else
            {
	            if (val.available) {
					if(val.exclusive)
						return ImageUtil.exclusiveIcon;
					else
						return ImageUtil.sharedIcon;
				}
	            else
	                return ImageUtil.cancelIcon;
            }
        }
		
		public static function getLogMessageButton(msg:LogMessage):Button {
			var img:Class;
			if(msg.isError)
				img = ImageUtil.errorIcon;
			else if(msg.type == LogMessage.TYPE_START)
				img = ImageUtil.rightIcon;
			else if(msg.type == LogMessage.TYPE_END)
				img = ImageUtil.rightIcon;
			else
				img = ImageUtil.availableIcon;

			var logButton:DataButton = new DataButton(msg.name,
				msg.groupId,
				img,
				msg);
			
			if(msg.isError)
				logButton.styleName = "failedStyle";
			
			return logButton;
		}
		
		public static function getRequestButton(r:Request):Button {
			return new DataButton(r.name, r.details, null, r);
		}
		
		// Gets a button for the slice
		public static function getSliceButton(s:Slice):Button {
			return new DataButton(s.urn.name, s.urn.full, null, s);
		}
		
		// Gets a button for the component manager
		public static function getGeniManagerButton(gm:GeniManager):Button {
			var cmButton:DataButton = new DataButton(gm.Hrn,
													"@ " + gm.Url,
													null,
													gm,
													"manager");
			cmButton.setStyle("chromeColor", ColorUtil.colorsDark[gm.colorIdx]);
			cmButton.setStyle("color", ColorUtil.colorsLight[gm.colorIdx]);
			return cmButton;
		}
		
		// Gets a button for the component manager
		public static function getSiteButton(s:Site):Button {
			return new DataButton(s.id + " (" + s.name + ")",
									s.name,
									null,
									s,
									"site");
		}
		
		// Gets a button for the physical node
		public static function getPhysicalNodeButton(n:PhysicalNode):Button {
			var nodeButton:DataButton = new DataButton(n.name,
																"@" + n.manager.Hrn,
																DisplayUtil.assignAvailabilityIcon(n),
																n,
																"physicalnode");
			nodeButton.setStyle("chromeColor", ColorUtil.colorsDark[n.manager.colorIdx]);
			nodeButton.setStyle("color", ColorUtil.colorsLight[n.manager.colorIdx]);
			return nodeButton;
		}
		
		public static function getPhysicalNodeGroupButton(ng:PhysicalNodeGroup):Button {
			var newLabel:String;
			if(ng.city.length == 0)
				newLabel = ng.collection.length.toString() + " Nodes";
			else
				newLabel = ng.city + " (" + ng.collection.length + ")";
			var nodeButton:DataButton = new DataButton(newLabel,
														ng.GetManager().Hrn,
														null,
														ng,
														"physicalnodegroup");
			nodeButton.setStyle("chromeColor", ColorUtil.colorsDark[ng.GetManager().colorIdx]);
			nodeButton.setStyle("color", ColorUtil.colorsLight[ng.GetManager().colorIdx]);
			return nodeButton;
		}
		
		// Gets a button for the physical node
		public static function getVirtualNodeButton(n:VirtualNode):Button {
			var nodeButton:DataButton = new DataButton(n.clientId,
														"@"+n.manager.Hrn,
														null,
														n,
														"virtualnode");
			nodeButton.setStyle("chromeColor", ColorUtil.colorsDark[n.manager.colorIdx]);
			nodeButton.setStyle("color", ColorUtil.colorsLight[n.manager.colorIdx]);
			return nodeButton;
		}
		
		// Gets a button for a physical link
		public static function getPhysicalLinkWithInterfaceButton(ni:PhysicalNodeInterface, nl:PhysicalLink):Button {
			return new DataButton(StringUtil.shortenString(ni.id, 50),
				ni.id + " on " + nl.name,
				ImageUtil.linkIcon,
				nl);
		}
		
		// Gets a button for the virtual link
		public static function getVirtualLinkButton(vl:VirtualLink):Button {
			return new DataButton(vl.clientId,
				vl.clientId,
				ImageUtil.linkIcon,
				vl);
		}
		
		public static function view(data:*):void {
			if(data is PhysicalNode)
				DisplayUtil.viewPhysicalNode(data);
			else if(data is PhysicalNodeGroup)
				DisplayUtil.viewNodeGroup(data);
			else if(data is GeniManager)
				DisplayUtil.viewGeniManager(data);
			else if(data is VirtualNode)
				DisplayUtil.viewVirtualNode(data);
			else if(data is PhysicalLink)
				DisplayUtil.viewPhysicalLink(data);
			else if(data is PhysicalLinkGroup)
				DisplayUtil.viewPhysicalLinkGroup(data);
			else if(data is Slice)
				DisplayUtil.viewSlice(data);
			else if(data is SliceNode)
				DisplayUtil.viewSliceNode(data);
			else if(data is SliceLink)
				DisplayUtil.viewSliceLink(data);
			else if(data is Site)
				DisplayUtil.viewSite(data);
			else if(data is Request)
				DisplayUtil.viewRequest(data);
			else if(data is LogMessage)
				DisplayUtil.viewLogMessage(data);
		}
		
		public static function viewLogMessage(msg:LogMessage):void {
			var logw:LogMessageWindow = new LogMessageWindow();
			logw.showWindow();
			logw.setMessage(msg);
		}
		
		// Opens a virtual link window
		public static function viewVirtualLink(pl:VirtualLink):void {
	    	var plWindow:VirtualLinkWindow = new VirtualLinkWindow();
			plWindow.showWindow();
       		plWindow.loadLink(pl);
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
		
		public static function viewSliceNode(sn:SliceNode):void {
			viewVirtualNode(sn.node);
		}
		
		public static function viewSliceLink(sl:SliceLink):void {
			viewVirtualLink(sl.virtualLink);
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
			//try {
				sWindow.loadSlice(s);
			//} catch(e:Error) {
			//	LogHandler.appendMessage(new LogMessage("", "View slice fail", e.toString(), true, LogMessage.TYPE_END));
			//	Alert.show("Problem loading slice, try refreshing the page");
			//}
			
		}
		
		public static function viewSite(s:Site):void {
			var newCollection:ArrayCollection = new ArrayCollection();
			for each(var node:PhysicalNode in s.nodes)
			newCollection.addItem(node);
			DisplayUtil.viewNodeCollection(newCollection);
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
		
		public static function viewUserWindow():void {
			var userWindow:UserWindow = new UserWindow();
			userWindow.showWindow();
		}
		
		public static function viewXml(xml:XML, title:String):void {
			var xmlView:XmlWindow = new XmlWindow();
			xmlView.title = title;
			xmlView.showWindow();
			xmlView.loadXml(xml);
		}
		
		public static function getButton(img:Class = null, imgOnly:Boolean = false):Button {
			var b:Button = new Button();
			if(imgOnly)
				b.width = minComponentWidth;
			b.height = minComponentHeight;
			if(img != null)
				b.setStyle("icon", img);
			return b;
		}
		
		private static var initialUserWindow:InitialUserWindow = null;
		public static function viewInitialUserWindow():void {
			if(initialUserWindow == null)
				initialUserWindow = new InitialUserWindow();
			initialUserWindow.showWindow();
		}
	}
}