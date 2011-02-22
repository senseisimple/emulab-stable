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
	// Sliver from a slice containing all resources from the CM
	public class Sliver
	{
		public static var STATE_READY : String = "ready";
	    public static var STATE_NOTREADY : String = "notready";
	    public static var STATE_FAILED : String = "failed";
		
		public static var STATUS_CHANGING:String = "changing";
		public static var STATUS_READY:String = "ready";
		public static var STATUS_NOTREADY:String = "notready";
		public static var STATUS_FAILED:String = "changing";
		public static var STATUS_UNKOWN:String = "unknown";
		public static var STATUS_MIXED:String = "mixed";
		
		public var created:Boolean = false;
	    
		public var credential : Object = null;
		public var manager:GeniManager = null;
		public var rspec : XML = null;
		[Bindable]
		public var urn : String = null;
		
		public var ticket:XML;
		public var manifest:XML;
		
		public var state : String;
		public var status : String;
		
		public var nodes:VirtualNodeCollection = new VirtualNodeCollection();
		public var links:VirtualLinkCollection = new VirtualLinkCollection();
		
		[Bindable]
		public var slice : Slice;
		
		public var validUntil:Date;
		
		public function Sliver(owner : Slice, newManager:GeniManager = null)
		{
			slice = owner;
			manager = newManager;
		}
		
		public function reset():void
		{
			nodes = new VirtualNodeCollection();
			links = new VirtualLinkCollection();
			state = "";
			status = "";
		}
		
		public function localNodes():VirtualNodeCollection
		{
			var on:VirtualNodeCollection = new VirtualNodeCollection();
			for each(var vn:VirtualNode in this.nodes)
			{
				if(vn.manager == this.manager)
					on.addItem(vn);
			}
			return on;
		}
		
		public function outsideNodes():VirtualNodeCollection
		{
			var on:VirtualNodeCollection = new VirtualNodeCollection();
			for each(var vn:VirtualNode in this.nodes)
			{
				if(vn.manager != this.manager)
					on.addItem(vn);
			}
			return on;
		}
		
		public function getVirtualNodeFor(pn:PhysicalNode):VirtualNode
		{
			for each(var vn:VirtualNode in this.nodes)
			{
				if(vn.physicalNode == pn)
					return vn;
			}
			
			return null;
		}
		
		public function getRequestRspec():XML
		{
			return manager.rspecProcessor.generateSliverRspec(this);
		}
		
		public function parseRspec():void
		{
			return manager.rspecProcessor.processSliverRspec(this);
		}
		
		public function removeOutsideReferences():void
		{
			for each(var node:VirtualNode in this.nodes)
			{
				if(node.physicalNode.virtualNodes.getItemIndex(node) > -1)
					node.physicalNode.virtualNodes.removeItemAt(node.physicalNode.virtualNodes.getItemIndex(node));
			}
		}
	}
}