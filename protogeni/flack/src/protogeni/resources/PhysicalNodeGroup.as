/* GENIPUBLIC-COPYRIGHT
 * Copyright (c) 2009 University of Utah and the Flux Group.
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
	import com.google.maps.LatLng;
	import com.google.maps.services.ClientGeocoder;
	import com.google.maps.services.GeocodingEvent;
	import com.google.maps.services.Placemark;
	
	// Group of physical nodes located in one area
	public class PhysicalNodeGroup
	{
		public var owner:PhysicalNodeGroupCollection = null;
		
		public var latitude:Number;
		public var longitude:Number;
		public var country:String;
		
		[Bindable]
		public var city:String = "";
		
		public var collection:Vector.<PhysicalNode> = new Vector.<PhysicalNode>();
		public var links:PhysicalLinkGroup = null;
		
		[Bindable]
		public var original:PhysicalNodeGroup = null;
		
		public function PhysicalNodeGroup(lat:Number = -1, lng:Number = -1, cnt:String = "", own:PhysicalNodeGroupCollection = null, orig:PhysicalNodeGroup = null)
		{
			latitude = lat;
			longitude = lng;
			country = cnt;
			owner = own;
			original = orig;
			if(original == null) {
				Geocode();
			}
		}
		
		public function Geocode():void {
			var geocoder:ClientGeocoder = new ClientGeocoder();
			geocoder.addEventListener(GeocodingEvent.GEOCODING_SUCCESS,
				function(event:GeocodingEvent):void {
					var placemarks:Array = event.response.placemarks;
					if (placemarks.length > 0) {
						try {
							var p:Placemark = event.response.placemarks[0] as Placemark;
							var fullAddress : String = p.address;
							var splitAddress : Array = fullAddress.split(',');
							if(splitAddress.length == 3)
								city = splitAddress[0];
							else 
								if(splitAddress.length == 4)
									city = splitAddress[1];
								else
									city = fullAddress;
							//nodeGroup.city = groupInfo.city;
						} catch (err:Error) { }
					}
				});
			
			geocoder.addEventListener(GeocodingEvent.GEOCODING_FAILURE,
				function(event:GeocodingEvent):void {
					//Main.log.appendMessage(
					//	new LogMessage("","Geocoding failed (" + event.status + " / " + event.eventPhase + ")","",true));
				});

			geocoder.reverseGeocode(new LatLng(latitude, longitude));
		}
		
		public function Add(n:PhysicalNode):void {
			collection.push(n);
		}

		public function GetByUrn(urn:String):PhysicalNode {
			for each ( var n:PhysicalNode in collection ) {
				if(n.id == urn)
					return n;
			}
			return null;
		}
		
		public function GetByName(name:String):PhysicalNode {
			for each ( var n:PhysicalNode in collection ) {
				if(n.name == name)
					return n;
			}
			return null;
		}
		
		public function GetByType(type:String):PhysicalNodeGroup {
			var group:PhysicalNodeGroup = new PhysicalNodeGroup();
			for each ( var n:PhysicalNode in collection ) {
				for each ( var nt:String in n.hardwareTypes ) {
					if(nt == type) {
						group.Add(n);
						break;
					}
				}
			}
			return group;
		}
		
		public function Available():Number {
			var cnt:Number = 0;
			for each ( var n:PhysicalNode in collection ) {
				if(n.available)
					cnt++;
			}
			return cnt;
		}
		
		public function ExternalLinks():Number {
			var cnt:Number = 0;
			for each ( var n:PhysicalNode in collection ) {
				for each ( var l:PhysicalLink in n.GetLinks() ) {
					if(l.owner != n.owner.links)
						cnt++;
				}
			}
			return cnt;
		}
		
		public function GetManager():GeniManager
		{
			return collection[0].manager;
		}
	}
}