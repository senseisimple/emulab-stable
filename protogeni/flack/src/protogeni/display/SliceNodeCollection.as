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

package protogeni.display
{
	import mx.collections.ArrayCollection;
	
	import protogeni.resources.PhysicalNode;
	import protogeni.resources.VirtualNode;
	
	public class SliceNodeCollection extends ArrayCollection
	{
		public function SliceNodeCollection(source:Array=null)
		{
			super(source);
		}
		
		public function containsPhysicalNode(node:PhysicalNode):Boolean
		{
			for each (var sn:SliceNode in this)
			{
				if(sn.node.physicalNode == node)
					return true;
			}
			return false;
		}
		
		public function containsVirtualNode(node:VirtualNode):Boolean
		{
			for each (var sn:SliceNode in this)
			{
				if(sn.node == node)
					return true;
			}
			return false;
		}
		
		public function getForPhysicalNode(node:PhysicalNode):SliceNode
		{
			for each (var sn:SliceNode in this)
			{
				if(sn.node.physicalNode == node)
					return sn;
			}
			return null;
		}
		
		public function getForVirtualNode(node:VirtualNode):SliceNode
		{
			for each (var sn:SliceNode in this)
			{
				if(sn.node == node)
					return sn;
			}
			return null;
		}
		
		public function seperateNodesByComponentManager():Array
		{
			var completedCms:ArrayCollection = new ArrayCollection();
			var completed:Array = new Array();
			for each(var sn:SliceNode in this)
			{
				if(!completedCms.contains(sn.node.manager))
				{
					var cmNodes:SliceNodeCollection = new SliceNodeCollection();
					for each(var sncm:SliceNode in this)
					{
						if(sncm.node.manager == sn.node.manager)
							cmNodes.addItem(sncm);
					}
					if(cmNodes.length > 0)
						completed.push(cmNodes);
				}
			}
			return completed;
		}
	}
}