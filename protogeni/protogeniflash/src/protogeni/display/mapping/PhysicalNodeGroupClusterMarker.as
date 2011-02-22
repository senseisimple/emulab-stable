/**
 * Code taken from the official tutorials for Google Maps API for Flash
 */
 
 package protogeni.display.mapping {

import flash.events.MouseEvent;
import flash.filters.DropShadowFilter;
import flash.text.TextField;
import flash.text.TextFieldAutoSize;

import mx.controls.Image;
import mx.core.DragSource;
import mx.core.UIComponent;
import mx.managers.DragManager;

import protogeni.resources.GeniManager;

/**
 * InfoWindowSprite is a sprite that contains sub sprites that function as tabs.
 */
	public class PhysicalNodeGroupClusterMarker extends UIComponent {
		
  [Embed('../../../../images/planetlabmultisite.png')] private var PlanetlabmultiImg:Class;
  [Embed('../../../../images/protogenimultisite.png')] private var ProtogenimultiImg:Class;
  [Embed('../../../../images/mixedmultisite.png')] private var MixedmultiImg:Class;
  
  
  public function PhysicalNodeGroupClusterMarker(newLabel:String, newMarker:GeniMapMarker, type:int) {
	  marker = newMarker;
	  this.addEventListener(MouseEvent.MOUSE_MOVE, drag);

	  switch(type) {
		case GeniManager.TYPE_PLANETLAB:
			addChild(new PlanetlabmultiImg());
		  	break;
		case GeniManager.TYPE_PROTOGENI:
			addChild(new ProtogenimultiImg());
			break;
		default:
			addChild(new MixedmultiImg());
	  }
	  managerType = type;
	  
	  // Apply the drop shadow filter to the box.
	  var shadow:DropShadowFilter = new DropShadowFilter();
	  shadow.distance = 5;
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
    	labelMc.y = 5;
    if(label.length == 1)
    	labelMc.x = 10;
    else if(label.length == 2)
    	labelMc.x = 7;
    else
    	labelMc.x = 5;
    
    addChild(labelMc);
    cacheAsBitmap = true;
	buttonMode=true;
	useHandCursor = true;
  }
  
  private var label:String;
  public var marker:GeniMapMarker;
  public var managerType:int;
  
  public function drag(e:MouseEvent):void
  {
	  var ds:DragSource = new DragSource();
	  ds.addData(marker, 'marker');
	  
	  var d:PhysicalNodeGroupClusterMarker = new PhysicalNodeGroupClusterMarker(label, marker, managerType)
	  
	  DragManager.doDrag(this, ds, e, d);
  }
  
}

}
