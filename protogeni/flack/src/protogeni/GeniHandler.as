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
	import mx.collections.ArrayList;
	
	import protogeni.communication.GeniRequestHandler;
	import protogeni.display.DisplayUtil;
	import protogeni.display.mapping.GeniMapHandler;
	import protogeni.resources.GeniManager;
	import protogeni.resources.GeniManagerCollection;
	import protogeni.resources.GeniUser;
	import protogeni.resources.PhysicalLink;
	import protogeni.resources.PhysicalNode;
	import protogeni.resources.Slice;
	import protogeni.resources.SliceAuthority;
	import protogeni.resources.SliceCollection;
	import protogeni.resources.Sliver;
	import protogeni.resources.VirtualLink;
	import protogeni.resources.VirtualNode;
	
	// Holds and handles all information regarding ProtoGENI
	/**
	 * Holds all of the current GENI data, possibly the most referenced variable. Only one is needed.
	 * 
	 * @author mstrum
	 * 
	 */
	public final class GeniHandler
	{
		[Bindable]
		public var requestHandler:GeniRequestHandler;
		
		[Bindable]
		public var mapHandler:protogeni.display.mapping.GeniMapHandler;
		
		[Bindable]
		public var CurrentUser:GeniUser;
		
		[Bindable]
		public var unauthenticatedMode:Boolean;
		
		public var publicUrl:String = "https://www.emulab.net/protogeni/advertisements/list.txt";
		public var salistUrl:String = "https://www.emulab.net/protogeni/authorities/salist.txt";
		public var certBundleUrl:String = "http://www.emulab.net/genica.bundle";
		public var rootBundleUrl:String = "http://www.emulab.net/rootca.bundle";
		
		public var forceAuthority:SliceAuthority = null;
		
		public var forceMapKey:String = null;
		
		[Bindable]
		public var GeniManagers:GeniManagerCollection;
		
		[Bindable]
		public var GeniAuthorities:ArrayList;
		
		public function GeniHandler()
		{
			this.requestHandler = new GeniRequestHandler();
			this.mapHandler = new GeniMapHandler(Main.Application().map);
			this.GeniManagers = new GeniManagerCollection();
			this.GeniAuthorities = new ArrayList();
			this.CurrentUser = new GeniUser();
			this.unauthenticatedMode = true;

			Main.geniDispatcher.dispatchUserChanged();
			//Main.geniDispatcher.dispatchGeniManagersChanged();
			Main.geniDispatcher.dispatchQueueChanged();
		}
		
		public function destroy():void {
			this.requestHandler.stop();
			this.clearComponents();
			this.mapHandler.destruct();
		}
		
		public function clearComponents() : void
		{
			Main.geniDispatcher.dispatchGeniManagersChanged(GeniEvent.ACTION_REMOVING);
			Main.geniDispatcher.dispatchSlicesChanged(GeniEvent.ACTION_REMOVING);
			if(this.CurrentUser != null) {
				this.CurrentUser.slices.removeOutsideReferences();
				this.CurrentUser.slices = new SliceCollection();
				Main.geniDispatcher.dispatchSlicesChanged(GeniEvent.ACTION_REMOVED);
			}
			this.GeniManagers = new GeniManagerCollection();
			Main.geniDispatcher.dispatchGeniManagersChanged(GeniEvent.ACTION_REMOVED);
		}
		
		public function search(s:String, matchAll:Boolean):Array
		{
			var searchFrom:Array = s.split(' ');
			var results:Array = new Array();
			for each(var manager:GeniManager in this.GeniManagers)
			{
				if(Util.findInAny(searchFrom, new Array(manager.Urn.full, manager.Hrn, manager.Url), matchAll))
					results.push(DisplayUtil.getGeniManagerButton(manager));
				for each(var pn:PhysicalNode in manager.AllNodes)
				{
					if(Util.findInAny(searchFrom, new Array(pn.id, pn.name), matchAll))
						results.push(DisplayUtil.getPhysicalNodeButton(pn));
				}
				for each(var pl:PhysicalLink in manager.AllLinks)
				{
					//if(pl.urn == s)
					//	results.push(DisplayUtil.getLinkButton((pn));
				}
			}
			
			for each(var slice:Slice in this.CurrentUser.slices)
			{
				if(Util.findInAny(searchFrom,
									new Array(slice.urn.full,
												slice.hrn),
									matchAll))
					results.push(DisplayUtil.getSliceButton(slice));
				for each(var sliver:Sliver in slice.slivers.collection)
				{
					//if(sliver.urn == s)
					//results.push(DisplayUtil.getSliverButton();
					for each(var vn:VirtualNode in sliver.nodes.collection)
					{
						//if(vn.urn == s || vn.uuid == s)
						//results.push(DisplayUtil.getVirtualNodeButton());
					}
					for each(var vl:VirtualLink in sliver.links.collection)
					{
						//if(vn.urn == s || vn.uuid == s)
						//results.push(DisplayUtil.getVirtualNodeButton());
					}
				}
			}
			
			//if(this.CurrentUser.uid == s || this.CurrentUser.uuid == s)
			//results.push(DisplayUtil.getLinkButton(this.CurrentUser);
			
			return results;
		}
	}
}