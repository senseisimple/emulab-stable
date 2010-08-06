package protogeni.display
{
	import flash.display.CapsStyle;
	import flash.display.LineScaleMode;
	import flash.display.Sprite;
	import flash.events.MouseEvent;
	
	import mx.core.UIComponent;
	import mx.effects.easing.Linear;
	
	import protogeni.resources.VirtualInterface;
	import protogeni.resources.VirtualLink;
	
	public class SliceLink extends UIComponent
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
		
		private var rawSprite:Sprite;
		
		public function SliceLink(newCanvas:SliceCanvas)
		{
			super();
			canvas = newCanvas;
			
			rawSprite = new Sprite();
			addChild(rawSprite);
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
			
			this.virtualLink = vl;
			
			removeButton = new ImageButton();
			removeButton.source = DisplayUtil.notAvailableIcon;
			removeButton.addEventListener(MouseEvent.CLICK, removeLink);
			canvas.addChild(removeButton);
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
			virtualLink = new VirtualLink(start.node.slivers[0]);
			if(virtualLink.establish(start.node, end.node))
			{
				establishFromExisting(virtualLink);
				return true;
			} else {
				virtualLink = null;
				return false;
			}
		}
		
		public function removeLink(event:MouseEvent = null):void
		{
			virtualLink.remove();
			startNode.removeLink(this);
			endNode.removeLink(this);
			canvas.removeChild(removeButton);
			canvas.removeChild(this);
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
			rawSprite.graphics.clear();
			rawSprite.graphics.lineStyle(4, color, 1.0, true,
				LineScaleMode.NORMAL, CapsStyle.ROUND);
			rawSprite.graphics.moveTo(startX, startY);
			rawSprite.graphics.lineTo(endX, endY);
			
			if(established)
			{
				rawSprite.graphics.moveTo(((startNode.getMiddleX() + endNode.getMiddleX()) / 2), ((startNode.getMiddleY() + endNode.getMiddleY()) / 2));
				rawSprite.graphics.lineStyle(2, color, 1.0);
				rawSprite.graphics.beginFill(color, 0.8);
				rawSprite.graphics.drawCircle(((startNode.getMiddleX() + endNode.getMiddleX()) / 2),
					((startNode.getMiddleY() + endNode.getMiddleY()) / 2),
					9);
			}
		}
	}
}