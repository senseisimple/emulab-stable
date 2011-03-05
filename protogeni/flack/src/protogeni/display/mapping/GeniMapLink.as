package protogeni.display.mapping
{
	import com.google.maps.LatLng;
	import com.google.maps.PaneId;
	import com.google.maps.overlays.Polyline;
	import com.google.maps.overlays.PolylineOptions;
	import com.google.maps.styles.StrokeStyle;
	
	import flash.events.Event;
	import flash.events.MouseEvent;
	
	import mx.core.FlexGlobals;
	
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