package protogeni.display
{
	import flash.display.CapsStyle;
	import flash.display.LineScaleMode;
	import flash.display.Sprite;
	import flash.events.MouseEvent;
	
	import mx.controls.Alert;
	import mx.core.UIComponent;
	import mx.graphics.IFill;
	import mx.graphics.SolidColor;
	
	import protogeni.resources.VirtualLink;
	
	import spark.components.Group;
	import spark.components.HGroup;
	import spark.primitives.Rect;
	
	public class SliceLink extends UIComponent
	{
		[Bindable]
		public static var NORMAL_COLOR:uint = 0x000000;
		[Bindable]
		public static var TUNNEL_COLOR:uint = 0x00ffff;
		[Bindable]
		public static var ION_COLOR:uint = 0xcc33cc;
		[Bindable]
		public static var GPENI_COLOR:uint = 0x0000ff;
		
		public static var INVALID_COLOR:uint = 0xff0000;
		public static var VALID_COLOR:uint = 0x00ff00;
		
		public static var color:uint;
		
		public var virtualLink:VirtualLink;
		public var startNode:SliceNode;
		public var endNode:SliceNode;
		public var established:Boolean = false;
		//public var removeButton:ImageButton;
		private var buttonGroup:HGroup;
		public var group:Group;
		public var groupColor:SolidColor;
		public var canvas:SliceCanvas;
		
		private var rawSprite:Sprite;
		
		public var lastId:String = "";
		
		public function SliceLink(newCanvas:SliceCanvas)
		{
			super();
			canvas = newCanvas;
			
			rawSprite = new Sprite();
			addChild(rawSprite);
			
			color = NORMAL_COLOR;
			groupColor = new SolidColor(color);
		}
		
		public function isForNodes(first:SliceNode, second:SliceNode):Boolean
		{
			return ((startNode == first && endNode == second)
				|| (startNode == second && endNode == first));
		}
		
		public function hasNode(node:SliceNode):Boolean
		{
			return startNode == node || endNode == node;
		}
		
		public function establishFromExisting(vl:VirtualLink):void
		{
			setLink(vl);
			
			group = new Group();
			var d:Rect = new Rect();
			d.percentHeight = 100;
			d.percentWidth = 100;
			d.fill = groupColor;
			group.addElement(d);
			buttonGroup = new HGroup();
			buttonGroup.gap = 2;
			buttonGroup.paddingBottom = 2;
			buttonGroup.paddingTop = 2;
			buttonGroup.paddingRight = 2;
			buttonGroup.paddingLeft = 2;
			buttonGroup.setStyle("backgroundColor", 0xCCCCCC);
			group.addElement(buttonGroup);
			
			var removeButton:ImageButton = new ImageButton();
			removeButton.setStyle("icon", ImageUtil.crossIcon);
			removeButton.addEventListener(MouseEvent.CLICK, removeLink);
			buttonGroup.addElement(removeButton);
			
			var infoButton:ImageButton = new ImageButton();
			infoButton.setStyle("icon", ImageUtil.infoIcon);
			infoButton.addEventListener(MouseEvent.CLICK, viewLink);
			buttonGroup.addElement(infoButton);
			
			canvas.addElement(group);
			canvas.allLinks.addItem(this);

			// For now just assume there's two ...
			startNode = this.canvas.allNodes.getForVirtualNode(virtualLink.firstNode);
			startNode.links.addItem(this);
			
			endNode = this.canvas.allNodes.getForVirtualNode(virtualLink.secondNode);
			endNode.links.addItem(this);

			established = true;
		}
		
		public function establish(start:SliceNode, end:SliceNode):Boolean
		{
			setLink(new VirtualLink(start.node.slivers[0]));
			if(virtualLink.establish(start.node, end.node))
			{
				establishFromExisting(virtualLink);
				return true;
			} else {
				virtualLink = null;
				return false;
			}
		}
		
		public function setLink(vl:VirtualLink):void
		{
			virtualLink = vl;
			lastId = vl.clientId;
		}
		
		public function removeLink(event:MouseEvent = null):void
		{
			virtualLink.remove();
			startNode.removeLink(this);
			endNode.removeLink(this);
			canvas.removeElement(group);
			canvas.removeElement(this);
			canvas.allLinks.removeItemAt(canvas.allLinks.getItemIndex(this));
		}
		
		public function viewLink(event:MouseEvent = null):void
		{
			DisplayUtil.viewVirtualLink(virtualLink);
		}
		
		public function drawEstablished():void
		{
			color = NORMAL_COLOR;
			switch(virtualLink.linkType) {
				case VirtualLink.TYPE_TUNNEL:
					color = TUNNEL_COLOR;
					break;
				case VirtualLink.TYPE_GPENI:
					color = GPENI_COLOR;
					break;
				case VirtualLink.TYPE_ION:
					color = ION_COLOR;
					break;
			}
			drawLink(startNode.getMiddleX(), startNode.getMiddleY(), endNode.getMiddleX(), endNode.getMiddleY());
			group.x = ((startNode.getMiddleX() + endNode.getMiddleX()) / 2) - (group.width/2 + 1);
			group.y = ((startNode.getMiddleY() + endNode.getMiddleY()) / 2) - (group.height/2);
		}
		
		public function drawEstablishing(startX:int, startY:int, endX:int, endY:int, ready:Boolean):void
		{
			color = ready ? VALID_COLOR : INVALID_COLOR;
			drawLink(startX, startY, endX, endY);
		}
		
		public function drawLink(startX:int, startY:int, endX:int, endY:int):void
		{
			rawSprite.graphics.clear();
			rawSprite.graphics.lineStyle(4, color, 1.0, true,
				LineScaleMode.NORMAL, CapsStyle.ROUND);
			rawSprite.graphics.moveTo(startX, startY);
			rawSprite.graphics.lineTo(endX, endY);
			
			groupColor.color = color;
		}
	}
}