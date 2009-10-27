package pgmap
{
	import com.google.maps.InfoWindowOptions;
	import com.google.maps.LatLng;
	import com.google.maps.MapMouseEvent;
	import com.google.maps.overlays.Marker;
	import com.google.maps.overlays.MarkerOptions;
	import com.google.maps.overlays.Polyline;
	import com.google.maps.overlays.PolylineOptions;
	import com.google.maps.services.ClientGeocoder;
	import com.google.maps.services.GeocodingEvent;
	import com.google.maps.services.Placemark;
	import com.google.maps.styles.FillStyle;
	import com.google.maps.styles.StrokeStyle;
	
	import flash.events.Event;
	import flash.events.MouseEvent;
	import flash.geom.Point;
	
	import mx.managers.PopUpManager;
    	
	public class ProtoGeniMapHandler
	{
		public var main : pgmap;
		
		public function ProtoGeniMapHandler()
		{
		}
		
		private function addMarker(g:NodeGroup):void
	    {
	    	// Create the group to be drawn
	    	var drawGroup:NodeGroup = new NodeGroup(g.latitude, g.longitude, g.country , main.pgHandler.Nodes);
	    	drawGroup.owner = g.owner;
	    	if(main.userResourcesOnly) {
	    		for each(var n:Node in g.collection) {
	    			if( n.slice != null &&
	    			      ((main.selectedSlice == null || main.selectedSlice.status != null) ||
	    			      (n.slice == main.selectedSlice)) )
	    			{
	    				drawGroup.Add(n);
	    			}
	    			
	    			// Figure out links
	    		}
	    	} else {
	    		drawGroup = g;
	    	}
	    	
	    	if(drawGroup.collection.length > 0) {
	    		var m:Marker = new Marker(
			      	new LatLng(drawGroup.latitude, drawGroup.longitude),
			      	new MarkerOptions({
			                  strokeStyle: new StrokeStyle({color: 0x092B9F}),
			                  fillStyle: new FillStyle({color: 0xD2E1F0, alpha: 1}),
			                  radius: 14,
			                  hasShadow: true,
			                  //tooltip: g.country,
			                  label: drawGroup.collection.length.toString()
			      	}));
	
		        var groupInfo:NodeGroupInfo = new NodeGroupInfo();
		        groupInfo.Load(drawGroup, main);
		        
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
				        			groupInfo.city = splitAddress[0];
				        		else 
				        		if(splitAddress.length == 4)
				        			groupInfo.city = splitAddress[1];
				        		else
				        			drawGroup.city = fullAddress;
				        	} catch (err:Error) { }
				        }
				      });
				        	
				  geocoder.addEventListener(GeocodingEvent.GEOCODING_FAILURE,
				        function(event:GeocodingEvent):void {
				          //Alert.show("Geocoding failed");
				        });
	
				  //geocoder.reverseGeocode(new LatLng(g.latitude, g.longitude));
		        
		        m.addEventListener(MapMouseEvent.CLICK, function(e:Event):void {
		            m.openInfoWindow(
		            	new InfoWindowOptions({
		            		customContent:groupInfo,
		            		customoffset: new Point(0, 10),
		            		width:140,
		            		height:150,
		            		drawDefaultFrame:true
		            	}));
		        });
	
		  		main.map.addOverlay(m);
	    	} else {
	    		// Draw an empty marker
	    		var nonodes:Marker = new Marker(
			      	new LatLng(drawGroup.latitude, drawGroup.longitude),
			      	new MarkerOptions({
			                  strokeStyle: new StrokeStyle({color: 0x666666}),
			                  fillStyle: new FillStyle({color: 0xCCCCCC, alpha: .8}),
			                  radius: 8,
			                  hasShadow: false
			      	}));
	
		        main.map.addOverlay(nonodes);
	    	}
	        
	    }
	    
	    public function addLink(lg:LinkGroup):void {
	    	// Create the group to be drawn
	    	var drawGroup:LinkGroup = lg;
	    	
	    	if(drawGroup.collection.length > 0) {
	    		// Add line
				var polyline:Polyline = new Polyline([
					new LatLng(drawGroup.latitude1, drawGroup.longitude1),
					new LatLng(drawGroup.latitude2, drawGroup.longitude2)
					], new PolylineOptions({ strokeStyle: new StrokeStyle({
						color: 0xFF00FF,
						thickness: 4,
						alpha:1})
					}));
	
				main.map.addOverlay(polyline);
				
				// Add link marker
				var ll:LatLng = new LatLng((drawGroup.latitude1 + drawGroup.latitude2)/2, (drawGroup.longitude1 + drawGroup.longitude2)/2);
				
				var t:TooltipOverlay = new TooltipOverlay(ll, Common.kbsToString(drawGroup.TotalBandwidth()));
		  		t.addEventListener(MouseEvent.CLICK, function(e:Event):void {
		            e.stopImmediatePropagation();
		            main.pgHandler.map.viewLinkGroup(drawGroup)
		        });
		        
		  		main.map.addOverlay(t);
	    	} else {
	    		// Add line
				var blankline:Polyline = new Polyline([
					new LatLng(drawGroup.latitude1, drawGroup.longitude1),
					new LatLng(drawGroup.latitude2, drawGroup.longitude2)
					], new PolylineOptions({ strokeStyle: new StrokeStyle({
						color: 0x666666,
						thickness: 3,
						alpha:.8})
					}));
	
				main.map.addOverlay(blankline);
	    	}
	    }
	    
	    public function addPointLink(pl:PointLink):void {
	    	// Create the group to be drawn
	    	if(pl.slice != main.selectedSlice)
	    		return;
	    	
    		// Add line
			var polyline:Polyline = new Polyline([
				new LatLng(pl.node1.GetLatitude(), pl.node1.GetLongitude()),
				new LatLng(pl.node2.GetLatitude(), pl.node2.GetLongitude())
				], new PolylineOptions({ strokeStyle: new StrokeStyle({
					color: 0xFF00FF,
					thickness: 4,
					alpha:1})
				}));

			main.map.addOverlay(polyline);
	    }
	    
	    public function drawAll():void {
	    	drawMap();
	    	main.fillCombobox();
	    }
	    
	    public function drawMap():void {
	    	main.map.closeInfoWindow();
	    	main.map.clearOverlays();
	    	
	    	main.setProgress("Drawing map",Common.waitColor);
	    	
	    	for each(var g:NodeGroup in main.pgHandler.Nodes.collection) {
	        	addMarker(g);
	        }
	        
	        var drawSlice:Boolean = main.userResourcesOnly && main.selectedSlice != null && main.selectedSlice.status != null;
	        if(drawSlice) {
	        	for each(var drawGroup:LinkGroup in main.pgHandler.Links.collection) {
	        		// Add line
					var blankline:Polyline = new Polyline([
						new LatLng(drawGroup.latitude1, drawGroup.longitude1),
						new LatLng(drawGroup.latitude2, drawGroup.longitude2)
						], new PolylineOptions({ strokeStyle: new StrokeStyle({
							color: 0x666666,
							thickness: 3,
							alpha:.8})
						}));
		
					main.map.addOverlay(blankline);
	        	}
	        	for each(var pl:PointLink in main.selectedSlice.Links) {
		        	addPointLink(pl);
		        }
	        } else {
	        	for each(var l:LinkGroup in main.pgHandler.Links.collection) {
		        	if(!l.IsSameSite()) {
		        		addLink(l);
		        	}
		        }
	        }
	        
	        main.setProgress("Done", Common.successColor);
	    }
	    
	    public function viewNodeGroup(group:NodeGroup):void {
	    	var ngWindow:NodeGroupAdvancedWindow = new NodeGroupAdvancedWindow();
	    	ngWindow.main = main;
	    	PopUpManager.addPopUp(ngWindow, main, false);
       		PopUpManager.centerPopUp(ngWindow);
       		ngWindow.loadGroup(group);
	    }
	    
	    public function viewLinkGroup(group:LinkGroup):void {
	    	var lgWindow:LinkGroupAdvancedWindow = new LinkGroupAdvancedWindow();
	    	lgWindow.main = main;
	    	PopUpManager.addPopUp(lgWindow, main, false);
       		PopUpManager.centerPopUp(lgWindow);
       		lgWindow.loadGroup(group);
       		
	    }
	}
}