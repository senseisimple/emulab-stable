package protogeni.display
{
	// Found at: http://flexponential.com/2010/01/10/resizable-titlewindow-in-flex-4/
	import flash.display.DisplayObject;
	import flash.events.Event;
	import flash.events.MouseEvent;
	import flash.geom.Point;
	
	import mx.core.FlexGlobals;
	import mx.core.IFlexDisplayObject;
	import mx.core.UIComponent;
	import mx.events.SandboxMouseEvent;
	import mx.managers.PopUpManager;
	
	import spark.components.TitleWindow;
	import protogeni.display.skins.ResizableTitleWindowSkin;
	
	/**
	 *  ResizableTitleWindow is a TitleWindow with
	 *  a resize handle.
	 */
	public class ResizableTitleWindow extends TitleWindow
	{
		
		//--------------------------------------------------------------------------
		//
		//  Constructor
		//
		//--------------------------------------------------------------------------
		
		/**
		 *  Constructor.
		 */
		public function ResizableTitleWindow()
		{
			super();
			this.setStyle("skinClass", ResizableTitleWindowSkin);
			this.addEventListener("close", closeWindow);
		}
		
		public function showWindow():void
		{
			PopUpManager.addPopUp(this, FlexGlobals.topLevelApplication as DisplayObject, false);
			PopUpManager.centerPopUp(this);
		}
		
		public function closeWindow(event:Event = null):void
		{
			this.removeEventListener("close", closeWindow);
			PopUpManager.removePopUp(this as IFlexDisplayObject);
		}
		
		//--------------------------------------------------------------------------
		//
		//  Variables
		//
		//--------------------------------------------------------------------------
		
		private var clickOffset:Point;
		
		//--------------------------------------------------------------------------
		//
		//  Properties 
		//
		//--------------------------------------------------------------------------
		
		//----------------------------------
		//  Resize Handle
		//---------------------------------- 
		
		[SkinPart("false")]
		
		/**
		 *  The skin part that defines the area where
		 *  the user may drag to resize the window.
		 */
		public var resizeHandle:UIComponent;
		
		//--------------------------------------------------------------------------
		//
		//  Overridden methods: UIComponent, SkinnableComponent
		//
		//--------------------------------------------------------------------------
		
		/**
		 *  @private
		 */
		override protected function partAdded(partName:String, instance:Object) : void
		{
			super.partAdded(partName, instance);
			
			if (instance == resizeHandle)
			{
				resizeHandle.addEventListener(MouseEvent.MOUSE_DOWN, resizeHandle_mouseDownHandler);
			}
		}
		
		/**
		 *  @private
		 */
		override protected function partRemoved(partName:String, instance:Object):void
		{
			if (instance == resizeHandle)
			{
				resizeHandle.removeEventListener(MouseEvent.MOUSE_DOWN, resizeHandle_mouseDownHandler);
			}
			
			super.partRemoved(partName, instance);
		}
		
		//--------------------------------------------------------------------------
		// 
		// Event Handlers
		//
		//--------------------------------------------------------------------------
		
		private var prevWidth:Number;
		private var prevHeight:Number;
		
		protected function resizeHandle_mouseDownHandler(event:MouseEvent):void
		{
			if (enabled && isPopUp && !clickOffset)
			{        
				clickOffset = new Point(event.stageX, event.stageY);
				prevWidth = width;
				prevHeight = height;
				
				var sbRoot:DisplayObject = systemManager.getSandboxRoot();
				
				sbRoot.addEventListener(
					MouseEvent.MOUSE_MOVE, resizeHandle_mouseMoveHandler, true);
				sbRoot.addEventListener(
					MouseEvent.MOUSE_UP, resizeHandle_mouseUpHandler, true);
				sbRoot.addEventListener(
					SandboxMouseEvent.MOUSE_UP_SOMEWHERE, resizeHandle_mouseUpHandler)
			}
		}
		
		/**
		 *  @private
		 */
		protected function resizeHandle_mouseMoveHandler(event:MouseEvent):void
		{
			// during a resize, only the TitleWindow should get mouse move events
			// we don't check the target since this is on the systemManager and the target
			// changes a lot -- but this listener only exists during a resize.
			event.stopImmediatePropagation();
			
			if (!clickOffset)
			{
				return;
			}
			
			width = prevWidth + (event.stageX - clickOffset.x);
			height = prevHeight + (event.stageY - clickOffset.y);
			event.updateAfterEvent();
		}
		
		/**
		 *  @private
		 */
		protected function resizeHandle_mouseUpHandler(event:Event):void
		{
			clickOffset = null;
			prevWidth = NaN;
			prevHeight = NaN;
			
			var sbRoot:DisplayObject = systemManager.getSandboxRoot();
			
			sbRoot.removeEventListener(
				MouseEvent.MOUSE_MOVE, resizeHandle_mouseMoveHandler, true);
			sbRoot.removeEventListener(
				MouseEvent.MOUSE_UP, resizeHandle_mouseUpHandler, true);
			sbRoot.removeEventListener(
				SandboxMouseEvent.MOUSE_UP_SOMEWHERE, resizeHandle_mouseUpHandler);
		}
	}
}