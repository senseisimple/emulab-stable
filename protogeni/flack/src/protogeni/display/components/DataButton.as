package protogeni.display.components
{
	import flash.events.MouseEvent;
	
	import mx.core.DragSource;
	import mx.managers.DragManager;
	
	import protogeni.display.DisplayUtil;
	
	import spark.components.Button;
	
	public class DataButton extends Button
	{
		[Bindable]
		public var data:*;
		public var dataType:String;
		private var allowDragging:Boolean = false;
		
		public function DataButton(newLabel:String, newToolTip:String, img:Class = null, newData:* = null, newDataType:String = "")
		{
			super();
			if(newLabel == null || newLabel.length == 0)
				this.width = DisplayUtil.minComponentWidth;
			else
				this.label = newLabel;
			this.toolTip = newToolTip;
			this.height = DisplayUtil.minComponentHeight;
			this.data = newData;
			if(newData != null)
				this.addEventListener(MouseEvent.CLICK, click);
			if(img != null)
				this.setStyle("icon", img);
			if(newDataType != null && newDataType.length > 0) {
				dataType = newDataType;
				this.addEventListener(MouseEvent.MOUSE_MOVE, startDragging);
				this.addEventListener(MouseEvent.MOUSE_DOWN, mouseDown);
				this.addEventListener(MouseEvent.ROLL_OUT, mouseExit);
			}
		}
		
		private function click(event:MouseEvent):void {
			DisplayUtil.view(event.currentTarget.data);
		}
		
		private function mouseDown(event:MouseEvent):void {
			allowDragging = true;
		}
		
		private function mouseExit(event:MouseEvent):void {
			allowDragging = false;
		}

		private function startDragging(event:MouseEvent):void {
			if(allowDragging) {
				var ds:DragSource = new DragSource();
				ds.addData(event.currentTarget.data, event.currentTarget.dataType);
				DragManager.doDrag(Button(event.currentTarget), ds, event);
			}
		}
	}
}