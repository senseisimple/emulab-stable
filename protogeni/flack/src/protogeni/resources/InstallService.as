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
	 * Archive to install on a resource
	 * 
	 * @author mstrum
	 * 
	 */
	public class InstallService
	{
		public var url:String;
		public var installPath:String;
		public var fileType:String;
		
		public function InstallService(newUrl:String = "",
									   newInstallPath:String = "/",
									   newFileType:String = "tar.gz")
		{
			this.url = newUrl;
			this.installPath = newInstallPath;
			this.fileType = newFileType;
		}
	}
}