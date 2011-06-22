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
	/**
	 * Responsible for users and their slices in ProtoGENI
	 * 
	 * @author mstrum
	 * 
	 */
	public class SliceAuthority
	{
		[Bindable]
		public var Name:String;
		[Bindable]
		public var Urn:IdnUrn;
		[Bindable]
		public var Url:String;
		[Bindable]
		public var workingCertGet:Boolean = false;
		
		public function SliceAuthority(newUrn:String,
									   newUrl:String,
									   newWorkingCertGet:Boolean = false)
		{
			this.Urn = new IdnUrn(newUrn);
			this.Name = this.Urn.authority;
			this.Url = newUrl;
			this.workingCertGet = newWorkingCertGet;
		}
	}
}