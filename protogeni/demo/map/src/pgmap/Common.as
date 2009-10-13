package pgmap
{
	import flash.events.MouseEvent;
	
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
		public static var nodeColor:Object = 0x092B9F;
		public static var nodeBorderColor:Object = 0xD2E1F0;
		
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
        [Embed(source="../../images/error.png")]
        public static var errorIcon:Class;
        
        public static function assignIcon(val:Boolean):Class {
			if (val)
	            return availableIcon;
	        else
	            return notAvailableIcon;
        }
        
        public static function assignAvailabilityIcon(val:Node):Class {
			if(val.slice != null)
				return ownedIcon;
            else if (val.available)
                return availableIcon;
            else
                return notAvailableIcon;
        }
		
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
		
		public static function Main():pgmap {
			return mx.core.Application.application as pgmap;
		}
		
		public static function firstToUpper (phrase : String) : String {
			return phrase.substring(1, 0).toUpperCase()+phrase.substring(1);
		}
		
		public static function shortenString(phrase : String, size : int) : String {
			if(phrase.length < size)
				return phrase;
			
			var removeChars:int = phrase.length - size + 3;
			var upTo:int = (phrase.length / 2) - (removeChars / 2);
			return phrase.substring(0, upTo) + "..." +  phrase.substring(upTo + removeChars);
		}
		
		public static function getNodeButton(n:Node):Button {
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
		
		public static function getLinkButton(ni:NodeInterface, nl:Link):Button {
			var linkButton:Button = new Button();
			linkButton.label = ni.id;
			linkButton.setStyle("icon", Common.linkIcon);
			linkButton.addEventListener(MouseEvent.CLICK,
				function openLink(event:MouseEvent):void {
					viewLink(nl);
				}
			);
			return linkButton;
		}
		
		public static function viewLink(l:Link):void {
			var lgWindow:LinkGroupAdvancedWindow = new LinkGroupAdvancedWindow();
	    	lgWindow.main = Main();
	    	PopUpManager.addPopUp(lgWindow, Main(), false);
       		PopUpManager.centerPopUp(lgWindow);
       		
       		var ac:ArrayCollection = new ArrayCollection();
       		ac.addItem(l);
       		lgWindow.loadCollection(ac);
		}
		
		public static function viewNode(n:Node):void {
			var ngWindow:NodeGroupAdvancedWindow = new NodeGroupAdvancedWindow();
	    	ngWindow.main = Main();
	    	PopUpManager.addPopUp(ngWindow, Main(), false);
       		PopUpManager.centerPopUp(ngWindow);
       		
       		var ac:ArrayCollection = new ArrayCollection();
       		ac.addItem(n);
       		ngWindow.loadCollection(ac);
		}
	}
}