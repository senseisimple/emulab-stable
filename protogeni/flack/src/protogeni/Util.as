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
	import mx.collections.ArrayCollection;
	
	/**
	 * Various common values/functions used throughout the library
	 * 
	 * @author mstrum
	 * 
	 */
	public final class Util
	{
		public static const defaultRspecVersion:Number = 2;
		[Bindable]
		public static var usableRspecVersions:ArrayCollection = new ArrayCollection([0.1, 0.2, 2]);
		
		// Takes the given bandwidth and creates a human readable string
		public static function kbsToString(bandwidth:Number):String {
			var bw:String = "";
			if(bandwidth < 1000) {
				return bandwidth + " Kb/s"
			} else if(bandwidth < 1000000) {
				return bandwidth / 1000 + " Mb/s"
			} else if(bandwidth < 1000000000) {
				return bandwidth / 1000000 + " Gb/s"
			}
			return bw;
		}
		
		public static function getDotString(name:String):String {
			return StringUtil.replaceString(StringUtil.replaceString(name, ".", ""), "-", "");
		}
		
		public static function parseProtogeniDate(value:String):Date
		{
			var dateString:String = value;
			dateString = dateString.replace(/(\d{4,4})\-(\d{2,2})\-(\d{2,2})/g, "$1/$2/$3");
			dateString = dateString.replace("T", " ");
			dateString = dateString.replace(/(\+|\-)(\d+):(\d+)/g, " GMT$1$2$3");
			dateString = dateString.replace("Z", " GMT-0000");
			return new Date(Date.parse(dateString));
		}
		
		public static function addIfNonexistingToArray(source:Array, o:*):void
		{
			if(source.indexOf(o) == -1)
				source.push(o);
		}
		
		public static function addIfNonexistingToArrayCollection(source:ArrayCollection, o:*):void
		{
			if(source.getItemIndex(o) == -1)
				source.addItem(o);
		}
		
		public static function findInAny(text:Array,
										 candidates:Array,
										 matchAll:Boolean = false,
										 caseSensitive:Boolean = false):Boolean
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
		
		public static function areEqual(a:Array,b:Array):Boolean {
			// handle null arrays
			if(a == null && b == null)
				return true;
			else if(a == null || b == null)
				return false;
			
			// obviously not equal
			if(a.length != b.length) {
				return false;
			}
			
			var len:int = a.length;
			for(var i:int = 0; i < len; i++) {
				if(a[i] !== b[i]) {
					return false;
				}
			}
			return true;
		}
		
		public static function haveSame(a:Array,b:Array):Boolean {
			if(a.length != b.length)
				return false;
			
			var len:int = a.length;
			for(var i:int = 0; i < len; i++) {
				if(b.indexOf(a[i]) == -1)
					return false;
			}
			return true;
		}
		
		public static function keepUniqueObjects(ac:ArrayCollection,
												 oc:ArrayCollection = null):ArrayCollection
		{
			var newAc:ArrayCollection;
			if(oc != null)
				newAc = oc;
			else
				newAc = new ArrayCollection();
			for each(var o:Object in ac) {
				if(o is ArrayCollection)
					newAc = keepUniqueObjects((o as ArrayCollection), newAc);
				else {
					if(!newAc.contains(o))
						newAc.addItem(o);
				}
			}
			return newAc;
		}
	}
}
