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
		public function PhysicalNode(own:PhysicalNodeGroup)
		{
			owner = own;
		}
		
		public var owner:PhysicalNodeGroup;
		
		[Bindable]
		public var name:String;
		
		[Bindable]
		public var urn:String;
		
		[Bindable]
		public var managerString:String;
		
		[Bindable]
		public var manager:GeniManager;
		
		[Bindable]
		public var available:Boolean;
		
		[Bindable]
		public var exclusive:Boolean;
		
		[Bindable]
		public var subNodeOf : PhysicalNode = null;
		public var subNodes : ArrayCollection = new ArrayCollection();
		public var virtualNodes : ArrayCollection = new ArrayCollection();
		
		public var diskImages:ArrayCollection = new ArrayCollection();
		
		[Bindable]
		public var types:ArrayCollection = new ArrayCollection();
		
		[Bindable]
		public var interfaces:PhysicalNodeInterfaceCollection = new PhysicalNodeInterfaceCollection();
		
		public var rspec:XML;
		
		public function IsSwitch():Boolean {
			for each(var d:NodeType in types) {
				if(d.name == "switch")
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
		
		public function GetLinks():ArrayCollection {
			var ac:ArrayCollection = new ArrayCollection();
			for each(var i:PhysicalNodeInterface in interfaces.collection) {
				for each(var l:PhysicalLink in i.links) {
					ac.addItem(l);
				}
			}
			return ac;
		}
		
		public function GetNodes():ArrayCollection {
			var ac:ArrayCollection = new ArrayCollection();
			for each(var i:PhysicalNodeInterface in interfaces.collection) {
				for each(var l:PhysicalLink in i.links) {
					if(l.interface1.owner != this && !ac.contains(l.interface1.owner)) {
						ac.addItem(l.interface1.owner);
					}
					if(l.interface2.owner != this && !ac.contains(l.interface2.owner)) {
						ac.addItem(l.interface2.owner);
					}
				}
			}
			return ac;
		}
		
		public function GetNodeLinks(n:PhysicalNode):ArrayCollection {
			var ac:ArrayCollection = new ArrayCollection();
			for each(var i:PhysicalNodeInterface in interfaces.collection) {
				for each(var l:PhysicalLink in i.links) {
					if(l.interface1.owner == n || l.interface2.owner == n) {
						ac.addItem(l);
					}
				}
			}
			return ac;
		}
	}
}