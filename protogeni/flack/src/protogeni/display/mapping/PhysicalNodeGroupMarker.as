package protogeni.display.mapping
 {
	import flash.display.Sprite;
	import flash.events.MouseEvent;
	import flash.filters.DropShadowFilter;
	import flash.text.TextField;
	import flash.text.TextFieldAutoSize;
	
	import mx.core.DragSource;
	import mx.core.UIComponent;
	import mx.managers.DragManager;
	
	import protogeni.display.ColorUtil;
	import protogeni.resources.GeniManager;
	
	/**
	 * Visual part of the marker on the map
	 * 
	 * Code taken from the official tutorials for Google Maps API for Flash
	 * 
	 */
	public class PhysicalNodeGroupMarker extends UIComponent
	{
		public var marker:GeniMapMarker;
		public var managers:Vector.<GeniManager>;
		public var sprite:Sprite;
		
		private var allowDragging:Boolean = false;
		
		public function PhysicalNodeGroupMarker(newMarker:GeniMapMarker)
		{
			marker = newMarker;
			this.addEventListener(MouseEvent.MOUSE_MOVE, drag);
			this.addEventListener(MouseEvent.MOUSE_DOWN, mouseDown);
			this.addEventListener(MouseEvent.ROLL_OUT, mouseExit);
			managers = newMarker.showGroups.GetManagers();
			
			sprite = new Sprite();
			var loc:int;
			if(managers.length > 1) {
				var numShownManagers:int = Math.min(managers.length, 5);
				loc = 3*(numShownManagers-1);
				for(var i:int = numShownManagers-1; i > -1; i--) {
					sprite.graphics.lineStyle(2, ColorUtil.colorsMedium[managers[i].colorIdx], 1);
					sprite.graphics.beginFill(ColorUtil.colorsDark[managers[i].colorIdx], 1);
					sprite.graphics.drawRoundRect(loc, loc, 28, 28, 10, 10);
					loc -= 3;
				}
			} else {
				loc = Math.min(3*(newMarker.showGroups.collection.length-1), 6);
				while(loc > -1) {
					sprite.graphics.lineStyle(2, ColorUtil.colorsMedium[managers[0].colorIdx], 1);
					sprite.graphics.beginFill(ColorUtil.colorsDark[managers[0].colorIdx], 1);
					sprite.graphics.drawRoundRect(loc, loc, 28, 28, 10, 10);
					loc -= 3;
				}
			}
			
			var labelMc:TextField=new TextField();
			labelMc.textColor = ColorUtil.colorsLight[managers[0].colorIdx];
			labelMc.selectable=false;
			labelMc.border=false;
			labelMc.embedFonts=false;
			labelMc.mouseEnabled=false;
			labelMc.width=28;
			labelMc.height=28;
			labelMc.text = newMarker.showGroups.GetAll().length.toString();
			labelMc.autoSize=TextFieldAutoSize.CENTER;
			labelMc.y = 4;
			sprite.addChild(labelMc);
			this.addChild(sprite);
			
			// Apply the drop shadow filter to the box.
			var shadow:DropShadowFilter = new DropShadowFilter();
			shadow.distance = 5;
			shadow.angle = 25;
			this.filters = [shadow];
		
			buttonMode=true;
			useHandCursor = true;
		}
		
		private function mouseDown(event:MouseEvent):void {
			allowDragging = true;
		}
		
		private function mouseExit(event:MouseEvent):void {
			allowDragging = false;
		}
		
		public function drag(e:MouseEvent):void
		{
			if(allowDragging) {
				var ds:DragSource = new DragSource();
				ds.addData(marker, 'marker');
				var d:PhysicalNodeGroupMarker = new PhysicalNodeGroupMarker(marker)
				DragManager.doDrag(this, ds, e, d);
			}
		}
	}
}
