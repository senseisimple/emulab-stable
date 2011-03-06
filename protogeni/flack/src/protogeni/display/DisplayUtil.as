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
	import mx.controls.Alert;
	import mx.core.DragSource;
	import mx.managers.DragManager;
	import mx.managers.PopUpManager;
	
	import protogeni.StringUtil;
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
	
	import spark.components.Button;
	import spark.components.Label;
	
	import protogeni.display.components.XmlWindow;
	
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
			var logButton:Button = getButton();
			logButton.label = msg.name;
			logButton.toolTip = msg.groupId;
			if(msg.isError)
			{
				logButton.setStyle("icon",ImageUtil.errorIcon);
				logButton.styleName = "failedStyle";
			}
			else
			{
				if(msg.type == LogMessage.TYPE_START) {
					logButton.setStyle("icon",ImageUtil.rightIcon);
					//logButton.labelPlacement = ButtonLabelPlacement.LEFT;
				}
				else if(msg.type == LogMessage.TYPE_END) {
					logButton.setStyle("icon",ImageUtil.rightIcon);
					//logButton.labelPlacement = ButtonLabelPlacement.RIGHT;
				}
				else {
					logButton.setStyle("icon",ImageUtil.availableIcon);
				}
			}
			logButton.addEventListener(MouseEvent.CLICK,
				function openLog():void {
					DisplayUtil.viewLogMessage(msg);
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
			sButton.label = s.urn.name;
			sButton.addEventListener(MouseEvent.CLICK,
				function openSlice(event:MouseEvent):void {
					viewSlice(s);
				}
			);
			return sButton;
		}
		
		// Gets a button for the component manager
		public static function getGeniManagerButton(gm:GeniManager):Button {
			var cmButton:Button = getButton();
			cmButton.label = gm.Hrn;
			cmButton.toolTip = gm.Hrn + " at " + gm.Url;
			cmButton.setStyle("chromeColor", ColorUtil.colorsDark[gm.colorIdx]);
			cmButton.setStyle("color", ColorUtil.colorsLight[gm.colorIdx]);
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
					var newCollection:ArrayCollection = new ArrayCollection();
					for each(var node:PhysicalNode in s.nodes)
						newCollection.addItem(node);
					DisplayUtil.viewNodeCollection(newCollection);
				}
			);
			return cmButton;
		}
		
		// Gets a button for the physical node
		public static function getPhysicalNodeButton(n:PhysicalNode):Button {
			var nodeButton:Button = getButton(DisplayUtil.assignAvailabilityIcon(n));
			nodeButton.label = n.name;
			nodeButton.toolTip = n.name + " on " + n.manager.Hrn;
			nodeButton.setStyle("chromeColor", ColorUtil.colorsDark[n.manager.colorIdx]);
			nodeButton.setStyle("color", ColorUtil.colorsLight[n.manager.colorIdx]);
			nodeButton.setStyle("icon", assignAvailabilityIcon(n));
			nodeButton.addEventListener(MouseEvent.CLICK,
				function openNode(event:MouseEvent):void {
					DisplayUtil.viewPhysicalNode(n);
				}
			);
			nodeButton.addEventListener(MouseEvent.MOUSE_MOVE,
				function startDrag(e:MouseEvent):void {
					var dragInitiator:Button=Button(e.currentTarget);
					var ds:DragSource = new DragSource();
					ds.addData(n, 'physicalnode');
					DragManager.doDrag(dragInitiator, ds, e);
				}
			);
			return nodeButton;
		}
		
		public static function getPhysicalNodeGroupButton(ng:PhysicalNodeGroup):Button {
			var nodeButton:Button = getButton();
			if(ng.city.length == 0)
				nodeButton.label = ng.collection.length.toString() + " Nodes";
			else
				nodeButton.label = ng.city + " (" + ng.collection.length + ")";
			nodeButton.toolTip = ng.GetManager().Hrn;
			nodeButton.setStyle("chromeColor", ColorUtil.colorsDark[ng.GetManager().colorIdx]);
			nodeButton.setStyle("color", ColorUtil.colorsLight[ng.GetManager().colorIdx]);
			nodeButton.addEventListener(MouseEvent.CLICK,
				function openNode(event:MouseEvent):void {
					DisplayUtil.viewNodeGroup(ng);
				}
			);
			nodeButton.addEventListener(MouseEvent.MOUSE_MOVE,
				function startDrag(e:MouseEvent):void {
					var dragInitiator:Button=Button(e.currentTarget);
					var ds:DragSource = new DragSource();
					ds.addData(ng, 'physicalnodegroup');
					DragManager.doDrag(dragInitiator, ds, e);
				}
			);
			return nodeButton;
		}
		
		// Gets a button for the physical node
		public static function getVirtualNodeButton(n:VirtualNode):Button {
			var nodeButton:Button = getButton();
			nodeButton.label = n.clientId;
			nodeButton.toolTip = n.clientId + " on " + n.manager.Hrn;
			nodeButton.setStyle("chromeColor", ColorUtil.colorsDark[n.manager.colorIdx]);
			nodeButton.setStyle("color", ColorUtil.colorsLight[n.manager.colorIdx]);
			nodeButton.addEventListener(MouseEvent.CLICK,
				function openNode(event:MouseEvent):void {
					DisplayUtil.viewVirtualNode(n);
				}
			);
			nodeButton.addEventListener(MouseEvent.MOUSE_MOVE,
				function startDrag(e:MouseEvent):void {
					var dragInitiator:Button=Button(e.currentTarget);
					var ds:DragSource = new DragSource();
					ds.addData(n, 'virtualnode');
					DragManager.doDrag(dragInitiator, ds, e);
				}
			);
			return nodeButton;
		}
		
		// Gets a button for a physical link
		public static function getPhysicalLinkWithInterfaceButton(ni:PhysicalNodeInterface, nl:PhysicalLink):Button {
			var linkButton:Button = getButton(ImageUtil.linkIcon);
			linkButton.label = StringUtil.shortenString(ni.id, 50);
			linkButton.toolTip = ni.id + " on " + nl.name;
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
			linkButton.label = pl.clientId;
			linkButton.setStyle("icon", ImageUtil.linkIcon);
			linkButton.addEventListener(MouseEvent.CLICK,
				function openLink(event:MouseEvent):void {
					DisplayUtil.viewVirtualLink(pl);
				}
			);
			return linkButton;
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
			var ngWindow:VirtualNodeWindow = new VirtualNodeWindow();
			ngWindow.showWindow();
			ngWindow.loadNode(sn.node);
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