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
	import flash.utils.Dictionary;
	
	import mx.collections.ArrayCollection;
	
	// Collection of all physical node groups
	public class PhysicalNodeGroupCollection
	{
		public function PhysicalNodeGroupCollection(own:GeniManager)
		{
			owner = own;
		}
		
		public var collection:ArrayCollection = new ArrayCollection();
		public var owner:GeniManager;
		
		public function GetByLocation(lat:Number, lng:Number):PhysicalNodeGroup {
			for each(var ng:PhysicalNodeGroup in collection) {
				if(ng.latitude == lat && ng.longitude == lng)
					return ng;
			}
			return null;
		}
		
		public function GetByUrn(urn:String):PhysicalNode {
			for each(var ng:PhysicalNodeGroup in collection) {
				var n:PhysicalNode = ng.GetByUrn(urn);
				if(n != null)
					return n;
			}
			return null;
		}
		
		public function GetByName(name:String):PhysicalNode {
			for each(var ng:PhysicalNodeGroup in collection) {
				var n:PhysicalNode = ng.GetByName(name);
				if(n != null)
					return n;
			}
			return null;
		}
		
		public function GetByType(type:String):PhysicalNodeGroup {
			var group:PhysicalNodeGroup = new PhysicalNodeGroup();
			for each(var ng:PhysicalNodeGroup in collection) {
				group.collection.addAll(ng.GetByType(type).collection);
			}
			return group;
		}
		
		public function GetInterfaceByID(id:String):PhysicalNodeInterface {
			for each(var ng:PhysicalNodeGroup in collection) {
				for each(var n:PhysicalNode in ng.collection) {
					var ni:PhysicalNodeInterface = n.interfaces.GetByID(id);
					if(ni != null)
						return ni;
				}
			}
			return null;
		}
		
		public function Add(g:PhysicalNodeGroup):void {
			collection.addItem(g);
		}
		
		public function GetAll():Array
		{
			var d:Array = [];
			for each(var ng:PhysicalNodeGroup in collection) {
				for each(var n:PhysicalNode in ng.collection)
					d.push(n);
			}
			return d;
		}
		
		public function GetType():int
		{
			var type:int = -2;
			for each(var ng:PhysicalNodeGroup in collection) {
				for each(var n:PhysicalNode in ng.collection) {
					if(type == -2)
						type = n.manager.type;
					else if(n.manager.type != type)
						type = -1;
				}
			}
			return type;
		}
		
		public function GetManagers():Vector.<GeniManager>
		{
			var d:Dictionary = new Dictionary();
			var a:Array = [];
			var biggestManager:GeniManager;
			var max:int = 0;
			for each(var ng:PhysicalNodeGroup in collection) {
				if(a.indexOf(ng.GetManager()) == -1) {
					a.push(ng.GetManager());
					d[ng.GetManager()] = ng.collection.length;
				} else {
					d[ng.GetManager()] += ng.collection.length;
				}
				if(d[ng.GetManager()] > max) {
					biggestManager = ng.GetManager();
					max = d[ng.GetManager()];
				}
			}
			var v:Vector.<GeniManager> = new Vector.<GeniManager>();
			v.push(biggestManager);
			for each(var m:GeniManager in a) {
				if(m != biggestManager)
					v.push(m);
			}
			return v;
		}
	}
}