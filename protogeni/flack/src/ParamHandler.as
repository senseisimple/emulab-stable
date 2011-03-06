package
{
	import mx.core.FlexGlobals;
	
	import protogeni.resources.SliceAuthority;

	public class ParamHandler
	{
		public static function preload():void {
			// Force a different Google Maps API key if given
			try{
				if(FlexGlobals.topLevelApplication.parameters.mapkey != null)
				{
					Main.Application().forceMapKey = FlexGlobals.topLevelApplication.parameters.mapkey;
				}
			} catch(all:Error) {
			}
			
			try{
				if(FlexGlobals.topLevelApplication.parameters.debug != null)
				{
					Main.debugMode = FlexGlobals.topLevelApplication.parameters.debug == "1";
				}
			} catch(all:Error) {
			}
			
			try{
				if(FlexGlobals.topLevelApplication.parameters.usejs != null)
				{
					Main.useJavascript = FlexGlobals.topLevelApplication.parameters.usejs == "1";
				}
			} catch(all:Error) {
			}
			
			try{
				if(FlexGlobals.topLevelApplication.parameters.pgonly != null)
				{
					Main.protogeniOnly = FlexGlobals.topLevelApplication.parameters.pgonly == "1";
				}
			} catch(all:Error) {
			}
		}
		
		public static function load():void {
			try{
				if(FlexGlobals.topLevelApplication.parameters.mode != null)
				{
					var input:String = FlexGlobals.topLevelApplication.parameters.mode;
					
					Main.Application().allowAuthenticate = input != "publiconly";
					Main.geniHandler.unauthenticatedMode = input != "authenticate";
				}
			} catch(all:Error) {
			}
			try{
				if(FlexGlobals.topLevelApplication.parameters.saurl != null)
				{
					for each(var sa:SliceAuthority in Main.geniHandler.GeniAuthorities.source) {
						if(sa.Url == FlexGlobals.topLevelApplication.parameters.saurl) {
							Main.geniHandler.forceAuthority = sa;
							break;
						}
					}
				}
			} catch(all:Error) {
			}
			try{
				if(FlexGlobals.topLevelApplication.parameters.publicurl != null)
				{
					Main.geniHandler.publicUrl = FlexGlobals.topLevelApplication.parameters.publicurl;
				}
			} catch(all:Error) {
			}
		}
	}
}