/* GENIPUBLIC-COPYRIGHT
 * Copyright (c) 2009 University of Utah and the Flux Group.
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
  import flash.utils.Dictionary;

  class ComponentManager
  {

    static var NORMAL = 0;
    static var LOADING = 1;
    static var FAILED = 2;

    public function ComponentManager(newId : String,
                                     newName : String,
                                     newHostName : String,
                                     newUrl : String,
                                     newUpdate : Function,
                                     newVersion : int) : void
    {
      id = newId;
      name = newName;
      hostName = newHostName;
      url = newUrl;
      ad = "";
      update = newUpdate;
      version = newVersion;
      changed = false;

      components = new Array();
      used = new Array();
      ticket = null;
      sliver = null;
      state = LOADING;
    }

    public function getName() : String
    {
      return name;
    }

    public function getUrl() : String
    {
      return url;
    }

    public function getAd() : String
    {
      return ad;
    }

    public function getHostName() : String
    {
      return hostName;
    }

    public function getId() : String
    {
      return id;
    }

    public function setState(newState : int) : void
    {
      state = newState;
    }

    public function getState() : int
    {
      return state;
    }

    public function setTicket(newTicket : String) : void
    {
      ticket = newTicket;
    }

    public function getTicket() : String
    {
      return ticket;
    }

    public function setSliver(newSliver : String) : void
    {
      sliver = newSliver;
    }

    public function getSliver() : String
    {
      return sliver;
    }

    public function isUsed(nodeIndex : int) : Boolean
    {
      return used.indexOf(nodeIndex) != -1;
    }

    public function addUsed(nodeIndex : int) : void
    {
      used.push(nodeIndex);
    }

    public function removeUsed(nodeIndex : int) : void
    {
      var target : int = used.indexOf(nodeIndex);
      if (target != -1)
      {
        used.splice(target, 1);
      }
      update();
    }

    public function getUsed() : Array
    {
      return used.slice();
    }

    public function getComponent(nodeIndex : int) : Component
    {
      return components[nodeIndex];
    }

    public function getComponentCount() : int
    {
      return components.length;
    }

    public function getVersion() : int
    {
      return version;
    }

    public function setChanged() : void
    {
      changed = true;
    }

    public function clearChanged() : void
    {
      changed = false;
    }

    public function hasChanged() : Boolean
    {
      return changed;
    }

    public function isUsedString(id : String) : Boolean
    {
      var result = true;
      var index = findId(id);
      if (index != -1)
      {
        result = isUsed(index);
      }
      return result;
    }

    // Returns the node index of the now used node.
    public function makeUsed(id : String) : int
    {
      var index = findId(id);
      if (index != -1 && ! isUsed(index))
      {
        addUsed(index);
        update();
      }
      return index;
    }

    function findId(id : String) : int
    {
      var result : int = -1;
      var i : int = 0;
      for (; i < components.length && result == -1; ++i)
      {
        if (components[i].uuid == id)
        {
          result = i;
        }
      }
      return result;
    }

    public function resourceFailure() : void
    {
      setState(FAILED);
      update();
    }

    public function resourceSuccess(str : String) : void
    {
      var console = Main.getConsole();
      if (console != null)
      {
        console.appendText("\npopulateNodes: " + name + "\n");
      }
      try
      {
        ad = str;
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
            if (version < 2 || isAvailable(xmlNodes[i]))
            {
              var com : Component = new Component();
              components.push(com);
              var uuid : String = xmlNodes[i].attribute("component_uuid");
              uuidToNode[uuid] = com;
              com.name = xmlNodes[i].attribute("component_name");
              com.uuid = uuid;
              if (version < 2)
              {
                com.managerId = id;
              }
              else
              {
                com.managerId = xmlNodes[i].attribute("component_manager_uuid");
              }
              var interfaceName = new QName(rspec.namespace(), "interface");
              var interfaceList = xmlNodes[i].elements(interfaceName);
              var interfaceNumber = 0;
              for each (var inter in interfaceList)
              {
                var interName = null;
                if (version < 2)
                {
                  interName = inter.attribute("component_name");
                }
                else
                {
                  interName = inter.attribute("component_id");
                }
                var newInterface = new Interface("virt-"
                                                 + String(interfaceNumber),
                                                 interName);
                com.interfaces.push(newInterface);
                ++interfaceNumber;
              }
            }
          }

          parseLinks(rspec, uuidToNode);
        }
        setState(NORMAL)
      }
      catch (e : Error)
      {
        var text = Main.getConsole();
        if (text != null)
        {
          text.appendText("\n" + e.toString() + "\n");
        }
        setState(FAILED);
      }
      update();
    }

    function parseLinks(rspec : XML, uuidToNode : Dictionary) : void
    {
      var linkName : QName = new QName(rspec.namespace(), "link");
      for each (var link in rspec.elements(linkName))
      {
        var interElement : QName = new QName(rspec.namespace(),
                                             "interface_ref");
        if (version < 2)
        {
          interElement = new QName(rspec.namespace(), "interface");
        }
        for each (var inter in link.descendants(interElement))
        {
          var interName : String = null;
          var uuid : String = null;
          if (version < 2)
          {
            var uuidList = inter.elements(new QName(rspec.namespace(),
                                                    "component_node_uuid"));
            uuid = uuidList.text();
            var nameList = inter.elements(new QName(rspec.namespace(),
                                                  "component_interface_name"));
            interName = nameList.text();
          }
          else
          {
            uuid = inter.attribute("component_node_uuid");
            interName = inter.attribute("component_interface_id");
          }
          var node : Component = uuidToNode[uuid];
          if (node != null)
          {
            for each (var nodeInter in node.interfaces)
            {
              if (interName == nodeInter.name)
              {
                nodeInter.role = Interface.EXPERIMENTAL;
              }
            }
          }
        }
      }
    }

    function isAvailable(node : XML) : Boolean
    {
      var result : Boolean = false;
      var availableName : QName = new QName(node.namespace(),
                                            "available");
      var available = node.elements(availableName);
      for each (var element in available)
      {
        var text = element.text();
        if (text.toString() == "true")
        {
          result = true;
          break;
        }
      }
      return result;
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

    var id : String;
    var name : String;
    var hostName : String;
    var url : String;
    var ad : String;

    var components : Array;
    var used : Array;
    var ticket : String;
    // Credential for the sliver
    var sliver : String;
    var state : int;
    var update : Function;

    var changed : Boolean;

    // Version 0 is baseline from GEC 4
    // Version 1 is request converted but advertisement the same
    // Version 2 is both ad and request converted
    var version : int;
  }
}
