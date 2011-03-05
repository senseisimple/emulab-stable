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
	
	// Physical node
	public class PhysicalNode
	{
		public var owner:PhysicalNodeGroup;
		
		public var rspec:XML;
		
		[Bindable]
		public var name:String;
		[Bindable]
		public var id:String;
		[Bindable]
		public var manager:GeniManager;
		[Bindable]
		public var exclusive:Boolean;
		[Bindable]
		public var available:Boolean;
		[Bindable]
		public var subNodeOf : PhysicalNode = null;
		public var subNodes:Vector.<PhysicalNode> = new Vector.<PhysicalNode>();
		[Bindable]
		public var hardwareTypes:Vector.<String> = new Vector.<String>();
		[Bindable]
		public var sliverTypes:Vector.<SliverType> = new Vector.<SliverType>();
		[Bindable]
		public var interfaces:PhysicalNodeInterfaceCollection = new PhysicalNodeInterfaceCollection();

		// Sliced
		public var virtualNodes : ArrayCollection = new ArrayCollection();
		
		// Use for anything, more inmportantly any additions by non-Protogeni managers
		public var tag:*;
		
		public function PhysicalNode(own:PhysicalNodeGroup, ownedBy:GeniManager)
		{
			owner = own;
			manager = ownedBy;
		}
		
		public function IsSwitch():Boolean {
			for each(var d:String in hardwareTypes) {
				if(d == "switch")
					return true;
			}
			return false;
		}
		
		public function ConnectedSwitches():ArrayCollection {
			var connectedNodes:ArrayCollection = GetNodes();
			var connectedSwitches:ArrayCollection = new ArrayCollection();
			for each(var connectedNode:PhysicalNode in connectedNodes) {
				if(connectedNode.IsSwitch())
					connectedSwitches.addItem(connectedNode);
			}
			return connectedSwitches;
		}

		public function GetLatitude():Number {
			return owner.latitude;
		}

		public function GetLongitude():Number {
			return owner.longitude;
		}
		
		// Gets all links
		public function GetLinks():ArrayCollection {
			var ac:ArrayCollection = new ArrayCollection();
			for each(var i:PhysicalNodeInterface in interfaces.collection) {
				for each(var l:PhysicalLink in i.physicalLinks) {
					ac.addItem(l);
				}
			}
			return ac;
		}
		
		// Get links to a certain node
		public function GetNodeLinks(n:PhysicalNode):ArrayCollection {
			var ac:ArrayCollection = new ArrayCollection();
			for each(var i:PhysicalNodeInterface in interfaces.collection) {
				for each(var l:PhysicalLink in i.physicalLinks) {
					if(l.GetNodes().indexOf(n) > -1) {
						ac.addItem(l);
						break;
					}
				}
			}
			return ac;
		}
		
		// Gets connected nodes
		public function GetNodes():ArrayCollection {
			var ac:ArrayCollection = new ArrayCollection();
			for each(var i:PhysicalNodeInterface in interfaces.collection) {
				for each(var l:PhysicalLink in i.physicalLinks) {
					for each(var ln:PhysicalNode in l.GetNodes()) {
						if(ln != this && !ac.contains(ln))
							ac.addItem(ln);
					}
				}
			}
			return ac;
		}
		
	}
}