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
	import com.mattism.http.xmlrpc.MethodFault;
	
	import flash.events.ErrorEvent;
	
	/**
	 * Skeleton structure and behavior of a request
	 * 
	 * @author mstrum
	 * 
	 */
	public class Request
	{
		public var op:Operation;
		[Bindable]
		public var name:String;
		[Bindable]
		public var details:String;
		public var startImmediately:Boolean;
		public var removeImmediately:Boolean;
		public var continueOnError:Boolean;
		public var running:Boolean = false;
		public var nonXmlrpc:Boolean = false;
		public var forceNext:Boolean = false;
		public var ignoreReturnCode:Boolean = false;
		public var numTries:int = 0;
		
		public var node:RequestQueueNode;
		
		public function Request(newName:String = "",
								newDetails:String = "",
								qualifiedMethod:Array = null,
								shouldContinueOnErrors:Boolean = false,
								shouldStartImmediately:Boolean = false,
								shouldRemoveImmediately:Boolean = true):void
		{
			op = new Operation(qualifiedMethod, this);
			op.timeout = 180;
			name = newName;
			details = newDetails;
			continueOnError = shouldContinueOnErrors;
			startImmediately = shouldStartImmediately;
			removeImmediately = shouldRemoveImmediately;
		}
		
		/**
		 * Called after everything has been done to do any leftover cleanup
		 * 
		 */
		public function cleanup():void
		{
			op.cleanup();
		}
		
		/**
		 * Cancels the request
		 * 
		 */
		public function cancel():void
		{
			cleanup();
		}
		
		/**
		 * 
		 * @return Value sent
		 * 
		 */
		public function getSent():String
		{
			return op.getSent();
		}
		
		/**
		 * 
		 * @return Last response
		 * 
		 */
		public function getResponse():String
		{
			return op.getResponse();
		}
		
		/**
		 * 
		 * @return URL for the request
		 * 
		 */
		public function getUrl():String
		{
			return op.getUrl();
		}
		
		/**
		 * Called immediately before the operation is run to add variables it may not have had when added to the queue
		 * @return Operation to be run
		 * 
		 */
		public function start():Operation
		{
			return op;
		}
		
		/**
		 * Called when the request returns
		 * 
		 * @param code ProtoGENI response code
		 * @param response Value returned from the request
		 * @return Optionally another request or a queue of multiple requests
		 * 
		 */
		public function complete(code:Number, response:Object):*
		{
			return null;
		}
		
		/**
		 * Called if the request fails
		 * @param event
		 * @param fault
		 * @return Optionally another request or a queue of multiple requests
		 * 
		 */
		public function fail(event:ErrorEvent, fault:MethodFault):*
		{
			return null;
		}
	}
}
