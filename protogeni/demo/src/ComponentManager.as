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
  import flash.utils.Dictionary;

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

      components = new Array();

      used = new Array();
      tickets = new Array();
      states = new Array();

      select.removeAll();
      select.selectedIndex = 0;
      select.rowCount = 4;

      var i : int = 0;
      for (; i < cmNames.length; ++i)
      {
        select.addItem(new ListItem(cmNames[i], null));
        components.push(new Array());
        used.push(new Array());
        tickets.push(null);
        states.push(LOADING);
//        if (cmResults[i] != null)
//        {
//          populateNodes(i, cmResults[i]);
//        }
      }

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
        var com = components[select.selectedIndex][event.index];
        nodes.addNode(com.name,
                      com.uuid,
                      com.interfaces,
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
      for (; i < components[current].length; ++i)
      {
        list.addItem(new ListItem(components[current][i].name, "NodeNone"));
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
      var console = Main.getConsole();
      if (console != null)
      {
        console.appendText("\npopulateNodes: " + cmNames[index] + "\n");
      }
      try
      {
        if (str != null)
        {
          var uuidToNode = new Dictionary();
          var rspec : XML = XML(str);
          var nodeName : QName = new QName(rspec.namespace(), "node");
          var xmlNodes : Array = new Array();
          for each (var element in rspec.elements(nodeName))
          {
            var uuidAtt : String = element.attribute("component_uuid");
            if (uuidAtt != "")
            {
              xmlNodes.push(element);
            }
          }
          xmlNodes.sort(xmlSort);
          var i : int = 0;
          for (; i < xmlNodes.length; ++i)
          {
            var com : Component = new Component();
            components[index].push(com);
            var uuid : String = xmlNodes[i].attribute("component_uuid");
            uuidToNode[uuid] = com;
            com.name = xmlNodes[i].attribute("component_name");
            com.uuid = uuid;
            var interfaceName = new QName(rspec.namespace(), "interface");
            var interfaceList = xmlNodes[i].elements(interfaceName);
            for each (var inter in interfaceList)
            {
              var interName = inter.attribute("component_name");
              com.interfaces.push(new Interface(interName));
            }
          }

          parseLinks(rspec, uuidToNode);
        }
        states[index] = NORMAL;
        updateList();
      }
      catch (e : Error)
      {
        var text = Main.getConsole();
        if (text != null)
        {
          text.appendText("\n" + e.toString() + "\n");
        }
      }
    }

    function parseLinks(rspec : XML, uuidToNode : Dictionary) : void
    {
      var linkName : QName = new QName(rspec.namespace(), "link");
      for each (var link in rspec.elements(linkName))
      {
        var interElement : QName = new QName(rspec.namespace(), "interface");
        for each (var inter in link.descendants(interElement))
        {
               var uuidList = inter.elements(new QName(rspec.namespace(),
                                                  "component_node_uuid"));
          var uuid : String = uuidList.text();
          var nameList = inter.elements(new QName(rspec.namespace(),
                                                  "component_interface_name"));
          var interName : String = nameList.text();
          var node : Component = uuidToNode[uuid];
          if (node != null)
          {
            for each (var nodeInter in node.interfaces)
            {
              if (interName == nodeInter.name)
              {
                nodeInter.used = false;
              }
            }
          }
        }
      }
    }


    function xmlSort(left : XML, right : XML) : int
    {
      // TODO: Make this an actual natural sort
      var leftAttribute = left.attribute("component_name");
      var rightAttribute = right.attribute("component_name");
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
    // An array of arrays. Outer == CM, inner == Component
    var components : Array;

    var used : Array;
    var tickets : Array;
    var states : Array;

    var NORMAL = 0;
    var LOADING = 1;
    var FAILED = 2;

    public static var hostName : Array = new Array("",
                                                   ".emulab.net",
                                                   ".emulab.net",
                                                   ".emulab.net",
                                                   ".uky.emulab.net",
                                                   ".schooner.wail.wisc.edu",
                                                   ".cmcl.cs.cmu.edu");

    public static var cmNames : Array = new Array("", "ProtoGENI", "Emulab",
                                                  "gtwelab", "Kentucky",
                                                  "Wisconsin", "CMU");
    static var cmUrls : Array =
      new Array(null,
                "https://myboss.myelab.testbed.emulab.net:443/protogeni/xmlrpc",
                "https://boss.emulab.net:443/protogeni/xmlrpc/",
                "https://myboss.emulab.geni.emulab.net:443/protogeni/xmlrpc",
                "https://www.uky.emulab.net/protogeni/xmlrpc",
                "https://www.schooner.wail.wisc.edu/protogeni/xmlrpc",
                "https://boss.cmcl.cs.cmu.edu/protogeni/xmlrpc");
    static var cmResults : Array =
      new Array(null,
                "<rspec xmlns=\"http://protogeni.net/resources/rspec/0.1\"> "+
                " <node uuid=\"de9803c2-773e-102b-8eb4-001143e453fe\" " +
                "       name=\"geni1\" " +
                "       virtualization_type=\"emulab-vnode\"> " +
                " </node>" +
                " <node uuid=\"de995217-773e-102b-8eb4-001143e453fe\" " +
                "       name=\"geni-other\" " +
                "       virtualization_type=\"emulab-vnode\"> " +
                " </node>" +
                "</rspec>",

                "<rspec xmlns=\"http://protogeni.net/resources/rspec/0.1\"> "+
                " <node uuid=\"deb23428-773e-102b-8eb4-001143e453fe\" " +
                "       name=\"pgeni4\" " +
                "       virtualization_type=\"emulab-vnode\"> " +
                " </node>" +
                " <node uuid=\"deb06bd8-773e-102b-8eb4-001143e453fe\" " +
                "       name=\"pgeni2\" " +
                "       virtualization_type=\"plab_node\"> " +
                " </node>" +
                " <node uuid=\"deaaad4b-773e-102b-8eb4-001143e453fe\" " +
                "       name=\"pgeni3\" " +
                "       virtualization_type=\"plab_node\"> " +
                " </node>" +
                "</rspec>",
                null,
                null,
                null,
                null);
  }
}
