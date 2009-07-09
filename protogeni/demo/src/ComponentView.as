/* GENIPUBLIC-COPYRIGHT
 * Copyright (c) 2008, 2009 University of Utah and the Flux Group.
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
  // This object populates and takes events from both 'select', a
  // combo-box used to specify the component manager and 'list', a
  // listbox with all of the nodes managed by the component manager.
  import fl.controls.ComboBox;
  import fl.controls.List;
  import fl.events.ListEvent;
  import flash.events.Event;

  class ComponentView
  {
    public function ComponentView(newSelect : ComboBox,
                                  newList : List,
                                  newNodes : ActiveNodes) : void
    {
      select = newSelect;
      list = newList;
      list.allowMultipleSelection = true;
      listStatus = new ListStatusClip();
      list.addChild(listStatus);
      listStatus.alpha = 0.3;
      nodes = newNodes;

      managers = new Array(
        new ComponentManager("", "", "", "", updateList),
/*
        new ComponentManager("e2a9e480-aa9f-11dd-9fcd-001143e43770",
                             "ProtoGENI",
                             ".emulab.net",
                             "https://myboss.myelab.testbed.emulab.net:443/protogeni/xmlrpc",
                             updateList),
*/
        new ComponentManager("28a10955-aa00-11dd-ad1f-001143e453fe",
                             "Emulab",
                             ".emulab.net",
                             "https://boss.emulab.net:443/protogeni/xmlrpc/",
                             updateList)/*,
        new ComponentManager("be300821-ecb7-11dd-a0f8-001143e43ff3",
                             "gtwelab",
                             ".emulab.net",
                             "https://myboss.emulab.geni.emulab.net:443/protogeni/xmlrpc",
                             updateList),
        new ComponentManager("b83b47be-e7f0-11dd-848b-0013468d3dc8",
                             "Kentucky",
                             ".uky.emulab.net",
                             "https://www.uky.emulab.net/protogeni/xmlrpc",
                             updateList),
        new ComponentManager("f38e8571-f7af-11dd-ab88-00304868a4be",
                             "Wisconsin",
                             ".schooner.wail.wisc.edu",
                             "https://www.schooner.wail.wisc.edu/protogeni/xmlrpc",
                             updateList),
        new ComponentManager("", "CMU", ".cmcl.cs.cmu.edu",
                             "https://boss.cmcl.cs.cmu.edu/protogeni/xmlrpc",
                             updateList)*/);
      select.removeAll();
      select.selectedIndex = 0;
      select.rowCount = 4;

      var i : int = 0;
      for (; i < managers.length; ++i)
      {
        select.addItem(new ListItem(managers[i].getName(), managers[i]));
      }

      managers[0].setState(ComponentManager.NORMAL);

      list.addEventListener(ListEvent.ITEM_CLICK, clickItem);
      list.addEventListener(Event.CHANGE, changeItem);
      select.addEventListener(Event.CHANGE, changeComponent);

      updateList();
    }

    public function cleanup() : void
    {
      list.removeEventListener(ListEvent.ITEM_CLICK, clickItem);
      list.removeEventListener(Event.CHANGE, changeItem);
      select.removeEventListener(Event.CHANGE, changeComponent);
      listStatus.parent.removeChild(listStatus);
    }

    public function getManagers() : Array
    {
      return managers;
    }

/*
    public function getTicket(index : int) : String
    {
      return tickets[index];
    }

    public function setTicket(index : int, value : String) : void
    {
      tickets[index] = value;
    }

    public function getUrl(index : int) : String
    {
      return cmUrls[index];
    }
*/
/*
    public function getCmCount() : int
    {
      return managers.length;
    }
*/
/*
    public function removeNode(cm : ComponentManager, nodeIndex : int) : void
    {
      cm.removeUsed(nodeIndex);
      if (cm == select.selectedItem.data)
      {
        updateList();
      }
    }
*/
    function clickItem(event : ListEvent) : void
    {
      var cm : ComponentManager = managers[select.selectedIndex];
      if (! cm.isUsed(event.index))
      {
        var component = cm.getComponent(event.index);
        nodes.addNode(component, cm, event.index,
                      select.stage.mouseX, select.stage.mouseY);
        cm.addUsed(event.index);
      }
      list.selectedIndices = cm.getUsed();
    }

    function changeItem(event : Event) : void
    {
      list.selectedIndices = managers[select.selectedIndex].getUsed();
    }

    function changeComponent(event : Event) : void
    {
      updateList();
    }

    function updateList() : void
    {
      var cm = managers[select.selectedIndex];
      list.removeAll();
      list.clearSelection();
      var i : int = 0;
      for (; i < cm.getComponentCount(); ++i)
      {
        list.addItem(new ListItem(cm.getComponent(i).name, "NodeNone"));
      }
      list.selectedIndices = cm.getUsed();
      if (cm.getState() == ComponentManager.NORMAL)
      {
        listStatus.visible = false;
      }
      else if (cm.getState() == ComponentManager.LOADING)
      {
        listStatus.visible = true;
        listStatus.text.text = "Loading";
        listStatus.text.backgroundColor = 0x00ff00;
      }
      else
      {
        listStatus.visible = true;
        listStatus.text.text = "Failed";
        listStatus.text.backgroundColor = 0xff0000;
      }
    }
/*
    public function succeedResources(cm : ComponentManager, str : String)
    {
      cm.populateNodes(str);
      updateList();
    }

    public function failResources(cm : ComponentManager)
    {
      cm.setState(FAILED);
      updateList();
    }
*/
    var select : ComboBox;
    var list : List;
    var listStatus : ListStatusClip;
    var nodes : ActiveNodes;
    // The ComponentManagers
    var managers : Array;
/*
    // An array of arrays. Outer == CM, inner == Component
    var components : Array;

    var used : Array;
    var tickets : Array;
    var states : Array;
*/
/*
    public static var hostName : Array = new Array("",
    public static var cmNames : Array = new Array("", ,
    static var cmUrls : Array =
    static var cmUuids : Array =
    static var cmResults : Array =
*/
  }
}
