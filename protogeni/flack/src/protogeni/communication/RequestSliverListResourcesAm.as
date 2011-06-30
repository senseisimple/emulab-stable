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

package protogeni.communication
{
	import flash.utils.ByteArray;
	
	import mx.utils.Base64Decoder;
	
	import protogeni.GeniEvent;
	import protogeni.StringUtil;
	import protogeni.resources.GeniManager;
	import protogeni.resources.ProtogeniRspecProcessor;
	import protogeni.resources.Sliver;
	
	/**
	 * Gets the manifest for the sliver using the GENI AM API
	 * 
	 * @author mstrum
	 * 
	 */
	public final class RequestSliverListResourcesAm extends Request
	{
		public var sliver:Sliver;
		
		public function RequestSliverListResourcesAm(s:Sliver/*, notFirst:Boolean = false*/):void
		{
			super("ListResources(Sliver)AM (" + StringUtil.shortenString(s.manager.Url, 15) + ")",
				"Listing resources for sliver on " + s.manager.Hrn + " on slice named " + s.slice.hrn,
				CommunicationUtil.listResourcesAm,
				true,
				true /*notFirst*/);
			ignoreReturnCode = true;
			op.timeout = 60;
			sliver = s;
			
			// Build up the args
			var options:Object = {geni_available:false, geni_compressed:true, geni_slice_urn:sliver.slice.urn.full};
			if(sliver.manager.rspecProcessor is ProtogeniRspecProcessor)
				options.rspec_version = {type:"ProtoGENI", version:sliver.manager.outputRspecVersion};
					
			op.pushField([sliver.slice.credential]);
			op.pushField(options);
			op.setExactUrl(sliver.manager.Url);
		}
		
		override public function start():Operation {
			if(sliver.manager.Status == GeniManager.STATUS_VALID)
				return op;
			else
				return null;
		}
		
		override public function complete(code:Number, response:Object):*
		{
			try
			{
				var decodor:Base64Decoder = new Base64Decoder();
				decodor.decode(response as String);
				var bytes:ByteArray = decodor.toByteArray();
				bytes.uncompress();
				var decodedRspec:String = bytes.toString();
				sliver.rspec = new XML(decodedRspec);
				
				sliver.created = true;
				sliver.parseRspec();
				
				if(sliver.nodes.length > 0 && !sliver.slice.slivers.contains(sliver)) {
					sliver.slice.slivers.add(sliver);
					return new RequestSliverStatusAm(sliver);
				}
			} catch(e:Error)
			{
			}
			
			return null;
		}
		
		override public function cleanup():void {
			super.cleanup();
			Main.geniDispatcher.dispatchSliceChanged(sliver.slice, GeniEvent.ACTION_POPULATING);
		}
	}
}
