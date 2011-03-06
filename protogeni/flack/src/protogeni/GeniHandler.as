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
 
 package protogeni
{
	 import flash.net.SharedObject;
	 
	 import mx.collections.ArrayList;
	 import mx.controls.Alert;
	 import mx.events.CloseEvent;
	 
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
		public var certBundleUrl:String = "http://www.emulab.net/genica.bundle";
		public var rootBundleUrl:String = "http://www.emulab.net/rootca.bundle";
		
		public var forceAuthority:SliceAuthority = null;
		
		public var forceMapKey:String = null;
		
		public var GeniManagers:GeniManagerCollection;
		
		[Bindable]
		public var GeniAuthorities:ArrayList;

		public function GeniHandler()
		{
			requestHandler = new GeniRequestHandler();
			mapHandler = new protogeni.display.mapping.GeniMapHandler(Main.Application().map);
			GeniManagers = new GeniManagerCollection();
			CurrentUser = new GeniUser();
			unauthenticatedMode = true;
			GeniAuthorities = new ArrayList([
				new SliceAuthority("utahemulab.sa","urn:publicid:IDN+emulab.net+authority+sa","https://www.emulab.net/protogeni/xmlrpc", true),
				new SliceAuthority("ukgeni.sa","urn:publicid:IDN+uky.emulab.net+authority+sa","https://www.uky.emulab.net/protogeni/xmlrpc", true),
				new SliceAuthority("wail.sa","urn:publicid:IDN+schooner.wail.wisc.edu+authority+sa","https://www.schooner.wail.wisc.edu/protogeni/xmlrpc"),
				new SliceAuthority("umlGENI.sa","urn:publicid:IDN+uml.emulab.net+authority+sa","https://boss.uml.emulab.net/protogeni/xmlrpc"),
				new SliceAuthority("cmulab.sa","urn:publicid:IDN+cmcl.cs.cmu.edu+authority+sa","https://boss.cmcl.cs.cmu.edu/protogeni/xmlrpc"),
				new SliceAuthority("myelab.sa","urn:publicid:IDN+myelab.testbed.emulab.net+authority+sa","https://myboss.myelab.testbed.emulab.net/protogeni/xmlrpc"),
				new SliceAuthority("bbn-pgeni.sa","urn:publicid:IDN+pgeni.gpolab.bbn.com+authority+sa","https://www.pgeni.gpolab.bbn.com/protogeni/xmlrpc"),
				new SliceAuthority("jonlab.sa","urn:publicid:IDN+jonlab.testbed.emulab.net+authority+sa","https://myboss.jonlab.testbed.emulab.net/protogeni/xmlrpc"),
				new SliceAuthority("cis.fiu.edu.sa","urn:publicid:IDN+cis.fiu.edu+authority+sa","https://pg-boss.cis.fiu.edu/protogeni/xmlrpc"),
				new SliceAuthority("geelab.sa","urn:publicid:IDN+elabinelab.geni.emulab.net+authority+sa","https://myboss.elabinelab.geni.emulab.net/protogeni/xmlrpc"),
				new SliceAuthority("bbn-pgeni3.sa","urn:publicid:IDN+pgeni3.gpolab.bbn.com+authority+sa","https://www.pgeni3.gpolab.bbn.com/protogeni/xmlrpc"),
				new SliceAuthority("cron.cct.lsu.edu.sa","urn:publicid:IDN+cron.cct.lsu.edu+authority+sa","https://www.cron.cct.lsu.edu/protogeni/xmlrpc"),
				new SliceAuthority("chihuas.sa","urn:publicid:IDN+chi.itesm.mx+authority+sa","https://boss.chi.itesm.mx/protogeni/xmlrpc/"),
				new SliceAuthority("bbn-pgeni1.sa","urn:publicid:IDN+pgeni1.gpolab.bbn.com+authority+sa","https://www.pgeni1.gpolab.bbn.com/protogeni/xmlrpc")
								]);
			CurrentUser.authority = GeniAuthorities.source[0] as SliceAuthority;
			
			Main.geniDispatcher.dispatchUserChanged();
			//Main.geniDispatcher.dispatchGeniManagersChanged();
			Main.geniDispatcher.dispatchQueueChanged();
		}
		
		public function destroy():void {
			mapHandler.destruct();
			requestHandler.pause();
			requestHandler.clearAll();
		}
		
		public function clearComponents() : void
		{
			Main.geniDispatcher.dispatchGeniManagersChanged(GeniEvent.ACTION_REMOVING);
			GeniManagers = new GeniManagerCollection();
			if(CurrentUser != null)
				CurrentUser.slices = new SliceCollection();
		}
		
		public function search(s:String, matchAll:Boolean):Array
		{
			var searchFrom:Array = s.split(' ');
			var results:Array = new Array();
			for each(var gm:GeniManager in this.GeniManagers)
			{
				if(Util.findInAny(searchFrom, new Array(gm.Urn.full, gm.Hrn, gm.Url), matchAll))
					results.push(DisplayUtil.getGeniManagerButton(gm));
				for each(var pn:PhysicalNode in gm.AllNodes)
				{
					if(Util.findInAny(searchFrom, new Array(pn.id, pn.name), matchAll))
						results.push(DisplayUtil.getPhysicalNodeButton(pn));
				}
				for each(var pl:PhysicalLink in gm.AllLinks)
				{
					//if(pl.urn == s)
					//	results.push(DisplayUtil.getLinkButton((pn));
				}
			}
			
			for each(var slice:Slice in this.CurrentUser.slices)
			{
				if(Util.findInAny(searchFrom, new Array(slice.urn.full, slice.hrn, slice.uuid), matchAll))
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