/**
* @author	Matt Shaw <xmlrpc@mattism.com>
* @url		http://sf.net/projects/xmlrpcflash
* 			http://www.osflash.org/doku.php?id=xmlrpcflash	
*
* @author   Daniel Mclaren (http://danielmclaren.net)
* @note     Updated to Actionscript 3.0		
*/

package com.mattism.http.xmlrpc.util
{
	public class XMLRPCDataTypes {

		public static const STRING:String   = "string";
		public static const CDATA:String    = "cdata";
		public static const i4:String       = "i4";
		public static const INT:String      = "int";
		public static const BOOLEAN:String  = "boolean";
		public static const DOUBLE:String   = "double";
		public static const DATETIME:String = "dateTime.iso8601";
		public static const BASE64:String   = "base64";
		public static const STRUCT:String   = "struct";
		public static const ARRAY:String    = "array";
		
	}
}

/*
<i4> or <int>		java.lang.Integer	Number
<boolean>			java.lang.Boolean	Boolean
<string>			java.lang.String	String
<double>			java.lang.Double	Number
<dateTime.iso8601>	java.util.Date		Date
<struct>			java.util.Hashtable	Object
<array>				java.util.Vector	Array
<base64>			byte[ ]				Base64
*/