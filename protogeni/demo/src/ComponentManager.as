/* EMULAB-COPYRIGHT
 * Copyright (c) 2008 University of Utah and the Flux Group.
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

  class ComponentManager
  {
    public function ComponentManager(newSelect : ComboBox,
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

      names = new Array();
      uuids = new Array();
      used = new Array();
      tickets = new Array();
      states = new Array();

      select.removeAll();

      var i : int = 0;
      for (; i < cmNames.length; ++i)
      {
        select.addItem(new ListItem(cmNames[i], null));
        names.push(new Array());
        uuids.push(new Array());
        used.push(new Array());
        tickets.push(null);
        states.push(LOADING);
//        populateNodes(i, cmResults[i]);
      }

      select.selectedIndex = 0;

      states[0] = NORMAL;

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

    public function getCmCount() : int
    {
      return cmNames.length;
    }

    public function removeNode(cmIndex : int, nodeIndex : int) : void
    {
      var target : int = used[cmIndex].indexOf(nodeIndex);
      if (target != -1)
      {
        used[cmIndex].splice(target, 1);
      }
      if (cmIndex == select.selectedIndex)
      {
        list.selectedIndices = used[select.selectedIndex].slice();
      }
    }

    function clickItem(event : ListEvent) : void
    {
      if (used[select.selectedIndex].indexOf(event.index) == -1)
      {
        nodes.addNode(names[select.selectedIndex][event.index],
                      uuids[select.selectedIndex][event.index],
                      select.selectedIndex,
                      event.index,
                      removeNode,
                      select.stage.mouseX,
                      select.stage.mouseY);
        used[select.selectedIndex].push(event.index);
      }
      list.selectedIndices = used[select.selectedIndex].slice();
    }

    function changeItem(event : Event) : void
    {
      list.selectedIndices = used[select.selectedIndex].slice();
    }

    function changeComponent(event : Event) : void
    {
      updateList();
    }

    function updateList() : void
    {
      var current = select.selectedIndex;
      list.removeAll();
      list.clearSelection();
      var i : int = 0;
      for (; i < names[current].length; ++i)
      {
        list.addItem(new ListItem(names[current][i], "NodeNone"));
      }
      list.selectedIndices = used[current].slice();
      if (states[current] == NORMAL)
      {
        listStatus.visible = false;
      }
      else if (states[current] == LOADING)
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

    public function populateNodes(index : int, str : String) : void
    {
      if (str != null)
      {
        var rspec : XML = XML(str);
        var nodeName : QName = new QName(rspec.namespace(), "node");
        var xmlNodes : Array = new Array();
        for each (var element in rspec.elements(nodeName))
        {
          if (element.attribute("uuid") != null
              && element.attribute("uuid") != "")
          {
            xmlNodes.push(element);
          }
        }
        xmlNodes.sort(xmlSort);
        var i : int = 0;
        for (; i < xmlNodes.length; ++i)
        {
          names[index].push(xmlNodes[i].attribute("name"));
          uuids[index].push(xmlNodes[i].attribute("uuid"));
        }
      }
      states[index] = NORMAL;
      updateList();
    }

    function xmlSort(left : XML, right : XML) : int
    {
      // TODO: Make this an actual natural sort
      var leftAttribute = left.attribute("name");
      var rightAttribute = right.attribute("name");
      if (leftAttribute.substr(0, 2) == "pc"
          && rightAttribute.substr(0, 2) == "pc"
          && int(leftAttribute.substr(2)) > 0
          && int(rightAttribute.substr(2)) > 0)
      {
        var leftNum = int(leftAttribute.substr(2));
        var rightNum = int(rightAttribute.substr(2));
        if (leftNum < rightNum)
        {
          return -1;
        }
        else if (leftNum > rightNum)
        {
          return 1;
        }
        else
        {
          return 0;
        }
      }
      else
      {
        return leftAttribute.localeCompare(rightAttribute);
      }
    }

    public function failResources(index : int)
    {
      states[index] = FAILED;
      updateList();
    }

    var select : ComboBox;
    var list : List;
    var listStatus : ListStatusClip;
    var nodes : ActiveNodes;

    var names : Array;
    var uuids : Array;
    var used : Array;
    var tickets : Array;
    var states : Array;

    var NORMAL = 0;
    var LOADING = 1;
    var FAILED = 2;

    public static var cmNames : Array = new Array("", "ProtoGENI", "Emulab");
    static var cmUrls : Array =
      new Array(null,
                "https://myboss.myelab.testbed.emulab.net:443/protogeni/xmlrpc",
                "https://boss.emulab.net:443/protogeni/xmlrpc/");
    static var cmResults : Array =
      new Array(null,
                "<rspec xmlns=\"http://protogeni.net/resources/rspec/0.1\"> "+
                " <node uuid=\"de9803c2-773e-102b-8eb4-001143e453fe\" " +
                "       hrn=\"geni1\" " +
                "       virtualization_type=\"emulab-vnode\"> " +
                " </node>" +
                " <node uuid=\"de995217-773e-102b-8eb4-001143e453fe\" " +
                "       hrn=\"geni-other\" " +
                "       virtualization_type=\"emulab-vnode\"> " +
                " </node>" +
                "</rspec>",

                "<rspec xmlns=\"http://protogeni.net/resources/rspec/0.1\"> "+
                " <node uuid=\"deb23428-773e-102b-8eb4-001143e453fe\" " +
                "       hrn=\"pgeni4\" " +
                "       virtualization_type=\"emulab-vnode\"> " +
                " </node>" +
                " <node uuid=\"deb06bd8-773e-102b-8eb4-001143e453fe\" " +
                "       hrn=\"pgeni2\" " +
                "       virtualization_type=\"plab_node\"> " +
                " </node>" +
                " <node uuid=\"deaaad4b-773e-102b-8eb4-001143e453fe\" " +
                "       hrn=\"pgeni3\" " +
                "       virtualization_type=\"plab_node\"> " +
                " </node>" +
                "</rspec>");
  }
}
