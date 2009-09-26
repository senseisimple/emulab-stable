package pgmap
{
	public class Common
	{
		public static var successColor:String = "#0E8219";
		public static var failColor:String = "#FE0000";
		public static var waitColor:String = "#FF7F00";
		
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
	}
}