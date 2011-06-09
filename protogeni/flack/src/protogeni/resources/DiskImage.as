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

package protogeni.resources
{
	/**
	 * Disk image used for a resource
	 * 
	 * @author mstrum
	 * 
	 */
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
		
		public static function getDiskImageShort(long:String,
												 manager:GeniManager):String
		{
			if(long.indexOf("urn:publicid:IDN+" + manager.Urn.authority + "+image+emulab-ops//") > -1)
				return long.replace("urn:publicid:IDN+" + manager.Urn.authority + "+image+emulab-ops//", "");
			else if(long.indexOf("urn:publicid:IDN+" + manager.Urn.authority + "+image+emulab-ops:") > -1)
				return long.replace("urn:publicid:IDN+" + manager.Urn.authority + "+image+emulab-ops:", "");
			else
				return long;
		}
		
		public static function getDiskImageLong(short:String,
												manager:GeniManager):String
		{
			return "urn:publicid:IDN+" + manager.Urn.authority + "+image+emulab-ops//" + short;
		}
	}
}