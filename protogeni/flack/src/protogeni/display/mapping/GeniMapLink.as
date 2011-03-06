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

package protogeni.display.mapping
{
	import com.google.maps.LatLng;
	import com.google.maps.overlays.Polyline;
	import com.google.maps.overlays.PolylineOptions;
	import com.google.maps.styles.StrokeStyle;
	
	import flash.events.Event;
	import flash.events.MouseEvent;
	
	import protogeni.Util;
	import protogeni.display.DisplayUtil;
	import protogeni.resources.PhysicalLinkGroup;
	
	public class GeniMapLink
	{
		public static const linkColor:Object = 0xFFCFD1;
		public static const linkBorderColor:Object = 0xFF00FF;
		
		public var polyline:Polyline;
		public var label:TooltipOverlay;
		public var group:PhysicalLinkGroup;
		public var added:Boolean = false;
		
		public function GeniMapLink(linkGroup:PhysicalLinkGroup)
		{
			group = linkGroup;
			
			// Add line
			polyline = new Polyline([
				new LatLng(linkGroup.latitude1, linkGroup.longitude1),
				new LatLng(linkGroup.latitude2, linkGroup.longitude2)
			], new PolylineOptions({ strokeStyle: new StrokeStyle({
				color: linkBorderColor,
				thickness: 4,
				alpha:1})
			}));
			
			// Add link marker
			label = new TooltipOverlay(
				new LatLng((linkGroup.latitude1 + linkGroup.latitude2)/2,
					(linkGroup.longitude1 + linkGroup.longitude2)/2),
				Util.kbsToString(linkGroup.TotalBandwidth()),
				linkBorderColor,
				linkColor);
			label.addEventListener(MouseEvent.CLICK, function(e:Event):void {
				e.stopImmediatePropagation();
				DisplayUtil.viewPhysicalLinkGroup(linkGroup)
			});
		}
	}
}