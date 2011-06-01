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

package
{
	import com.hurlant.util.Base64;
	
	import flash.events.NetStatusEvent;
	import flash.net.SharedObject;
	import flash.net.SharedObjectFlushStatus;
	import flash.utils.ByteArray;
	
	import mx.controls.Alert;
	import mx.events.CloseEvent;
	
	import protogeni.GeniEvent;
	import protogeni.GeniHandler;
	import protogeni.NetUtil;
	import protogeni.resources.GeniManager;
	import protogeni.resources.IdnUrn;
	import protogeni.resources.PhysicalNode;
	import protogeni.resources.PhysicalNodeGroup;
	import protogeni.resources.PlanetlabAggregateManager;
	import protogeni.resources.PlanetlabRspecProcessor;
	import protogeni.resources.ProtogeniComponentManager;
	import protogeni.resources.Site;
	import protogeni.resources.Slice;
	import protogeni.resources.SliceAuthority;
	import protogeni.resources.Sliver;
	
	/**
	 * Manages data cached to file.  The basic cache uses common
	 * settings used throughout flack.  The offline cache is
	 * used to restore GENI data without making calls.
	 * 
	 * @author mstrum
	 * 
	 */
	public class FlackCache
	{
		private static var _instance:FlackCache = new FlackCache();
		public static function get instance():FlackCache { return _instance; } 
		
		private static var _basicSharedObject:SharedObject = null;
		public static function get basicAvailable():Boolean
		{
			return _basicSharedObject != null
				&& _basicSharedObject.size > 0;
		}
		private static var _offlineSharedObject:SharedObject = null;
		public static function get offlineAvailable():Boolean
		{
			return _offlineSharedObject != null
				&& _offlineSharedObject.size > 0;
		}
		
		public static var userAuthorityUrn:String = "";
		[Bindable]
		public static var userSslPem:String = "";
		public static var userPassword:String = "";
		public static var maxParallelRequests:int = 3;
		public static var geniBundle:String = "";
		public static var rootBundle:String = "";
		
		function FlackCache() {
			Main.geniDispatcher.addEventListener(GeniEvent.GENIMANAGER_CHANGED, updateManager);
			Main.geniDispatcher.addEventListener(GeniEvent.USER_CHANGED, updateUser);
			Main.geniDispatcher.addEventListener(GeniEvent.SLICE_CHANGED, updateSlice);
		}
		
		public function updateManager(event:GeniEvent):void {
			if(!Main.offlineMode && event.action == GeniEvent.ACTION_POPULATED) {
				setManagerOffline(event.changedObject as GeniManager);
			}
		}
		
		public static function clearManagersOffline():void {
			if(_offlineSharedObject == null)
				loadOfflineSharedObject();
			if(_offlineSharedObject == null)
				return;
			
			_offlineSharedObject.data.managers = [];
		}
		
		public static function setManagerOffline(manager:GeniManager):void {
			if(_offlineSharedObject == null)
				loadOfflineSharedObject();
			if(_offlineSharedObject == null)
				return;
			
			if(_offlineSharedObject.data.managers == null) {
				_offlineSharedObject.data.managers = [];
			} else {
				for(var i:int = 0; i < _offlineSharedObject.data.managers.length; i++)
					if(_offlineSharedObject.data.managers[i].urn == manager.Urn.full) {
						_offlineSharedObject.data.managers.splice(i, 1);
						break;
					}
			}
			
			var newManagerObject:Object = new Object();
			if(manager.type == GeniManager.TYPE_PROTOGENI)
				newManagerObject.inputRspecVersion = (manager as ProtogeniComponentManager).inputRspecVersion;
			else if(manager.type == GeniManager.TYPE_PLANETLAB) {
				newManagerObject.sites = [];
				for each(var site:Site in (manager as PlanetlabAggregateManager).sites) {
					var siteObject:Object = new Object();
					siteObject.hrn = site.hrn;
					siteObject.latitude = site.latitude;
					siteObject.longitude = site.longitude;
					newManagerObject.sites.push(siteObject);
				}
			}
			newManagerObject.colorIdx = manager.colorIdx;
			newManagerObject.Hrn = manager.Hrn;
			
			var encodedManagerRspec:ByteArray = new ByteArray();
			encodedManagerRspec.writeUTFBytes(manager.Rspec.toXMLString());
			encodedManagerRspec.compress();
			newManagerObject.Rspec = encodedManagerRspec;
			
			newManagerObject.Show = manager.Show;
			newManagerObject.Status = manager.Status;
			newManagerObject.supportsGpeni = manager.supportsGpeni;
			newManagerObject.supportsIon = manager.supportsIon;
			newManagerObject.type = manager.type;
			newManagerObject.Url = manager.Url;
			newManagerObject.urn = manager.Urn.full;
			newManagerObject.Version = manager.Version;
			_offlineSharedObject.data.managers.push(newManagerObject);
		}
		
		public function updateUser(event:GeniEvent):void {
			setUserOffline();
		}
		
		public static function setUserOffline():void {
			if(_offlineSharedObject == null)
				loadOfflineSharedObject();
			if(_offlineSharedObject == null)
				return;
			
			_offlineSharedObject.data.credential = Main.geniHandler.CurrentUser.credential;
			_offlineSharedObject.data.hrn = Main.geniHandler.CurrentUser.hrn;
			_offlineSharedObject.data.keys = Main.geniHandler.CurrentUser.keys;
			_offlineSharedObject.data.name = Main.geniHandler.CurrentUser.name;
			_offlineSharedObject.data.uid = Main.geniHandler.CurrentUser.uid;
			_offlineSharedObject.data.urn = Main.geniHandler.CurrentUser.urn.full;
		}
		
		public static function updateSlice(event:GeniEvent):void {
			setSliceOffline(event.changedObject as Slice);
		}
		
		public static function setSliceOffline(slice:Slice):void {
			if(_offlineSharedObject == null)
				loadOfflineSharedObject();
			if(_offlineSharedObject == null)
				return;
			
			if(_offlineSharedObject.data.slices == null) {
				_offlineSharedObject.data.slices = [];
			} else {
				for(var i:int = 0; i < _offlineSharedObject.data.slices.length; i++)
					if(_offlineSharedObject.data.slices[i].urn == slice.urn.full) {
						_offlineSharedObject.data.slices.splice(i, 1);
						break;
					}
			}
			
			var newSliceObject:Object = new Object();
			newSliceObject.credential = slice.credential;
			newSliceObject.hrn = slice.hrn;
			newSliceObject.urn = slice.urn.full;
			
			newSliceObject.slivers = [];
			if(slice.isCreated()) {
				for each(var sliver:Sliver in slice.slivers.collection) {
					var newSliverObject:Object = new Object();
					newSliverObject.managerUrn = sliver.manager.Urn.full;
					newSliverObject.credential = sliver.credential;
					newSliverObject.state = sliver.state;
					newSliverObject.status = sliver.status;
					newSliverObject.urn = sliver.urn.full;
					var encodedSliverRspec:ByteArray = new ByteArray();
					encodedSliverRspec.writeUTFBytes(sliver.rspec.toXMLString());
					encodedSliverRspec.compress();
					newSliverObject.rspec = encodedSliverRspec;
					newSliceObject.slivers.push(newSliverObject);
				}
			}
			
			_offlineSharedObject.data.slices.push(newSliceObject);
		}
		
		/**
		 * Loads the basic cache from file 
		 * 
		 */		
		public static function loadBasicSharedObject():void {
			try {
				_basicSharedObject = SharedObject.getLocal("flackCacheSharedObject");
				if(_basicSharedObject.size > 0) {
					if(_basicSharedObject.data.userAuthorityUrn != null)
						userAuthorityUrn = _basicSharedObject.data.userAuthorityUrn;
					if(_basicSharedObject.data.userSslPem != null)
						userSslPem = _basicSharedObject.data.userSslPem;
					if(_basicSharedObject.data.userPassword != null)
						userPassword = _basicSharedObject.data.userPassword;
					if(_basicSharedObject.data.maxParallelRequests != null)
						maxParallelRequests = _basicSharedObject.data.maxParallelRequests;
					if(_basicSharedObject.data.geniBundle != null)
						geniBundle = _basicSharedObject.data.geniBundle;
					if(_basicSharedObject.data.rootBundle != null)
						rootBundle = _basicSharedObject.data.rootBundle;
				}
			} catch(e:Error) {
				Alert.show("It looks like Flash is unable to save/load local data." +
					"  Would you like to load directions on how to enable local data?",
					"Enable Local Data?", Alert.YES|Alert.NO, Main.Application(),
					function allowData(e:CloseEvent):void {
						if(e.detail == Alert.YES)
							NetUtil.openWebsite("http://kb2.adobe.com/cps/408/kb408202.html");
					});
			}
			
			/*
			if(geniBundle.length == 0)
			geniBundle = (new FallbackGeniBundle()).toString();
			if(rootBundle.length == 0)
			rootBundle =  (new FallbackRootBundle()).toString();
			*/
			
			// keep data from the old caching method
			try {
				var tempSharedObject:SharedObject = SharedObject.getLocal("geniLocalSharedObject");
				if (tempSharedObject != null && tempSharedObject.size > 0)
				{
					for each(var sa:SliceAuthority in Main.geniHandler.GeniAuthorities.source) {
						if(sa.Url == tempSharedObject.data.authority) {
							userAuthorityUrn = sa.Urn;
							break;
						}
					}
					
					userSslPem = tempSharedObject.data.sslPem;
					
					if(tempSharedObject.data.password != null && tempSharedObject.data.password.length > 0)
						userPassword = tempSharedObject.data.password;
					
					tempSharedObject.clear();
				}
				
			} catch(e:Error) {}
		}
		
		/**
		 * Loads the offline shared object without applying anything 
		 * 
		 */		
		public static function loadOfflineSharedObject():void {
			try {
				_offlineSharedObject = SharedObject.getLocal("offlineCacheSharedObject");
			}
			catch(e:Error) { }
		}
		
		/**
		 * Applies the cached data to Flack 
		 * 
		 */		
		public static function applyBasic():void {
			if (_basicSharedObject != null && _basicSharedObject.size > 0)
			{
				for each(var sa:SliceAuthority in Main.geniHandler.GeniAuthorities.source) {
					if(sa.Urn == userAuthorityUrn) {
						Main.geniHandler.CurrentUser.authority = sa;
						break;
					}
				}
				
				if(userPassword.length > 0)
					Main.geniHandler.CurrentUser.setPassword(userPassword, true);
				
				Main.geniHandler.requestHandler.maxRunning = maxParallelRequests;
			}
		}
		
		/**
		 * Saves the basic cache to file
		 * 
		 */		
		public static function saveBasic():void {
			if(_basicSharedObject == null)
				return;

			var flushStatus:String = null;
			try {
				_basicSharedObject.data.userAuthorityUrn = userAuthorityUrn;
				_basicSharedObject.data.userSslPem = userSslPem;
				_basicSharedObject.data.userPassword = userPassword;
				_basicSharedObject.data.maxParallelRequests = maxParallelRequests;
				_basicSharedObject.data.geniBundle = geniBundle;
				_basicSharedObject.data.rootBundle = rootBundle;
				flushStatus = _basicSharedObject.flush();
			} catch (e:Error) {
				if(Main.debugMode)
					trace("Problem saving shared object");
			}
			
			// deal with dialog to increase cache size
			if (flushStatus != null) {
				switch (flushStatus) {
					case SharedObjectFlushStatus.PENDING:
						_basicSharedObject.addEventListener(NetStatusEvent.NET_STATUS, onFlushStatus);
						break;
					case SharedObjectFlushStatus.FLUSHED:
						// saved
						break;
				}
			}
		}
		
		/**
		 * Called after user closes dialog
		 * 
		 * @param event
		 * 
		 */		
		private static function onFlushStatus(event:NetStatusEvent):void {
			if(event.info.code == "SharedObject.Flush.Success") {
				// saved
			}
			_basicSharedObject.removeEventListener(NetStatusEvent.NET_STATUS, onFlushStatus);
		}	
		private static function onOfflineFlushStatus(event:NetStatusEvent):void {
			if(event.info.code == "SharedObject.Flush.Success") {
				// saved
			}
			_offlineSharedObject.removeEventListener(NetStatusEvent.NET_STATUS, onFlushStatus);
		}
		
		/**
		 * Clears and removes the cache from file 
		 * 
		 */
		public static function clearAll():void {
			if(_basicSharedObject != null) {
				_basicSharedObject.clear();
				if(!offlineAvailable)
					loadOfflineSharedObject();
			}
			if(_offlineSharedObject != null)
				_offlineSharedObject.clear();
		}
		
		public static function clearOffline():void {
			if(_offlineSharedObject == null)
				loadOfflineSharedObject();
			if(_offlineSharedObject != null)
				_offlineSharedObject.clear();
		}
		
		public static function saveOffline():void {
			if(_offlineSharedObject == null)
				loadOfflineSharedObject();
			if(_offlineSharedObject == null)
				return;
			
			var flushStatus:String = null;
			try {
				// save user info
				setUserOffline();
				
				// save manager info
				_offlineSharedObject.data.managers = [];
				for each(var manager:GeniManager in Main.geniHandler.GeniManagers) {
					if(manager.Status == GeniManager.STATUS_VALID)
						setManagerOffline(manager);
				}
				
				// Save slices
				_offlineSharedObject.data.slices = [];
				for each(var slice:Slice in Main.geniHandler.CurrentUser.slices) {
					setSliceOffline(slice);
				}
				flushStatus = _offlineSharedObject.flush();
			} catch (e:Error) {
				if(Main.debugMode)
					trace("Problem saving shared object");
			}
			
			// deal with dialog to increase cache size
			if (flushStatus != null) {
				switch (flushStatus) {
					case SharedObjectFlushStatus.PENDING:
						_offlineSharedObject.addEventListener(NetStatusEvent.NET_STATUS, onOfflineFlushStatus);
						break;
					case SharedObjectFlushStatus.FLUSHED:
						// saved
						break;
				}
				
			}
		}
		
		public static function applyOffline():void {
			if(offlineAvailable) {
				try {
					// get user info
					Main.geniHandler.CurrentUser.credential = _offlineSharedObject.data.credential;
					Main.geniHandler.CurrentUser.hrn = _offlineSharedObject.data.hrn;
					Main.geniHandler.CurrentUser.keys = _offlineSharedObject.data.keys;
					Main.geniHandler.CurrentUser.name = _offlineSharedObject.data.name;
					Main.geniHandler.CurrentUser.uid = _offlineSharedObject.data.uid;
					Main.geniHandler.CurrentUser.urn = new IdnUrn(_offlineSharedObject.data.urn);
					
					Main.geniDispatcher.dispatchUserChanged();
					
					for each(var initialManagerObject:Object in _offlineSharedObject.data.managers) {
						var initialManager:GeniManager;
						if(initialManagerObject.type == GeniManager.TYPE_PROTOGENI) {
							initialManager = new ProtogeniComponentManager();
							(initialManager as ProtogeniComponentManager).inputRspecVersion = initialManagerObject.inputRspecVersion;
						} else if(initialManagerObject.type == GeniManager.TYPE_PLANETLAB) {
							initialManager = new PlanetlabAggregateManager();
						}
						
						initialManager.colorIdx = initialManagerObject.colorIdx;
						initialManager.Hrn = initialManagerObject.Hrn;
						
						var compressedRspec:ByteArray = initialManagerObject.Rspec;
						compressedRspec.uncompress();
						initialManager.Rspec = new XML(compressedRspec.toString());
						compressedRspec.compress();
						
						initialManager.Show = initialManagerObject.Show;
						initialManager.Status = initialManagerObject.Status;
						initialManager.supportsGpeni = initialManagerObject.supportsGpeni;
						initialManager.supportsIon = initialManagerObject.supportsIon;
						initialManager.type = initialManagerObject.type;
						initialManager.Url = initialManagerObject.Url;
						initialManager.Urn = new IdnUrn(initialManagerObject.urn);
						initialManager.Version = initialManagerObject.Version;
						initialManager.Status = GeniManager.STATUS_INPROGRESS;
						
						Main.geniHandler.GeniManagers.addItem(initialManager);
						
						Main.geniDispatcher.dispatchGeniManagerChanged(initialManager);
					}
					
					// get manager info
					for each(var managerObject:Object in _offlineSharedObject.data.managers) {
						var newManager:GeniManager = Main.geniHandler.GeniManagers.getByUrn(managerObject.urn);
						
						if(newManager.type == GeniManager.TYPE_PLANETLAB) {
							(newManager.rspecProcessor as PlanetlabRspecProcessor).callGetSites = false;
							newManager.data = managerObject.sites;
							newManager.rspecProcessor.processResourceRspec(
								function parseSites(finishedManager:PlanetlabAggregateManager):void {
									for each(var siteObject:Object in finishedManager.data) {
										var site:Site = finishedManager.getSiteWithHrn(siteObject.hrn);
										if(site != null) {
											site.latitude = siteObject.latitude;
											site.longitude = siteObject.longitude;
											
											var ng:PhysicalNodeGroup = finishedManager.Nodes.GetByLocation(site.latitude,site.longitude);
											var tempString:String;
											if(ng == null) {
												ng = new PhysicalNodeGroup(site.latitude, site.longitude, "", finishedManager.Nodes);
												finishedManager.Nodes.Add(ng);
											}
											for each(var n:PhysicalNode in site.nodes) {
												ng.Add(n);
											}
										}
									}
									Main.geniDispatcher.dispatchGeniManagerChanged(finishedManager, GeniEvent.ACTION_POPULATED);
									tryApplyOfflineSlices(finishedManager);
								});
						} else
							newManager.rspecProcessor.processResourceRspec(tryApplyOfflineSlices);
					}
					
					Main.geniDispatcher.dispatchGeniManagersChanged();
				} catch (e:Error) {
					Alert.show(e.toString(), "Error applying offline data");
				}
			}
		}
		
		public static function tryApplyOfflineSlices(manager:GeniManager):void {
			for each(var manager:GeniManager in Main.geniHandler.GeniManagers) {
				if(manager.Status == GeniManager.STATUS_INPROGRESS)
					return;
			}
			for each(var sliceObject:Object in _offlineSharedObject.data.slices) {
				var newSlice:Slice = new Slice();
				newSlice.creator = Main.geniHandler.CurrentUser;
				newSlice.credential = sliceObject.credential;
				newSlice.hrn = sliceObject.hrn;
				newSlice.urn = new IdnUrn(sliceObject.urn);
				
				for each(var sliverObject:Object in sliceObject.slivers) {
					var newSliver:Sliver = newSlice.getOrCreateSliverFor(Main.geniHandler.GeniManagers.getByUrn(sliverObject.managerUrn));
					newSliver.credential = sliverObject.credential;
					newSliver.urn = new IdnUrn(sliverObject.urn);
					newSliver.created = true;
					newSliver.state = sliverObject.state;
					newSliver.status = sliverObject.status;
					
					var compressedRspec:ByteArray = sliverObject.rspec;
					compressedRspec.uncompress();
					newSliver.rspec = new XML(compressedRspec.toString());
					compressedRspec.compress();
					
					newSliver.parseRspec();
				}
				
				Main.geniHandler.CurrentUser.slices.add(newSlice);
			}
			Main.geniDispatcher.dispatchUserChanged();
			Main.geniDispatcher.dispatchSlicesChanged();
		}
	}
}