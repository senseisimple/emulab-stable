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
	import mx.collections.Sort;
	import mx.collections.SortField;
	import mx.utils.ObjectUtil;
	
	// ProtoGENI user information
	public class User
	{
		[Bindable]
		public var uid : String;
		
		public var uuid : String;
		public var hrn : String;
		public var email : String;
		public var name : String;
		public var credential : String;
		public var urn : String;
		
		public var slices:ArrayCollection;
		
		public function User()
		{
			slices = new ArrayCollection();
		}
		
		private function compareSlices(slicea:Slice, sliceb:Slice):int {
			return ObjectUtil.compare(slicea.CompareValue(), sliceb.CompareValue());
        }
		
		public function displaySlices():ArrayCollection {
			var ac : ArrayCollection = new ArrayCollection();
			ac.addItem(new Slice());
			for each(var s:Slice in slices) {
				ac.addItem(s);
			}
			
			var dataSortField:SortField = new SortField();
            dataSortField.compareFunction = compareSlices;
            
            var sort:Sort = new Sort();
            sort.fields = [dataSortField];
			ac.sort = sort;
	    	ac.refresh();
			return ac;
		}
	}
}