package
{
	import flash.events.NetStatusEvent;
	import flash.net.SharedObject;
	import flash.net.SharedObjectFlushStatus;
	
	import mx.controls.Alert;
	import mx.events.CloseEvent;
	
	import protogeni.NetUtil;
	import protogeni.resources.SliceAuthority;

	public class FlackCache extends Object
	{
		// ensure one instance and not written to cache
		[Transient]
		private static var _instance:FlackCache = new FlackCache();
		[Transient]
		public static function get instance():FlackCache { return _instance; }
		[Transient]
		private static var _sharedObject:SharedObject = null;
		
		public var userAuthorityUrn:String = "";
		[Bindable]
		public var userSslPem:String = "";
		public var userPassword:String = "";
		public var maxParallelRequests:int = 3;
		public var skipInitialWindow:Boolean = false;
		public var geniBundle:String = "";
		public var rootBundle:String = "";
		
		public function FlackCache()
		{
			super();
		}
		
		public function loadSharedObject():void {
			try {
				_sharedObject = SharedObject.getLocal("flackCacheSharedObject");
				_instance = _sharedObject.data.flackCache;
			} catch(typeError:TypeError) {
				// This happens if the properties change, save any which we think were there before
				var o:Object = _sharedObject.data.flackCache;
				
				if(o.userAuthorityUrn != null)
					this.userAuthorityUrn = o.userAuthorityUrn;
				if(o.userSslPem != null)
					this.userSslPem = o.userSslPem;
				if(o.userPassword != null)
					this.userPassword = o.userPassword;
				if(o.maxParallelRequests != null)
					this.maxParallelRequests = o.maxParallelRequests;
				if(o.skipInitialWindow != null)
					this.skipInitialWindow = o.skipInitialWindow;
				if(o.geniBundle != null)
					this.geniBundle = o.geniBundle;
				if(o.rootBundle != null)
					this.rootBundle = o.rootBundle;
				
			} catch(e:Error) {
				Alert.show("It looks like Flash is unable to save/load local data." +
					"  Would you like to load directions on how to enable local data?",
					"Enable Local Data?", Alert.YES|Alert.NO, Main.Application(),
					function allowData(e:CloseEvent):void {
						if(e.detail == Alert.YES)
							NetUtil.openWebsite("http://kb2.adobe.com/cps/408/kb408202.html");
					});
			}
			
			if(this.geniBundle.length == 0)
				this.geniBundle = (new FallbackGeniBundle()).toString();
			if(this.rootBundle.length == 0)
				this.rootBundle =  (new FallbackRootBundle()).toString();
			
			// keep data from the old caching method
			try {
				var tempSharedObject:SharedObject = SharedObject.getLocal("geniLocalSharedObject");
				if (tempSharedObject != null && tempSharedObject.size > 0)
				{
					for each(var sa:SliceAuthority in Main.geniHandler.GeniAuthorities.source) {
						if(sa.Url == tempSharedObject.data.authority) {
							this.userAuthorityUrn = sa.Urn;
							break;
						}
					}
					
					this.userSslPem = tempSharedObject.data.sslPem;
					
					if(tempSharedObject.data.password != null && tempSharedObject.data.password.length > 0)
						this.userPassword = tempSharedObject.data.password;
					
					tempSharedObject.clear();
				}
				
			} catch(e:Error) {}
		}
		
		public function applyData():void {
			if (_sharedObject != null && _sharedObject.size > 0)
			{
				for each(var sa:SliceAuthority in Main.geniHandler.GeniAuthorities.source) {
					if(sa.Urn == this.userAuthorityUrn) {
						Main.geniHandler.CurrentUser.authority = sa;
						break;
					}
				}
				
				if(this.userPassword.length > 0)
					Main.geniHandler.CurrentUser.setPassword(this.userPassword, true);
				
				Main.geniHandler.requestHandler.maxRunning = this.maxParallelRequests;
			}
		}
		
		public function save():void {
			if(_sharedObject == null)
				return;
			
			var flushStatus:String = null;
			try {
				_sharedObject.data.flackCache = this;
				flushStatus = _sharedObject.flush();
			} catch (e:Error) {
				if(Main.debugMode)
					trace("Problem saving shared object");
			}

			// deal with dialog to increase cache size
			if (flushStatus != null && flushStatus == SharedObjectFlushStatus.PENDING) {
				switch (flushStatus) {
					case SharedObjectFlushStatus.PENDING:
						_sharedObject.addEventListener(NetStatusEvent.NET_STATUS, onFlushStatus);
						break;
					case SharedObjectFlushStatus.FLUSHED:
						// saved
						break;
				}
				
			}
		}
		
		private function onFlushStatus(event:NetStatusEvent):void {
			if(event.info.code == "SharedObject.Flush.Success") {
				// saved
			}
			_sharedObject.removeEventListener(NetStatusEvent.NET_STATUS, onFlushStatus);
		}
		
		public function clear():void {
			_sharedObject.clear();
		}
	}
}