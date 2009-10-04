package pgmap
{
	import mx.core.Application;
	
	public class Common
	{
		public static var successColor:String = "#0E8219";
		public static var failColor:String = "#FE0000";
		public static var waitColor:String = "#FF7F00";
		
		[Bindable]
        [Embed(source="../../images/tick.png")]
        public static var availableIcon:Class;

        [Bindable]
        [Embed(source="../../images/cross.png")]
        public static var notAvailableIcon:Class;
        
        [Bindable]
        [Embed(source="../../images/computer.png")]
        public static var ownedIcon:Class;
        
        public static function assignIcon(val:Boolean):Class {
				if (val)
                    return Common.availableIcon;
                else
                    return Common.notAvailableIcon;
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
	}
}