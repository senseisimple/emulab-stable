package com.mstrum
{
	import flash.utils.ByteArray;

	public class Asn1Field
	{
		public var identifierClass:uint;
		public var identifierTag:uint;
		public var identifierConstructed:Boolean; // false = primitive
		
		public var length:uint;
		
		public var rawContents:ByteArray;
		public var contents:*;
		
		public var parent:Asn1Field;
		
		public function Asn1Field(newParent:Asn1Field = null)
		{
			rawContents = new ByteArray();
			parent = newParent;
		}
		
		public function getValue():* {
			if(contents is Array) {
				var valueIdx:int = 0;
				if(contents[0] is Asn1Field && contents[0].identifierClass == Asn1Classes.UNIVERSAL && contents[0].identifierTag == Asn1Tags.OID)
					valueIdx = contents.length-1;
				if(contents[valueIdx] is Asn1Field)
					return contents[valueIdx].getValue();
				else
					return contents[valueIdx];
			} else if(contents is Asn1Field)
				return contents.getValue();
			else
				return contents;
		}
		
		public function getHolderFor(oid:String):Asn1Field {
			if(this.identifierTag == Asn1Tags.OID) {
				if(this.contents == oid)
					return this.parent;
				else
					return null;
			} else if(contents is Asn1Field) {
				return contents.getHolderFor(oid);
			} else if(contents is Array) {
				for each(var child:Asn1Field in contents) {
					var resultField:Asn1Field = child.getHolderFor(oid);
					if(resultField != null)
						return resultField;
				}
			}
			return null;
		}
	}
}