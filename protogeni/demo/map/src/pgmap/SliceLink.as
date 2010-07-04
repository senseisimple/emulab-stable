package pgmap
{
	import flash.display.CapsStyle;
	import flash.display.LineScaleMode;
	import flash.display.Sprite;
	import flash.events.MouseEvent;
	
	import mx.core.UIComponent;
	
	public class SliceLink extends Sprite
	{
		public static var ESTABLISHED_COLOR:uint = 0x0000ff;
		public static var TUNNEL_COLOR:uint = 0x00ffff;
		public static var INVALID_COLOR:uint = 0xff0000;
		public static var VALID_COLOR:uint = 0x00ff00;
		
		public var virtualLink:VirtualLink;
		public var startNode:SliceNode;
		public var endNode:SliceNode;
		public var established:Boolean = false;
		public var removeButton:ImageButton;
		public var canvas:SliceCanvas;
		
		public function SliceLink(newCanvas:SliceCanvas)
		{
			super();
			canvas = newCanvas;
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
		
		public function establish(start:SliceNode, end:SliceNode):Boolean
		{
			if(virtualLink.establish(start.node, end.node))
			{
				removeButton = new ImageButton();
				removeButton.source = Common.notAvailableIcon;
				removeButton.addEventListener(MouseEvent.CLICK, removeLink);
				canvas.addChild(removeButton);
				canvas.allLinks.addItem(this);
				startNode.links.addItem(this);
				endNode.links.addItem(this);
				established = true;
				startNode = start;
				endNode = end;
				return true;
			} else {
				return false;
			}
		}
		
		public function removeLink(event:MouseEvent):void
		{
			virtualLink.remove();
			startNode.removeLink(this);
			endNode.removeLink(this);
			canvas.removeChild(removeButton);
			canvas.removeRawChild(this);
			canvas.allLinks.removeItemAt(canvas.allLinks.getItemIndex(this));
		}
		
		public function drawEstablished():void
		{
			var color:uint = virtualLink.isTunnel() ? TUNNEL_COLOR : INVALID_COLOR;
			drawLink(startNode.getMiddleX(), startNode.getMiddleY(), endNode.getMiddleX(), endNode.getMiddleY(), color);
			removeButton.x = ((startNode.getMiddleX() + endNode.getMiddleX()) / 2) - (removeButton.width/2 + 1);
			removeButton.y = ((startNode.getMiddleY() + endNode.getMiddleY()) / 2) - (removeButton.height/2);
		}
		
		public function drawEstablishing(startX:int, startY:int, endX:int, endY:int, ready:Boolean):void
		{
			var color:uint = ready ? VALID_COLOR : INVALID_COLOR;
			drawLink(startX, startY, endX, endY, color);
		}
		
		public function drawLink(startX:int, startY:int, endX:int, endY:int, color:uint):void
		{
			graphics.clear();
			graphics.lineStyle(4, color, 1.0, true,
				LineScaleMode.NORMAL, CapsStyle.ROUND);
			graphics.moveTo(startX, startY);
			graphics.lineTo(endX, endY);
			
			if(established)
			{
				graphics.moveTo(((startNode.getMiddleX() + endNode.getMiddleX()) / 2), ((startNode.getMiddleY() + endNode.getMiddleY()) / 2));
				graphics.lineStyle(2, color, 1.0);
				graphics.beginFill(color, 0.8);
				graphics.drawCircle(((startNode.getMiddleX() + endNode.getMiddleX()) / 2),
					((startNode.getMiddleY() + endNode.getMiddleY()) / 2),
					9);
			}
		}
	}
}