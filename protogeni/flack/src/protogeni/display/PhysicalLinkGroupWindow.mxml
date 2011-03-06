<?xml version="1.0" encoding="utf-8"?>

<!--

GENIPUBLIC-COPYRIGHT
Copyright (c) 2008-2011 University of Utah and the Flux Group.
All rights reserved.

Permission to use, copy, modify and distribute this software is hereby
granted provided that (1) source code retains these copyright, permission,
and disclaimer notices, and (2) redistributions including binaries
reproduce the notices in supporting documentation.

THE UNIVERSITY OF UTAH ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
CONDITION.  THE UNIVERSITY OF UTAH DISCLAIMS ANY LIABILITY OF ANY KIND
FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.

-->

<components:DefaultWindow xmlns:fx="http://ns.adobe.com/mxml/2009" 
						xmlns:s="library://ns.adobe.com/flex/spark" 
						xmlns:mx="library://ns.adobe.com/flex/mx" xmlns:display="protogeni.display.*"
						title="Link Group Information"
						close="closeWindow()" xmlns:components="protogeni.display.components.*">
	<fx:Script>
		<![CDATA[
			import mx.collections.ArrayCollection;
			
			import protogeni.resources.PhysicalLink;
			import protogeni.resources.PhysicalLinkGroup;
			
			[Bindable]
			public var links:ArrayCollection;
			
			public function loadCollection(group:ArrayCollection):void {
				links = group;
				
				if(links.length > 1) {
					listLinks.selectedIndex = 0;
				} else {
					listLinks.visible = false;
					listLinks.includeInLayout = false;
					linkInfo.percentWidth = 100;
					title = "Link Information";
				}
				
				linkInfo.load(links[0]);
			}
			
			public function loadGroup(group:PhysicalLinkGroup):void {
				links = new ArrayCollection();
				for each(var link:PhysicalLink in group.collection)
					links.addItem(link);
				loadCollection(links);
			}
		]]>
	</fx:Script>
	<mx:HDividedBox width="100%" height="100%">
		<s:List width="35%" height="100%" id="listLinks"
				 dataProvider="{links}"
				 labelField="name" change="linkInfo.load(event.target.selectedItem)" />
		<display:PhysicalLinkInfo  height="100%" width="65%" id="linkInfo"/>
	</mx:HDividedBox>
</components:DefaultWindow>
