/* GENIPUBLIC-COPYRIGHT
 * Copyright (c) 2008, 2009 University of Utah and the Flux Group.
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
  import flash.external.ExternalInterface;

  public class Util
  {
    public static function makeUrn(authority : String,
                                   type : String,
                                   name : String) : String
    {
      return "urn:publicid:IDN+" + authority + "+" + type + "+" + name;
    }
	
	// Takes the given bandwidth and creates a human readable string
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
	
	// Makes the first letter uppercase
	public static function firstToUpper (phrase : String) : String {
		return phrase.substring(1, 0).toUpperCase()+phrase.substring(1);
	}
	
	public static function replaceString(original:String, find:String, replace:String):String {
		return original.split(find).join(replace);
	}
	
	public static function getDotString(name : String) : String {
		return replaceString(replaceString(name, ".", ""), "-", "");
	}
	
	public static function findInAny(text:Array, candidates:Array, matchAll:Boolean = false, caseSensitive:Boolean = false):Boolean
	{
		if(!caseSensitive)
		{
			for each(var textTemp:String in text)
			textTemp = textTemp.toLowerCase();
		}
			
		for each(var candidate:String in candidates)
		{
			if(!caseSensitive)
				candidate = candidate.toLowerCase();
			for each(var s:String in text)
			{
				
				if(matchAll)
				{
					if(candidate == s)
						return true;
				}
				else
				{
					if(candidate.indexOf(s) > -1)
						return true;
				}
			}
			
		}
		return false;
	}
	
	// Shortens the given string to a length, taking out from the middle
	public static function shortenString(phrase : String, size : int) : String {
		if(phrase.length < size)
			return phrase;
		
		var removeChars:int = phrase.length - size + 3;
		var upTo:int = (phrase.length / 2) - (removeChars / 2);
		return phrase.substring(0, upTo) + "..." +  phrase.substring(upTo + removeChars);
	}
	
	public static  function getBrowserName():String
	{
		var browser:String;
		var browserAgent:String = ExternalInterface.call("function getBrowser(){return navigator.userAgent;}");
		
		if(browserAgent == null)
			return "Undefined";
		else if(browserAgent.indexOf("Firefox") >= 0)
			browser = "Firefox";
		else if(browserAgent.indexOf("Safari") >= 0)
			browser = "Safari";
		else if(browserAgent.indexOf("MSIE") >= 0)
			browser = "IE";
		else if(browserAgent.indexOf("Opera") >= 0)
			browser = "Opera";
		else
			browser = "Undefined";
		
		return (browser);
	}
  }
}
