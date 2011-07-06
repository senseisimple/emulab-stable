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
	import protogeni.Util;
	
	public final class SliverCollection
	{
		public var collection:Vector.<Sliver>;
		public function SliverCollection()
		{
			this.collection = new Vector.<Sliver>();
		}
		
		public function add(s:Sliver):void {
			this.collection.push(s);
		}
		
		public function remove(s:Sliver):void
		{
			var idx:int = this.collection.indexOf(s);
			if(idx > -1)
				this.collection.splice(idx, 1);
		}
		
		public function contains(s:Sliver):Boolean
		{
			return this.collection.indexOf(s) > -1;
		}
		
		public function get length():int{
			return this.collection.length;
		}
		
		public function addIfNotExisting(s:Sliver):void
		{
			for each(var sliver:Sliver in this.collection)
			{
				if(sliver.manager == s.manager)
					return;
			}
			this.collection.push(s);
		}
		
		public function clone():SliverCollection
		{
			var sc:SliverCollection = new SliverCollection();
			for each(var sliver:Sliver in this.collection)
				sc.add(sliver);
			return sc;
		}
		
		public function getByUrn(urn:String):Sliver
		{
			for each(var sliver:Sliver in this.collection)
			{
				if(sliver.urn.full == urn)
					return sliver;
			}
			return null;
		}
		
		public function getByGm(gm:GeniManager):Sliver
		{
			for each(var sliver:Sliver in this.collection)
			{
				if(sliver.manager == gm)
					return sliver;
			}
			return null;
		}
		
		public function getCombined():Sliver
		{
			if(this.collection.length == 0)
				return null;
			var fakeManager:ProtogeniComponentManager = new ProtogeniComponentManager();
			if(this.collection.length > 0)
				fakeManager.inputRspecVersion = this.collection[0].slice.useInputRspecVersion;
			else
				fakeManager.inputRspecVersion = Util.defaultRspecVersion;
			fakeManager.Hrn = "Combined";
			var newSliver:Sliver = new Sliver(null, fakeManager);
			for each(var sliver:Sliver in this.collection) {
				for each(var ns:Namespace in sliver.extensionNamespaces) {
					if(newSliver.extensionNamespaces.indexOf(ns) == -1)
						newSliver.extensionNamespaces.push(ns);
				}
				if(newSliver.slice == null)
					newSliver.slice = sliver.slice;
				for each(var node:VirtualNode in sliver.nodes.collection) {
					if(!newSliver.nodes.contains(node))
						newSliver.nodes.add(node);
				}
				for each(var link:VirtualLink in sliver.links.collection) {
					if(!newSliver.links.contains(link))
						newSliver.links.add(link);
				}
			}
			return newSliver;
		}
		
		public function expires():Date {
			var d:Date = null;
			for each(var sliver:Sliver in this.collection)
			{
				if(d == null || sliver.expires < d)
					d = sliver.expires;
			}
			return d;
		}
	}
}