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
  import flash.display.MovieClip;
  import flash.display.DisplayObjectContainer;
  import flash.text.TextField;
  import flash.events.MouseEvent;
  import NodeClip;

  public class Node
  {
    public function Node(parent : DisplayObjectContainer,
                         newComponent : Component,
                         newManager : ComponentManager, newNodeIndex : int,
                         newNumber : int, newMouseDownNode : Function,
                         newMouseDownLink : Function) : void
    {
      name = newComponent.name;
      id = newComponent.uuid;
      managerId = newManager.getId();
      isVirtual = newComponent.isVirtual;
      isShared = newComponent.isShared;
      sliverId = null;
      interfaces = cloneInterfaces(newComponent);
      manager = newManager;
      nodeIndex = newNodeIndex;
      mouseDownNode = newMouseDownNode;
      mouseDownLink = newMouseDownLink;

      clip = new NodeClip();
      parent.addChild(clip);
      clip.width = WIDTH;
      clip.height = HEIGHT;
      clip.node.addEventListener(MouseEvent.MOUSE_DOWN, mouseDownNode);
      clip.addLink.addEventListener(MouseEvent.MOUSE_DOWN, mouseDownLink);
      clip.node.mouseChildren = false;
      clip.node.selected.visible = false;
      updateNodeClip();
      renumber(newNumber);

      links = new Array();
      oldState = ActiveNodes.PLANNED;
      newState = ActiveNodes.PLANNED;
      updateState();
    }

    public function cleanup() : void
    {
      if (! isVirtual)
      {
        manager.removeUsed(nodeIndex);
      }
      clip.node.removeEventListener(MouseEvent.MOUSE_DOWN, mouseDownNode);
      clip.addLink.removeEventListener(MouseEvent.MOUSE_DOWN, mouseDownLink);
      clip.parent.removeChild(clip);
    }

    function cloneInterfaces(newComponent : Component) : Array
    {
      var result : Array = new Array();
      for each (var inter in newComponent.interfaces)
      {
        result.push(inter.clone());
      }
      return result;
    }

    function updateNodeClip() : void
    {
      clip.node.nameField.text = name;
      clip.node.cmField.text = manager.getName().substring(0, 1);
    }

    public function renumber(number : int) : void
    {
      clip.node.number = number;
      clip.addLink.number = number;
    }

    public function move(x : int, y : int) : void
    {
      clip.x = x;
      clip.y = Math.min(y, maxY);
      var i : int = 0;
      for (; i < links.length; ++i)
      {
        links[i].update();
      }
    }

    public function select() : void
    {
      clip.node.selected.visible = true;
    }

    public function deselect() : void
    {
      clip.node.selected.visible = false;
    }

    static var maxY : int = 460;

    public function centerX() : int
    {
      return Math.floor(clip.x) + CENTER_X;
    }

    public function centerY() : int
    {
      return Math.floor(clip.y) + CENTER_Y;
    }

    public function contains(targetX : int, targetY : int) : Boolean
    {
      return clip.hitTestPoint(targetX, targetY, true);
    }

    public function addLink(newLink : Link)
    {
      links.push(newLink);
    }

    public function removeLink(oldLink : Link)
    {
      var index : int = links.indexOf(oldLink);
      if (index != -1)
      {
        // Remove the node from the middle of the array. O(n)
        links.splice(index, 1);
      }
    }

    public function getLinksCopy() : Array
    {
      // slice() without arguments just copies the array.
      return links.slice();
    }

    public function getId() : String
    {
      return id;
    }

    public function setSliverId(newSliverId : String) : void
    {
      sliverId = newSliverId;
    }

    public function getSliverId() : String
    {
      return sliverId;
    }

    public function getName() : String
    {
      return name;
    }
/*
    public function getCmIndex() : int
    {
      return cmIndex;
    }
*/

    public function getManager() : ComponentManager
    {
      return manager;
    }

    public function isBooted() : Boolean
    {
      return oldState == ActiveNodes.BOOTED && newState == ActiveNodes.BOOTED;
    }

    public function getHostName() : String
    {
      return name + manager.getHostName();
    }

    public function allocateInterface() : String
    {
      var result = null;
      for each (var candidate in interfaces)
      {
        if (! candidate.used && candidate.role == Interface.EXPERIMENTAL)
        {
          candidate.used = true;
          if (manager.getVersion() == 0)
          {
            result = candidate.name;
          }
          else
          {
            result = candidate.virtualId;
          }
          break;
        }
      }
      if (isVirtual || result == null)
      {
        var newInterface = new Interface("virt-" + String(interfaces.length),
                                         null);
        newInterface.used = true;
        newInterface.role = Interface.EXPERIMENTAL;
        interfaces.push(newInterface);
        result = newInterface.virtualId;
      }
      return result;
    }

    public function freeInterface(interName : String) : void
    {
      for each (var candidate in interfaces)
      {
        if (interName == candidate.virtualId)
        {
          candidate.used = false;
          break;
        }
      }
    }

    public function interfaceUsed(interName : String) : Boolean
    {
      var result : Boolean = false;
      for each (var candidate in interfaces)
      {
        if (interName == candidate.virtualId)
        {
          result = candidate.used;
          break;
        }
      }
      return result;
    }

    public function changeState(target : ComponentManager, state : int) : void
    {
      if (target == manager)
      {
        newState = state;
        updateState();
      }
    }

    public function commitState(target : ComponentManager) : void
    {
      if (target == manager)
      {
        clip.node.nameField.textColor = 0x000000;
        oldState = newState;
        updateState();
      }
    }

    public function revertState(target : ComponentManager) : void
    {
      if (target == manager)
      {
        clip.node.nameField.textColor = 0xff0000;
        newState = oldState;
        updateState();
      }
    }

    public function calculateState() : int
    {
      var state : int = newState;
      if (newState != oldState)
      {
        state = ActiveNodes.PENDING;
      }
      return state;
    }

    public function updateState() : void
    {
      var state : int = calculateState();
      clip.node.nameField.backgroundColor = stateColor[state];
      if (state != oldState)
      {
        clip.alpha = 0.65;
      }
      else
      {
        clip.alpha = 1.0;
      }
    }

    public function getXml(useTunnels : Boolean, version : int) : XML
    {
      var result : XML = null;
      var str : String = "<node ";
      if (version < 1)
      {
        str += "uuid=\"" + id + "\" nickname=\"" + name
          + "\" virtualization_type=\"" + virtType + "\" ";
        if (useTunnels && sliverId != null)
        {
          str += "sliver_uuid=\"" + sliverId + "\" ";
        }
        str += "> </node>";
        result = XML(str);
      }
      else
      {
        if (! isVirtual)
        {
          str += "component_uuid=\"" + id + "\" ";
        }
        str += "component_manager_uuid=\"" + managerId + "\" ";
        str += "virtual_id=\"" + name + "\" "
          + "virtualization_type=\"" + virtType + "\" ";
        if (sliverId != null)
        {
          str += "sliver_uuid=\"" + sliverId + "\" ";
        }
        if (isShared)
        {
          str += "virtualization_subtype=\"emulab-openvz\" ";
          str += "exclusive=\"0\"";
        }
        else
        {
          str += "exclusive=\"1\"";
        }
        str += ">";
        var nodeType = "pc";
        if (isShared)
        {
          nodeType = "pcvm";
        }
        else
        {
          nodeType = "pc";
//          if (manager.getName() == "Emulab")
//          {
//            nodeType = "pc600";
//          }
        }
        str += "<node_type type_name=\"" + nodeType
          + "\" type_slots=\"1\" /> ";
        for each (var current in interfaces)
        {
          if (current.used)
          {
            str += "<interface virtual_id=\"" + current.virtualId + "\" ";
/*
            if (current.name != null)
            {
              str += "component_id=\"" + current.name + "\" ";
            }
*/
            str += " />";
          }
        }
        str += "<interface virtual_id=\"control\" />";
        str += " </node>";
        result = XML(str);
      }
      return result;
    }

    public function isState(target : ComponentManager,
                            exemplar : int) : Boolean
    {
      var state : int = calculateState();
      return target == manager && state == exemplar;
    }

    public function getStatusText() : String
    {
      var state : int = calculateState();
      var result : String = "";
      result += "<font color=\"#7777ff\">Name:</font> " + name + "\n";
      result += "<font color=\"#7777ff\">UUID:</font> " + id + "\n";
      result += "<font color=\"#7777ff\">Component Manager:</font> "
        + manager.getName() + "\n";
      if (sliverId != "" && sliverId != null)
      {
        result += "<font color=\"#7777ff\">Sliver UUID:</font> "
          + sliverId + "\n";
      }
      if (state != ActiveNodes.PLANNED)
      {
        result += "<font color=\"#7777ff\">Status:</font> "
          + ActiveNodes.statusText[state];
        if (state == ActiveNodes.PENDING)
        {
          result += ": " + ActiveNodes.statusText[oldState] + " -> "
            + ActiveNodes.statusText[newState];
        }
        result += "\n";
      }
      for each (var inter in interfaces)
      {
        if (inter.used || inter.role == Interface.CONTROL)
        {
          result += "<font color=\"#770000\">";
        }
        else
        {
          result += "<font color=\"#000000\">";
        }
        result += inter.virtualId + " ";
        result += "</font>"
      }
      return result;
    }

    public function mapRequest(root : XML,
                               mapManager : ComponentManager) : void
    {
      if (isVirtual && name == root.attribute("virtual_id"))
      {
        var newId = root.attribute("component_uuid");
        if (! manager.isUsedString(newId) || isShared)
        {
          var newIndex = mapManager.makeUsed(newId);
          if (newIndex != -1)
          {
            id = newId;
            nodeIndex = newIndex;
            manager = mapManager;
            managerId = manager.getId();
            isVirtual = false;
            var component = manager.getComponent(nodeIndex);
            if (isShared)
            {
              name = name + "_map";
            }
            else
            {
              name = component.name;
            }
/*
            if (! isShared)
            {
              var newInterfaces = cloneInterfaces(component);
              for each (var inter in newInterfaces)
              {
                inter.used = interfaceUsed(inter.virtualId);
              }
              interfaces = newInterfaces;
            }
*/
            updateNodeClip();
          }
        }
      }
    }

    var clip : NodeClip;
    var name : String;
    var id : String;
    var managerId : String;
    var sliverId : String;
    var manager : ComponentManager;
    var nodeIndex : int;
    var links : Array;
    var mouseDownNode : Function;
    var mouseDownLink : Function;
    var oldState : int;
    var newState : int;
    var interfaces : Array;
    var isVirtual : Boolean;
    var isShared : Boolean;

    static var virtType = "emulab-vnode";

    public static var SCALE : Number = 1.0;
    public static var WIDTH : int = Math.floor(137*SCALE);
    public static var HEIGHT : int = Math.floor(23*SCALE);
    public static var CENTER_X : int = Math.floor(WIDTH/2);
    public static var CENTER_Y : int = Math.floor(HEIGHT/2);

    static var stateColor = new Array(0xaaaaff,
                                      0xaaffaa,
                                      0xaaffff,
                                      0xaaaaaa);
  }
}
