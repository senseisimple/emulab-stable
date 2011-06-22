/* GENIPUBLIC-COPYRIGHT
* Copyright (c) 2008-2011 University of Utah and the Flux Group.
* All rights reserved.
*
* Permission to use, copy, modify and distribute this software is hereby
* granted provided that (1) source code retains these copyright, permission,
* and disclaimer notices, and (2) redistributions including binaries
* reproduce the notices in supporting documentation.
*
* THE UNIVERSITY OF UTAH ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
* CONDITION.  THE UNIVERSITY OF UTAH DISCLAIMS ANY LIABILITY OF ANY KIND
* FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
*/

package
{
	import mx.core.FlexGlobals;
	
	import protogeni.resources.SliceAuthority;

	/**
	 * Handles all of the parameters handed externally
	 * 
	 * mapkey - Force a different Google Maps API key if given
	 * debug - Output debug stuff
	 * pgonly - Only use ProtoGENI managers
	 * mode - Operating mode
	 * saurl - Only allow a certain slice authority to be selected
	 * publicurl - URL for the public listing of RSPECs
	 * 
	 * @author mstrum
	 * 
	 */
	public class ParamHandler
	{
		public static function preload():void {
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