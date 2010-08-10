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
	import mx.collections.ArrayCollection;
	
	import protogeni.display.DisplayUtil;
	
	// Slice that a user created in ProtoGENI
	public class Slice
	{
		// Status values
	    public static var CHANGING : String = "changing";
		public static var READY : String = "ready";
	    public static var NOTREADY : String = "notready";
	    public static var FAILED : String = "failed";
	    public static var UNKNOWN : String = "unknown";
	    public static var NA : String = "N/A";
	    
	    // State values
	    public static var STARTED : String = "started";
		public static var STOPPED : String = "stopped";
	    public static var MIXED : String = "mixed";
	    
		public var uuid : String = null;
		[Bindable]
		public var hrn : String = null;
		[Bindable]
		public var urn : String = null;
		public var creator : User = null;
		public var credential : String = "";
		public var slivers:SliverCollection = new SliverCollection();
		
		public var validUntil:Date;

		public function Slice()
		{
		}
		
		public function Status():String {
			if(hrn == null) return null;
			var status:String = NA;
			for each(var sliver:Sliver in slivers) {
				if(status == NA) status = sliver.status;
				else {
					if(sliver.status != status)
						return "mixed";
				}
			}
			return status;
		}
		
		public function GetAllNodes():ArrayCollection
		{
			var nodes:ArrayCollection = new ArrayCollection();
			for each(var s:Sliver in slivers)
			{
				for each(var n:VirtualNode in s.nodes)
				{
					if(!nodes.contains(n))
						nodes.addItem(n);
				}
					
			}
			return nodes;
		}
		
		public function GetPhysicalNodes():ArrayCollection
		{
			var nodes:ArrayCollection = new ArrayCollection();
			for each(var s:Sliver in slivers)
			{
				for each(var n:VirtualNode in s.nodes)
				{
					if(!nodes.contains(n) && n.physicalNode != null)
						nodes.addItem(n.physicalNode);
				}
				
			}
			return nodes;
		}
		
		public function hasSliverFor(cm:ComponentManager):Boolean
		{
			for each(var s:Sliver in slivers)
			{
				if(s.componentManager == cm)
					return true;
			}
			return false;
		}
		
		public function getOrCreateSliverFor(cm:ComponentManager):Sliver
		{
			for each(var s:Sliver in slivers)
			{
				if(s.componentManager == cm)
					return s;
			}
			var newSliver:Sliver = new Sliver(this, cm);
			slivers.addItem(newSliver);
			return newSliver;
		}
		
		public function clone(addOutsideReferences:Boolean = true):Slice
		{
			var newSlice:Slice = new Slice();
			newSlice.uuid = this.uuid;
			newSlice.hrn = this.hrn;
			newSlice.urn = this.urn;
			newSlice.creator = this.creator;
			newSlice.credential = this.credential;
			newSlice.validUntil = this.validUntil;
			for each(var sliver:Sliver in this.slivers)
				newSlice.slivers.addItem(sliver.clone(addOutsideReferences));
			// Deal with the pointers to slivers since they're all created
			for each(sliver in this.slivers)
			{
				for each(var node:VirtualNode in sliver.nodes)
				{
					for each(var nodeSliver:Sliver in node.slivers)
						newSlice.slivers.getByUrn(sliver.urn).nodes.getById(node.id).slivers.push(newSlice.slivers.getByUrn(nodeSliver.urn));
				}
				for each(var link:VirtualLink in sliver.links)
				{
					for each(var linkSliver:Sliver in link.slivers)
						newSlice.slivers.getByUrn(sliver.urn).links.getById(link.id).slivers.push(newSlice.slivers.getByUrn(linkSliver.urn));
				}
			}
			return newSlice;
		}
		
		public function ReadyIcon():Class {
			switch(Status()) {
				case READY : return DisplayUtil.flagGreenIcon;
				case NOTREADY : return DisplayUtil.flagYellowIcon;
				case FAILED : return DisplayUtil.flagRedIcon;
				default : return null;
			}
		}
		
		public function DisplayString():String {
			
			if(hrn == null && uuid == null) {
				return "All Resources";
			}
			
			var returnString : String;
			if(hrn == null)
				returnString = uuid;
			else
				returnString = hrn;
				
			return returnString + " (" + Status() + ")";
		}
		
		// Used to push more important slices to the top of lists
		public function CompareValue():int {
			
			if(hrn == null && uuid == null) {
				return -1;
			}
			
			if(Status() == "ready")
				return 0;
			else
				return 1;
		}
		
		public function getUniqueVirtualLinkId(l:VirtualLink = null):String
		{
			var highest:int = 0;
			for each(var s:Sliver in slivers)
			{
				for each(var l:VirtualLink in s.links)
				{
					try
					{
						if(l.id.substr(0,5) == "link-")
						{
							var testHighest:int = parseInt(l.id.substring(5));
							if(testHighest >= highest)
								highest = testHighest+1;
						}
					} catch(e:Error)
					{
						
					}
				}
			}
			return "link-" + highest;
			/*
			if(l == null)
				return "link-" + highest;
			else
			{
				if(l.isTunnel())
					return "tunnel-" + highest;
				else
					return "link-" + highest;
			}
			*/
		}
		
		public function getUniqueVirtualNodeId(n:VirtualNode = null):String
		{
			var highest:int = 0;
			for each(var s:Sliver in slivers)
			{
				for each(var n:VirtualNode in s.nodes)
				{
					try
					{
						var testHighest:int = parseInt(n.id.substring(n.id.lastIndexOf("-")+1));
						if(testHighest >= highest)
							highest = testHighest+1;
					} catch(e:Error)
					{
						
					}
				}
			}
			if(n == null)
				return "node-" + highest;
			else
			{
				if(n.isShared)
					return "shared-" + highest;
				else
					return "exclusive-" + highest;
			}
		}
		
		public function getUniqueVirtualInterfaceId():String
		{
			var highest:int = 0;
			for each(var s:Sliver in slivers)
			{
				for each(var l:VirtualLink in s.links)
				{
					for each(var i:VirtualInterface in l.interfaces)
					{
						try
						{
							if(i.id.substr(0,10) == "interface-")
							{
								var testHighest:int = parseInt(i.id.substring(10));
								if(testHighest >= highest)
									highest = testHighest+1;
							}
						} catch(e:Error)
						{
							
						}
					}
					
				}
			}
			return "interface-" + highest;
		}
	}
}