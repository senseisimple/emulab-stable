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
	 * Unique resource identifier in the form: urn:publicid:IDN+authority+type+name
	 * 
	 * @author mstrum
	 * 
	 */
	public class IdnUrn
	{
		[Bindable]
		public var full:String;
		
		public function get authority():String {
			if(full.length > 0)
				return full.split("+")[1];
			else
				return "";
		}
		
		public function get type():String {
			if(full.length > 0)
				return full.split("+")[2];
			else
				return "";
		}
		
		public function get name():String {
			if(full.length > 0)
				return full.split("+")[3];
			else
				return "";
		}
		
		public function IdnUrn(urn:String = "")
		{
			full = urn;
		}
		
		public static function getAuthorityFrom(urn:String) : String
		{
			return urn.split("+")[1];
		}
		
		public static function getTypeFrom(urn:String) : String
		{
			return urn.split("+")[2];
		}
		
		public static function getNameFrom(urn:String) : String
		{
			return urn.split("+")[3];
		}
		
		public static function makeFrom(newAuthority:String,
										newType:String,
										newName:String):IdnUrn
		{
			return new IdnUrn("urn:publicid:IDN+" + newAuthority + "+" + newType + "+" + newName);
		}
	}
}