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
 
 package pgmap
{
	import flash.events.MouseEvent;
	import flash.external.ExternalInterface;
	
	import mx.collections.ArrayCollection;
	import mx.controls.Button;
	import mx.core.Application;
	import mx.managers.PopUpManager;
	
	public class Common
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
        [Embed(source="../../images/tick.png")]
        public static var availableIcon:Class;

        [Bindable]
        [Embed(source="../../images/cross.png")]
        public static var notAvailableIcon:Class;
        
        [Bindable]
        [Embed(source="../../images/drive_network.png")]
        public static var ownedIcon:Class;
        
        [Bindable]
        [Embed(source="../../images/link.png")]
        public static var linkIcon:Class;
        
        [Bindable]
        [Embed(source="../../images/flag_green.png")]
        public static var flagGreenIcon:Class;
        
        [Bindable]
        [Embed(source="../../images/flag_red.png")]
        public static var flagRedIcon:Class;
        
        [Bindable]
        [Embed(source="../../images/flag_yellow.png")]
        public static var flagYellowIcon:Class;
        
        [Bindable]
        [Embed(source="../../images/flag_orange.png")]
        public static var flagOrangeIcon:Class;
        
        [Bindable]
        [Embed(source="../../images/error.png")]
        public static var errorIcon:Class;
        
        [Bindable]
        [Embed(source="../../images/exclamation.png")]
        public static var exclamationIcon:Class;
        
        // Gets an icon for a boolean value
        public static function assignIcon(val:Boolean):Class {
			if (val)
	            return availableIcon;
	        else
	            return notAvailableIcon;
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
	                return notAvailableIcon;
            }
        }
        
        // Get's the CM icon
        public static function assignIconForComponentManager(val:ComponentManager):Class {
			if(val.Status == ComponentManager.VALID)
				return availableIcon;
			else
				return notAvailableIcon;
        }
		
		// Takes the given bandwidth and creates a human readable string
		public static function kbsToString(bandwidth:Number):String {
			var bw:String = "";
			if(bandwidth < 1000) {
				return bandwidth + " Kb\\s"
			} else if(bandwidth < 1000000) {
				return bandwidth / 1000 + " Mb\\s"
			} else if(bandwidth < 1000000000) {
				return bandwidth / 1000000 + " Gb\\s"
			}
			return bw;
		}
		
		// Returns the main class
		public static function Main():pgmap {
			return mx.core.Application.application as pgmap;
		}
		
		// Makes the first letter uppercase
		public static function firstToUpper (phrase : String) : String {
			return phrase.substring(1, 0).toUpperCase()+phrase.substring(1);
		}
		
		public static function replaceString(original:String, find:String, replace:String):String {
			return original.split(find).join(replace);
		}
		
		public static function getDotString(name : String) : String {
			return replaceString(replaceString(name, ".", ""), "-", "");
		}
		
		// Shortens the given string to a length, taking out from the middle
		public static function shortenString(phrase : String, size : int) : String {
			if(phrase.length < size)
				return phrase;
			
			var removeChars:int = phrase.length - size + 3;
			var upTo:int = (phrase.length / 2) - (removeChars / 2);
			return phrase.substring(0, upTo) + "..." +  phrase.substring(upTo + removeChars);
		}
		
		// Gets a button for the slice
		public static function getSliceButton(s:Slice):Button {
			var sButton:Button = new Button();
			sButton.label = s.hrn;
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
			cmButton.setStyle("icon", Common.assignIconForComponentManager(cm));
			cmButton.addEventListener(MouseEvent.CLICK,
				function openComponentManager(event:MouseEvent):void {
					viewComponentManager(cm);
				}
			);
			return cmButton;
		}
		
		// Gets a button for the physical node
		public static function getNodeButton(n:PhysicalNode):Button {
			var nodeButton:Button = new Button();
			nodeButton.label = n.name;
			nodeButton.setStyle("icon", Common.assignAvailabilityIcon(n));
			nodeButton.addEventListener(MouseEvent.CLICK,
				function openNode(event:MouseEvent):void {
					viewNode(n);
				}
			);
			return nodeButton;
		}
		
		// Gets a button for a physical link
		public static function getLinkButton(ni:PhysicalNodeInterface, nl:PhysicalLink):Button {
			var linkButton:Button = new Button();
			linkButton.label = ni.id;
			linkButton.setStyle("icon", Common.linkIcon);
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
			linkButton.label = pl.virtualId;
			linkButton.setStyle("icon", Common.linkIcon);
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
	    	plWindow.main = Main();
	    	PopUpManager.addPopUp(plWindow, Main(), false);
       		PopUpManager.centerPopUp(plWindow);
       		plWindow.loadPointLink(pl);
	    }
		
		// Opens a physical link window
		public static function viewPhysicalLink(l:PhysicalLink):void {
			var lgWindow:PhysicalLinkAdvancedWindow = new PhysicalLinkAdvancedWindow();
	    	lgWindow.main = Main();
	    	PopUpManager.addPopUp(lgWindow, Main(), false);
       		PopUpManager.centerPopUp(lgWindow);
       		lgWindow.loadLink(l);
		}
		
		// Opens a group of physical links
		public static function viewPhysicalLinkCollection(lc:ArrayCollection):void {
			if(lc.length == 1)
				viewPhysicalLink(lc[0]);
			else {
				var lgWindow:PhysicalLinkGroupAdvancedWindow = new PhysicalLinkGroupAdvancedWindow();
		    	lgWindow.main = Main();
		    	PopUpManager.addPopUp(lgWindow, Main(), false);
	       		PopUpManager.centerPopUp(lgWindow);
	       		lgWindow.loadCollection(lc);
			}
		}
		
		// Opens a group of physical links
		public static function viewPhysicalLinkGroup(lg:PhysicalLinkGroup):void {
			var lgWindow:PhysicalLinkGroupAdvancedWindow = new PhysicalLinkGroupAdvancedWindow();
	    	lgWindow.main = Main();
	    	PopUpManager.addPopUp(lgWindow, Main(), false);
       		PopUpManager.centerPopUp(lgWindow);
       		lgWindow.loadGroup(lg);
		}
		
		// Opens a component manager in a window
		public static function viewComponentManager(cm:ComponentManager):void {
			var cmWindow:ComponentManagerAdvancedWindow = new ComponentManagerAdvancedWindow();
	    	cmWindow.main = Main();
	    	PopUpManager.addPopUp(cmWindow, Main(), false);
       		PopUpManager.centerPopUp(cmWindow);
       		cmWindow.loadCm(cm);
		}
		
		// Opens a physical node in a window
		public static function viewNode(n:PhysicalNode):void {
			var ngWindow:PhysicalNodeAdvancedWindow = new PhysicalNodeAdvancedWindow();
	    	ngWindow.main = Main();
	    	PopUpManager.addPopUp(ngWindow, Main(), false);
       		PopUpManager.centerPopUp(ngWindow);
       		ngWindow.loadNode(n);
		}
		
		// Opens a group of physical nodes in a window
		public static function viewNodeGroup(ng:PhysicalNodeGroup):void {
			var ngWindow:PhysicalNodeGroupAdvancedWindow = new PhysicalNodeGroupAdvancedWindow();
	    	ngWindow.main = Main();
	    	PopUpManager.addPopUp(ngWindow, Main(), false);
       		PopUpManager.centerPopUp(ngWindow);
       		ngWindow.loadGroup(ng);
		}
		
		// Opens a group of physical nodes in a window
		public static function viewNodeCollection(nc:ArrayCollection):void {
			if(nc.length == 1)
				viewNode(nc[0]);
			else {
				var ngWindow:PhysicalNodeGroupAdvancedWindow = new PhysicalNodeGroupAdvancedWindow();
		    	ngWindow.main = Main();
		    	PopUpManager.addPopUp(ngWindow, Main(), false);
	       		PopUpManager.centerPopUp(ngWindow);
	       		ngWindow.loadCollection(nc);
			}
		}
		
		// Opens a component manager in a window
		public static function viewSlice(s:Slice):void {
			var sWindow:SliceWindow = new SliceWindow();
			PopUpManager.addPopUp(sWindow, Main(), false);
			PopUpManager.centerPopUp(sWindow);
			sWindow.loadSlice(s);
		}
		
		public static  function getBrowserName():String
        {
            var browser:String;
            var browserAgent:String = ExternalInterface.call("function getBrowser(){return navigator.userAgent;}");
 
 			if(browserAgent == null)
 				return "Undefined";
            else if(browserAgent.indexOf("Firefox") >= 0)
                browser = "Firefox";
            else if(browserAgent.indexOf("Safari") >= 0)
                browser = "Safari";
            else if(browserAgent.indexOf("MSIE") >= 0)
                browser = "IE";
            else if(browserAgent.indexOf("Opera") >= 0)
                browser = "Opera";
            else
                browser = "Undefined";
            
            return (browser);
        }
	}
}