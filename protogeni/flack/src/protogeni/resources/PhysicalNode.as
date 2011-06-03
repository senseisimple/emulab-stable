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
		public var subNodeOf:PhysicalNode = null;
		public var subNodes:Vector.<PhysicalNode> = new Vector.<PhysicalNode>();
		[Bindable]
		public var hardwareTypes:Vector.<String> = new Vector.<String>();
		[Bindable]
		public var sliverTypes:Vector.<SliverType> = new Vector.<SliverType>();
		[Bindable]
		public var interfaces:PhysicalNodeInterfaceCollection = new PhysicalNodeInterfaceCollection();
		
		// Sliced
		public var virtualNodes:VirtualNodeCollection = new VirtualNodeCollection();
		
		// Use for anything, more inmportantly any additions by non-Protogeni managers
		public var tag:*;
		
		public function PhysicalNode(own:PhysicalNodeGroup,
									 ownedBy:GeniManager)
		{
			this.owner = own;
			this.manager = ownedBy;
		}
		
		public function IsSwitch():Boolean {
			return this.hardwareTypes.indexOf("switch") > 1;
		}
		
		public function ConnectedSwitches():Vector.<PhysicalNode> {
			var connectedNodes:Vector.<PhysicalNode> = this.GetNodes();
			var connectedSwitches:Vector.<PhysicalNode> = new Vector.<PhysicalNode>();
			for each(var connectedNode:PhysicalNode in connectedNodes) {
				if(connectedNode.IsSwitch())
					connectedSwitches.push(connectedNode);
			}
			return connectedSwitches;
		}
		
		public function GetLatitude():Number {
			return this.owner.latitude;
		}
		
		public function GetLongitude():Number {
			return this.owner.longitude;
		}
		
		// Gets all links
		public function GetLinks():Vector.<PhysicalLink> {
			var ac:Vector.<PhysicalLink> = new Vector.<PhysicalLink>();
			for each(var i:PhysicalNodeInterface in this.interfaces.collection) {
				for each(var l:PhysicalLink in i.physicalLinks)
					ac.push(l);
			}
			return ac;
		}
		
		// Get links to a certain node
		public function GetNodeLinks(n:PhysicalNode):Vector.<PhysicalLink> {
			var ac:Vector.<PhysicalLink> = new Vector.<PhysicalLink>();
			for each(var i:PhysicalNodeInterface in this.interfaces.collection) {
				for each(var l:PhysicalLink in i.physicalLinks) {
					if(ac.indexOf(l) == -1 && l.GetNodes().indexOf(n) > -1) {
						ac.push(l);
						break;
					}
				}
			}
			return ac;
		}
		
		// Gets connected nodes
		public function GetNodes():Vector.<PhysicalNode> {
			var ac:Vector.<PhysicalNode> = new Vector.<PhysicalNode>();
			for each(var i:PhysicalNodeInterface in this.interfaces.collection) {
				for each(var l:PhysicalLink in i.physicalLinks) {
					for each(var ln:PhysicalNode in l.GetNodes()) {
						if(ln != this && ac.indexOf(ln) == -1)
							ac.push(ln);
					}
				}
			}
			return ac;
		}
		
	}
}