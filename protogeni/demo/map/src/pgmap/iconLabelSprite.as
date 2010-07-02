/**
 * Code taken from the official tutorials for Google Maps API for Flash
 */
 
 package pgmap {

import flash.display.Sprite;
import flash.text.TextField;
import flash.text.TextFieldAutoSize;

/**
 * InfoWindowSprite is a sprite that contains sub sprites that function as tabs.
 */
	public class iconLabelSprite extends Sprite {
  [Embed('../../images/cloud.png')] private var CloudImg:Class;
  
  
  public function iconLabelSprite(label:String) {
      addChild(new CloudImg());
      
      var radius:int = 60;
    var labelMc:TextField = new TextField();
    labelMc.autoSize = TextFieldAutoSize.LEFT;
    labelMc.selectable = false;
    labelMc.border = false;
    labelMc.embedFonts = false;
    labelMc.mouseEnabled = false;
    labelMc.width = radius;
    labelMc.height = radius;
    labelMc.text = label;
    	labelMc.y = 8;
    if(label.length == 1)
    	labelMc.x = 15;
    else if(label.length == 2)
    	labelMc.x = 13;
    else
    	labelMc.x = 11;
    
    addChild(labelMc);
    cacheAsBitmap = true;
	buttonMode=true;
	useHandCursor = true;
  }
  
}

}
