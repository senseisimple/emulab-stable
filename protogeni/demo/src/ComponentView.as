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
  import flash.events.MouseEvent;
  import flash.net.navigateToURL;
  import flash.net.URLRequest;

  class ComponentView
  {
    static var gaAd = '<rspec type="advertisement" ' +
'xmlns="http://www.protogeni.net/resources/rspec/0.1"> ' +
'  <node component_uuid="gtnoise.net+node+atlanta_mux" ' +
'        component_name="atlanta_mux" ' +
'        component_manager_uuid="gtnoise.net" ' +
'        virtualization_type="bgpmux"> ' +
'    <node_type type_name="bgpmux" type_slots="1"> ' +
'      <field key="upstream_as" value="2637" /> ' +
'    </node_type> ' +
'    <available>true</available> ' +
'    <exclusive>false</exclusive> ' +
'    <interface component_id="physical_interface"/> ' +
'  </node> ' +
'  <node component_uuid="gtnoise.net+node+madison_mux" ' +
'        component_name="madison_mux" ' +
'        component_manager_uuid="gtnoise.net" ' +
'        virtualization_type="bgpmux"> ' +
'    <node_type type_name="bgpmux" type_slots="1"> ' +
'      <field key="upstream_as" value="2831" /> ' +
'    </node_type> ' +
'    <available>true</available> ' +
'    <exclusive>false</exclusive> ' +
'    <interface component_id="physical_interface"/> ' +
'  </node> ' +
'  <bgp_prefix address="168.62.16.0" netmask="21" /> ' +
'</rspec> ';

    public static var georgia
      = new ComponentManager("f00",
                             "Georgia Tech",
                             ".ga.edu",
                             "http://www.ga.edu/protogeni/xmlrpc",
                             null, 3);

    public function ComponentView(newSelect : ComboBox,
                                  newList : List,
                                  newNodes : ActiveNodes) : void
    {
      select = newSelect;
      list = newList;
      list.allowMultipleSelection = true;
      list.setStyle("cellRenderer", CustomCellRenderer);

      listStatus = new ListStatusClip();
      listStatus.text.wordWrap = true;
      listStatus.fixIt.visible = false;
      listStatus.fixItText.visible = false;
      listStatus.fixItText.mouseEnabled = false;
      list.addChild(listStatus);
//      listStatus.alpha = 0.3;
      nodes = newNodes;

      buttons = new ButtonList([listStatus.fixIt], [clickFixIt]);

/*
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
//                             "https://boss.emulab.net/protogeni/stoller/xmlrpc",
                             updateList, 2),


        new ComponentManager("be300821-ecb7-11dd-a0f8-001143e43ff3",
                             "gtwelab",
                             ".emulab.net",
                             "https://myboss.emulab.geni.emulab.net:443/protogeni/xmlrpc",
                             updateList, 2),

        new ComponentManager("27cd73fb-b908-11de-837a-0002b33f8548",
                             "jonlab",
                             ".emulab.net",
                             "https://myboss.jonlab.geni.emulab.net:443/protogeni/xmlrpc",
                             updateList, 2),
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
        new ComponentManager("urn:publicid:IDN+uml.emulab.net+authority+cm",
                             "umlGENI",
                             ".uml.emulab.net",
                             "https://boss.uml.emulab.net/protogeni/xmlrpc",
                             updateList, 2),
        georgia);
*/
      managers = new Array();

      select.removeAll();
      select.selectedIndex = 0;

      addManager(new ComponentManager("", "", "", "", updateList, 2));
      addManager(georgia);
/*
      var i : int = 0;
      for (; i < managers.length; ++i)
      {
        select.addItem(new ListItem(managers[i].getName(), managers[i]));
      }
*/
      managers[0].setState(ComponentManager.NORMAL);

      list.addEventListener(ListEvent.ITEM_CLICK, clickItem);
      list.addEventListener(Event.CHANGE, changeItem);
      select.addEventListener(Event.CHANGE, changeComponent);

      updateList();
    }

    public function addManager(newManager : ComponentManager) : void
    {
      newManager.setUpdate(updateList);
      managers.push(newManager);
      select.addItem(new ListItem(newManager.getName(), ""));
      updateList();
    }

    public function cleanup() : void
    {
      buttons.cleanup();
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
        var superNode = null;
        var superNodeName = null;
        if (component.superNode != -1 && ! cm.isUsed(component.superNode))
        {
          superNode = cm.getComponent(component.superNode);
          superNodeName = superNode.name;
        }
        nodes.addNode(component, cm, event.index,
                      select.stage.mouseX, select.stage.mouseY, true,
                      superNodeName);
        cm.addUsed(event.index);
        if (superNode != null)
        {
          nodes.addNode(superNode, cm, component.superNode,
                        400, 200, false, null);
          cm.addUsed(component.superNode);
        }
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
      var state = cm.getState();
      list.removeAll();
      list.clearSelection();
      var i : int = 0;
      for (; i < cm.getComponentCount(); ++i)
      {
        list.addItem(new ListItem(cm.getComponent(i).name, "NodeNone"));
      }
      list.selectedIndices = cm.getUsed();
      if (state == ComponentManager.NORMAL)
      {
        listStatus.visible = false;
      }
      else if (state == ComponentManager.LOADING)
      {
        listStatus.visible = true;
        listStatus.text.text = "Loading";
        listStatus.text.backgroundColor = 0x55ff55;
      }
      else
      {
        listStatus.visible = true;
        var errorText = "IO Failure";
        if (state == ComponentManager.SECURITY_FAILURE)
        {
          errorText = "Security Failure\n\n"
            + "Click fix it below, add a security exception, then reload "
            + "client.";
        }
        else if (state == ComponentManager.INTERNAL_FAILURE)
        {
          errorText = "Internal Failure";
        }
        listStatus.text.text = errorText;
        listStatus.text.backgroundColor = 0xff5555;
        if (state == ComponentManager.SECURITY_FAILURE)
        {
          listStatus.fixIt.visible = true;
          listStatus.fixItText.visible = true;
        }
        else
        {
          listStatus.fixIt.visible = false;
          listStatus.fixItText.visible = false;
        }
      }
    }

    function clickFixIt(event : MouseEvent) : void
    {
      Main.getConsole().appendText("Clicked\n\n\n");
      var cm = managers[select.selectedIndex];
      var urlText = cm.getUrl();
      var url = new URLRequest(urlText);
      navigateToURL(url, "_blank");
    }

    var select : ComboBox;
    var list : List;
    var listStatus : ListStatusClip;
    var nodes : ActiveNodes;
    // The ComponentManagers
    var managers : Array;
    var buttons : ButtonList;
  }
}
