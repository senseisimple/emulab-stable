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

package protogeni.resources
{
	// Sliver from a slice containing all resources from the CM
	public class Sliver
	{
		public static const STATE_STARTED:String = "started";
		public static const STATE_STOPPED:String = "stopped";
		public static const STATE_MIXED:String = "mixed";
		public static const STATE_NA:String = "N/A";
		
		public static const STATUS_CHANGING:String = "changing";
		public static const STATUS_READY:String = "ready";
		public static const STATUS_NOTREADY:String = "notready";
		public static const STATUS_FAILED:String = "failed";
		public static const STATUS_UNKOWN:String = "unknown";
		public static const STATUS_MIXED:String = "mixed";
		public static const STATUS_NA:String = "N/A";
		
		public var created:Boolean = false;
		public var staged:Boolean = false;
		
		public var credential:Object = null;
		public var manager:GeniManager = null;
		public var rspec:XML = null;
		[Bindable]
		public var urn:IdnUrn = new IdnUrn();
		
		public var ticket:XML;
		public var manifest:XML;
		
		public var state:String;
		public var status:String;
		
		public var nodes:VirtualNodeCollection = new VirtualNodeCollection();
		public var links:VirtualLinkCollection = new VirtualLinkCollection();
		
		[Bindable]
		public var slice:Slice;
		
		public var expires:Date;
		
		public function Sliver(owner:Slice,
							   newManager:GeniManager = null)
		{
			this.slice = owner;
			this.manager = newManager;
		}
		
		public function reset():void
		{
			this.nodes = new VirtualNodeCollection();
			this.links = new VirtualLinkCollection();
			this.state = "";
			this.status = "";
		}
		
		public function localNodes():VirtualNodeCollection
		{
			var on:VirtualNodeCollection = new VirtualNodeCollection();
			for each(var vn:VirtualNode in this.nodes.collection)
			{
				if(vn.manager == this.manager)
					on.add(vn);
			}
			return on;
		}
		
		public function outsideNodes():VirtualNodeCollection
		{
			var on:VirtualNodeCollection = new VirtualNodeCollection();
			for each(var vn:VirtualNode in this.nodes.collection)
			{
				if(vn.manager != this.manager)
					on.add(vn);
			}
			return on;
		}
		
		public function getVirtualNodeFor(pn:PhysicalNode):VirtualNode
		{
			for each(var vn:VirtualNode in this.nodes.collection)
			{
				if(vn.physicalNode == pn)
					return vn;
			}
			
			return null;
		}
		
		public function getRequestRspec(removeNonexplicitBinding:Boolean):XML
		{
			return this.manager.rspecProcessor.generateSliverRspec(this, removeNonexplicitBinding);
		}
		
		public function parseRspec():void
		{
			return this.manager.rspecProcessor.processSliverRspec(this);
		}
		
		public function removeOutsideReferences():void
		{
			for each(var node:VirtualNode in this.nodes.collection)
			{
				if(node.physicalNode.virtualNodes.contains(node))
					node.physicalNode.virtualNodes.remove(node);
			}
		}
	}
}