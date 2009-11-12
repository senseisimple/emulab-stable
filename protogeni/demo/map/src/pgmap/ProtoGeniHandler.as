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
 
 package pgmap
{
	import mx.collections.ArrayCollection;
	
	public class ProtoGeniHandler
	{
		public var main : pgmap;
		
		[Bindable]
		public var rpc : ProtoGeniRpcHandler;
		
		[Bindable]
		public var map : ProtoGeniMapHandler;
		
		[Bindable]
		public var CurrentUser:User;
		
		public var ComponentManagers : ArrayCollection;

		public function ProtoGeniHandler()
		{
			main = Common.Main();
			rpc = new ProtoGeniRpcHandler();
			map = new ProtoGeniMapHandler();
			ComponentManagers = new ArrayCollection();
			CurrentUser = new User();
			rpc.main = main;
			map.main = main;
		}
		
		public function clearAll() : void
		{
			ComponentManagers = new ArrayCollection();
		}
		
		public function clearComponents() : void
		{
			for each(var cm : ComponentManager in this.ComponentManagers)
			{
				 cm.Nodes.collection.removeAll();
				 cm.Links.collection.removeAll();
				 cm.Status = ComponentManager.UNKOWN;
				 cm.Message = "";
			}
		}
		
		public function getCredential(afterCompletion : Function):void {
			rpc.AfterCall = afterCompletion;
			rpc.startCredential();
		}
		
		public function guarenteeCredential(afterCompletion : Function):void {
			if(CurrentUser.credential != null)
				afterCompletion();
			else
				getCredential(afterCompletion);
		}
		
		public function getComponents(afterCompletion : Function):void {
			clearAll();
			rpc.AfterCall = afterCompletion;
			rpc.startListComponents();
		}
		
		public function getResourcesAndSlices():void {
			clearComponents();
			for each ( var cm:ComponentManager in ComponentManagers)
				cm.Status = ComponentManager.INPROGRESS;
			main.chooseCMWindow.ResetAll();
			rpc.startIndexedCall(rpc.startResourceLookup);
		}
	}
}