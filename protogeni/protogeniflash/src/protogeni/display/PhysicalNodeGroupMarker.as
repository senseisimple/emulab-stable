/**
 * Code taken from the official tutorials for Google Maps API for Flash
 */
 
 package protogeni.display {

import flash.display.BitmapData;
import flash.display.Sprite;
import flash.events.MouseEvent;
import flash.filters.DropShadowFilter;
import flash.geom.Matrix;
import flash.text.TextField;
import flash.text.TextFieldAutoSize;

import mx.controls.Image;
import mx.core.DragSource;
import mx.core.UIComponent;
import mx.events.DragEvent;
import mx.managers.DragManager;

/**
 * InfoWindowSprite is a sprite that contains sub sprites that function as tabs.
 */
	public class PhysicalNodeGroupMarker extends UIComponent {
  [Embed('../../../images/circle_blue.png')] private var CloudImg:Class;
  
  public function PhysicalNodeGroupMarker(newLabel:String, newMarker:ProtogeniMapMarker) {
	  marker = newMarker;
	  this.addEventListener(MouseEvent.MOUSE_MOVE, drag);
      addChild(new CloudImg());
	  
	  // Apply the drop shadow filter to the box.
	  var shadow:DropShadowFilter = new DropShadowFilter();
	  shadow.distance = 10;
	  shadow.angle = 25;
	  
	  // You can also set other properties, such as the shadow color,
	  // alpha, amount of blur, strength, quality, and options for 
	  // inner shadows and knockout effects.
	  
	  this.filters = [shadow];
	  
	  label = newLabel;
      
      var radius:int = 60;
    var labelMc:TextField = new TextField();
    labelMc.autoSize = TextFieldAutoSize.LEFT;
	labelMc.textColor = 0xFFFFFF;
    labelMc.selectable = false;
    labelMc.border = false;
    labelMc.embedFonts = false;
    labelMc.mouseEnabled = false;
    labelMc.width = radius;
    labelMc.height = radius;
    labelMc.text = label;
    	labelMc.y = 8;
    if(label.length == 1)
    	labelMc.x = 13;
    else if(label.length == 2)
    	labelMc.x = 11;
    else
    	labelMc.x = 9;
    
    addChild(labelMc);
    cacheAsBitmap = true;
	buttonMode=true;
	useHandCursor = true;
  }
  
  private var label:String;
  public var marker:ProtogeniMapMarker;
  
  public function drag(e:MouseEvent):void
  {
	  var ds:DragSource = new DragSource();
	  ds.addData(marker, 'marker');
	  
	  var d:PhysicalNodeGroupMarker = new PhysicalNodeGroupMarker(label, marker)
	  
	  DragManager.doDrag(this, ds, e, d);
  }
  
}

}
