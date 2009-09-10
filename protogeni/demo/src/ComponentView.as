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
      list.setStyle("cellRenderer", CustomCellRenderer);

      listStatus = new ListStatusClip();
      list.addChild(listStatus);
      listStatus.alpha = 0.3;
      nodes = newNodes;

      managers = new Array(
        new ComponentManager("", "", "", "", updateList, 2),
        new ComponentManager("e2a9e480-aa9f-11dd-9fcd-001143e43770",
                             "ProtoGENI",
                             ".emulab.net",
                             "https://myboss.myelab.testbed.emulab.net:443/protogeni/xmlrpc",
                             updateList, 2),
        new ComponentManager("28a10955-aa00-11dd-ad1f-001143e453fe",
                             "Emulab",
                             ".emulab.net",
                             "https://boss.emulab.net:443/protogeni/xmlrpc",
// Doesn't work ???                             "https://boss.emulab.net:443/dev/stoller/protogeni/xmlrpc",
                             updateList, 2),
/*
        new ComponentManager("be300821-ecb7-11dd-a0f8-001143e43ff3",
                             "gtwelab",
                             ".emulab.net",
                             "https://myboss.emulab.geni.emulab.net:443/protogeni/xmlrpc",
                             updateList, 2),
*/
        new ComponentManager("b83b47be-e7f0-11dd-848b-0013468d3dc8",
                             "Kentucky",
                             ".uky.emulab.net",
                             "https://www.uky.emulab.net/protogeni/xmlrpc",
                             updateList, 2),
        new ComponentManager("f38e8571-f7af-11dd-ab88-00304868a4be",
                             "Wisconsin",
                             ".schooner.wail.wisc.edu",
                             "https://www.schooner.wail.wisc.edu/protogeni/xmlrpc",
                             updateList, 0),
        new ComponentManager("b14b4a9d-8e53-11de-be30-001ec9540a39", "CMU", ".cmcl.cs.cmu.edu",
                             "https://boss.cmcl.cs.cmu.edu/protogeni/xmlrpc",
                             updateList, 2),
        new ComponentManager("a4539e9b-876a-11de-af17-00a0c9983803",
                             "umlGENI",
                             ".uml.emulab.net",
                             "https://boss.uml.emulab.net/protogeni/xmlrpc",
                             updateList, 2));
      select.removeAll();
      select.selectedIndex = 0;

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

    public function getCurrentManager() : ComponentManager
    {
      return managers[select.selectedIndex];
    }

    public function isValidManager() : Boolean
    {
      return select.selectedIndex != 0;
    }

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
      list.scrollToIndex(0);
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

    var select : ComboBox;
    var list : List;
    var listStatus : ListStatusClip;
    var nodes : ActiveNodes;
    // The ComponentManagers
    var managers : Array;
  }
}
