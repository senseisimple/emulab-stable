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

package protogeni
{
	public final class StringUtil
	{
		// Makes the first letter uppercase
		public static function firstToUpper (phrase : String) : String {
			return phrase.substring(1, 0).toUpperCase() + phrase.substring(1);
		}
		
		public static function replaceString(original:String, find:String, replace:String):String {
			return original.split(find).join(replace);
		}
		
		// Shortens the given string to a length, taking out from the middle
		public static function shortenString(phrase : String, size : int) : String {
			// Remove any un-needed elements
			var a:Array = phrase.split("https://");
			if(a.length == 1)
				a = phrase.split("http://");
			if(a.length == 2)
				phrase = a[1];
			
			if(phrase.length < size)
				return phrase;
			
			var removeChars:int = phrase.length - size + 3;
			var upTo:int = (phrase.length / 2) - (removeChars / 2);
			return phrase.substring(0, upTo) + "..." +  phrase.substring(upTo + removeChars);
		}
	}
}