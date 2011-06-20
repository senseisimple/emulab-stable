// Adapted from com.hurlant.util.der.DER

package com.mstrum
{
	import flash.utils.ByteArray;
	
	import mx.core.UIComponent;

	// '-----BEGIN [^-]+-----([A-Za-z0-9+\/=\\s]+)-----END [^-]+-----');
	public class DER
	{
		public static function Parse(bytes:ByteArray, parent:Asn1Field = null):Asn1Field {
			if(bytes.bytesAvailable < 2)
				throw new Error("Not enough bytes to parse DER structure");
			var newField:Asn1Field = new Asn1Field(parent);
			// type
			var currentOctet:uint = bytes.readUnsignedByte();
			newField.identifierClass = (currentOctet&0xC0) >> 6;
			newField.identifierConstructed = (currentOctet&0x20) != 0;
			newField.identifierTag = currentOctet&0x1F; // XXX this will be 11s if the id is multiple octets
			// Check for type above 30
			if(newField.identifierTag == 31) {
				newField.identifierTag = 0;
				do {
					currentOctet = bytes.readUnsignedByte();
					newField.identifierTag = ((newField.identifierTag)<<7) | currentOctet&0x7F;
				} while(currentOctet&0x80)
			}

			// length
			currentOctet = bytes.readUnsignedByte();
			newField.length = currentOctet;
			// Check for long form
			if (newField.length >= 0x80) {
				// long form of length
				if(newField.length == 0xFF)
					throw new Error("Long length of 0xFF is not allowed");
				var count:int = newField.length & 0x7F;
				newField.length = 0;
				while (count > 0) {
					newField.length = (newField.length<<8) | bytes.readUnsignedByte();
					count--;
				}
			}
			if(bytes.bytesAvailable < newField.length)
				throw new Error("Not enough bytes to parse DER value");
			
			if(newField.length == 0)
				return newField;
			
			// data
			bytes.readBytes(newField.rawContents, 0, newField.length);
			
			// XXX These need to be context specific, I only see these a limited number of times now so it's good until something breaks...
			if(newField.identifierClass == Asn1Classes.CONTEXT_SPECIFIC) {
				// 0xAX
				if(newField.identifierConstructed)
					newField.contents = DER.Parse(newField.rawContents, newField);
				else {
					// 0x8X
					switch(newField.identifierTag) {
						case Asn1Tags.RFC822NAME:
						case Asn1Tags.UNIFORM_RESOURCE_IDENTIFIER:
							newField.contents = newField.rawContents.readMultiByte(newField.length, "x-IA5");
							break;
						default:
							newField.contents = newField.rawContents;
					}
				}
			} else {
				var left:int = newField.length;
				switch(newField.identifierTag) {
					case Asn1Tags.SEQ_SEQOF:	// For now treat as the same thing
					case Asn1Tags.SET_SETOF:
						currentOctet = newField.rawContents.readUnsignedByte();
						newField.rawContents.position--;
						switch(currentOctet) {
							case 0x00: // XXX no idea why this is encountered ???? It was in an attribute cert in IdCeAuthorityKeyIdentifier
								newField.contents = newField.rawContents;
								break;
							default:
								newField.contents = [];
								while(newField.rawContents.bytesAvailable>0) {
									newField.contents.push(DER.Parse(newField.rawContents, newField));
								}
						}
						break;
					case Asn1Tags.OID:
						var oidString:String = "";
						currentOctet = newField.rawContents.readUnsignedByte();
						oidString += String(uint(currentOctet/40)) + ".";
						oidString += String(uint(currentOctet%40));
						
						var v:uint = 0;
						while (newField.rawContents.bytesAvailable>0) {
							currentOctet = newField.rawContents.readUnsignedByte();
							var last:Boolean = (currentOctet&0x80)==0;
							currentOctet &= 0x7f;
							v = v*128 + currentOctet;
							if (last) {
								oidString += "." + String(v);
								v = 0;
							}
						}
						newField.contents = Oids.getNameFor(oidString);
						break;
					case Asn1Tags.INTEGER: // XXX won't work for huge values
						currentOctet = newField.rawContents.readUnsignedByte();
						left--;
						var integerValue:int = 0;
						if(currentOctet&0x80)
							integerValue = -1;
						integerValue = (integerValue << 8) | currentOctet;
						while(left--) {
							integerValue = (integerValue << 8) | newField.rawContents.readUnsignedByte();
						} 
						break;
					case Asn1Tags.BITSTRING:
						var paddedBitStrings:uint = newField.rawContents.readUnsignedByte();
						left--;
						var bitstring:String = "";
						while(left--) {
							currentOctet = newField.rawContents.readUnsignedByte();
							for(var bitstringPos:int = 7; bitstringPos > -1; bitstringPos--) {
								bitstring += String((currentOctet >> bitstringPos)&0x01);
							}
						}
						newField.contents = bitstring.substr(0, (newField.length-1)*8 - paddedBitStrings);
						break;
					case Asn1Tags.OCTETSTRING:
						// See if there is anything underneath
						// TECHNICALLY they should say this is constructed and not primitive,
						// BUT it seems like octet strings are sometiems arrays and sometimes ASN.1 things
						currentOctet = newField.rawContents.readUnsignedByte();
						newField.rawContents.position--;
						switch(currentOctet) {
							case 0x30: //Asn1Tags.SEQ_SEQOF
							case 0x04: //Asn1Tags.OCTETSTRING
								newField.contents = DER.Parse(newField.rawContents, newField);
								break;
							default:
								newField.contents = newField.rawContents;
						}
						break;
					case Asn1Tags.BOOLEAN:
						newField.contents = newField.rawContents.readUnsignedByte() != 0;
						break;
					case Asn1Tags.NULL:
						return null;
					case Asn1Tags.UTF8STRING:
						newField.contents = newField.rawContents.readMultiByte(newField.length, "utf-8");
						break;
					case Asn1Tags.PRINTABLESTRING:
					case Asn1Tags.BMP_STRING:
					case Asn1Tags.UNIVERSAL_STRING:
					case Asn1Tags.GENERAL_STRING:
					case Asn1Tags.VISIBLE_STRING:
					case Asn1Tags.GRAPHIC_STRING:
					case Asn1Tags.NUMERIC_STRING:
					case Asn1Tags.VIDEOTEX_STRING:
						newField.contents = newField.rawContents.readMultiByte(newField.length, "US-ASCII");
						break;
					case Asn1Tags.T61STRING:
						newField.contents = newField.rawContents.readMultiByte(newField.length, "latin1");
						break;
					case Asn1Tags.XIA5STRING:
						newField.contents = newField.rawContents.readMultiByte(newField.length, "x-IA5");
						break;
					case Asn1Tags.GENERALIZEDTIME:
						var GeneralizedTimeString:String = newField.rawContents.readMultiByte(newField.length, "US-ASCII");
						var generalyear:uint = parseInt(GeneralizedTimeString.substr(0, 4));
						var generalmonth:uint = parseInt(GeneralizedTimeString.substr(4,2));
						var generalday:uint = parseInt(GeneralizedTimeString.substr(6,2));
						var generalhour:uint = parseInt(GeneralizedTimeString.substr(8,2));
						var generalminute:uint = parseInt(GeneralizedTimeString.substr(10,2));
						var generalsecond:uint = parseInt(GeneralizedTimeString.substr(12,2));
						newField.contents = new Date(generalyear, generalmonth-1, generalday, generalhour, generalminute, generalsecond);
						break;
					case Asn1Tags.UTCTIME:
						var UtcTimeString:String = newField.rawContents.readMultiByte(newField.length, "US-ASCII");
						var year:uint = parseInt(UtcTimeString.substr(0, 2));
						if (year<50)
							year+=2000;
						else
							year+=1900;
						var month:uint = parseInt(UtcTimeString.substr(2,2));
						var day:uint = parseInt(UtcTimeString.substr(4,2));
						var hour:uint = parseInt(UtcTimeString.substr(6,2));
						var minute:uint = parseInt(UtcTimeString.substr(8,2));
						newField.contents = new Date(year, month-1, day, hour, minute);
						break;
					// TODO, but not really used very often apparently
					case Asn1Tags.OD:
					case Asn1Tags.EXTERNAL_INSTANCEOF:
					case Asn1Tags.REAL:
					case Asn1Tags.ENUMERATED:
					case Asn1Tags.EMBEDDED_PDV:
					case Asn1Tags.RELATIVE_OID:
					default:
						newField.contents = newField.rawContents;
				}
			}
			
			return newField;
		}
	}
}