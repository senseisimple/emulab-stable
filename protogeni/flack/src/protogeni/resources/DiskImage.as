package protogeni.resources
{
	public class DiskImage
	{
		public function DiskImage(newName:String = "",
								  newOs:String = "",
								  newVersion:String = "",
								  newDescription:String = "",
								  newIsDefault:Boolean = false)
		{
			name = newName.replace("+image+emulab-ops:", "+image+emulab-ops//");
			os = newOs;
			version = newVersion;
			description = newDescription;
			isDefault = newIsDefault;
		}
		
		public var name:String;
		public var os:String;
		public var version:String;
		public var description:String;
		public var isDefault:Boolean;
		
		public static function getDiskImageShort(long:String, manager:GeniManager):String
		{
			if(long.indexOf("urn:publicid:IDN+" + manager.Authority + "+image+emulab-ops//") > -1)
				return long.replace("urn:publicid:IDN+" + manager.Authority + "+image+emulab-ops//", "");
			else if(long.indexOf("urn:publicid:IDN+" + manager.Authority + "+image+emulab-ops:") > -1)
				return long.replace("urn:publicid:IDN+" + manager.Authority + "+image+emulab-ops:", "");
			else
				return long;
		}
		
		public static function getDiskImageLong(short:String, manager:GeniManager):String
		{
			return "urn:publicid:IDN+" + manager.Authority + "+image+emulab-ops//" + short;
		}
	}
}