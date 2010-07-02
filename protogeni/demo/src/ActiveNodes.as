/* GENIPUBLIC-COPYRIGHT
 * Copyright (c) 2008-2009 University of Utah and the Flux Group.
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
  import flash.display.DisplayObject;
  import flash.display.DisplayObjectContainer;
  import flash.display.Sprite;
  import flash.display.Shape;
  import flash.display.CapsStyle;
  import flash.display.LineScaleMode;
  import flash.text.TextField;
  import flash.events.MouseEvent;

  class ActiveNodes
  {
    public function ActiveNodes(newParent : DisplayObjectContainer,
                                newTrash : DisplayObject,
                                newDescription : TextField) : void
    {
      parent = newParent;
      linkLayer = new Sprite();
      parent.addChild(linkLayer);
      nodeLayer = new Sprite();
      parent.addChild(nodeLayer);
      trash = newTrash;
      description = newDescription;
      nodes = new Array();
      links = new Array();
      dragging = null;
      dragX = 0;
      dragY = 0;
      newLink = null;
      selected = NO_SELECTION;
    }

    public function cleanup() : void
    {
      parent.stage.removeEventListener(MouseEvent.MOUSE_MOVE, moveDrag);
      parent.stage.removeEventListener(MouseEvent.MOUSE_UP, endDrag);
      parent.stage.removeEventListener(MouseEvent.MOUSE_MOVE, moveAddLink);
      parent.stage.removeEventListener(MouseEvent.MOUSE_UP, endAddLink);

      var i : int = 0;
      for (i = 0; i < links.length; ++i)
      {
        links[i].cleanup();
      }

      for (i = 0; i < nodes.length; ++i)
      {
        nodes[i].cleanup();
      }
      nodes = null;
      parent = null;
      trash = null;
      dragging = null;
      if (newLink != null)
      {
        parent.removeChild(newLink);
        newLink = null;
      }
      nodeLayer.parent.removeChild(nodeLayer);
      nodeLayer = null;
      linkLayer.parent.removeChild(linkLayer);
      linkLayer = null;
    }

    public function addNode(component : Component, manager : ComponentManager,
                            nodeIndex : int, x : int, y : int,
                            shouldDrag : Boolean, newSuperNode : String) : void
    {
      var newNode : Node = new Node(nodeLayer, component, manager,
                                    nodeIndex, nodes.length, newSuperNode,
                                    beginDragEvent, beginAddLink);
      dragX = Node.CENTER_X;
      dragY = Node.CENTER_Y;
      newNode.move(x - dragX, y - dragY);
      nodes.push(newNode);
      if (shouldDrag)
      {
        beginDrag(newNode);
      }
      newNode.getManager().setChanged();
    }

    function removeNode(doomedNode : Node) : void
    {
      var nodeLinks : Array = doomedNode.getLinksCopy();
      var i : int = 0;
      for (; i < nodeLinks.length; ++i)
      {
        removeLink(nodeLinks[i]);
      }
      doomedNode.getManager().setChanged();
      doomedNode.cleanup();

      var index = nodes.indexOf(doomedNode);
      if (index != -1)
      {
        // Remove node. O(nk) where n is the number of nodes and k is
        // the incidence of this node.
        nodes.splice(index, 1);
        for (i = 0; i < nodes.length; ++i)
        {
          nodes[i].renumber(i);
        }
        if (selected > index)
        {
          --selected;
        }
      }
    }

    function beginDragEvent(event : MouseEvent) : void
    {
      dragX = event.localX;
      dragY = event.localY;
      beginDrag(nodes[event.target.number]);
      changeSelect(event.target.number);
    }

    function beginDrag(clickedNode : Node) : void
    {
      dragging = clickedNode;
      parent.stage.addEventListener(MouseEvent.MOUSE_MOVE, moveDrag);
      parent.stage.addEventListener(MouseEvent.MOUSE_UP, endDrag);
    }

    function moveDrag(event : MouseEvent) : void
    {
      dragging.move(event.stageX - dragX, event.stageY - dragY);
    }

    function endDrag(event : MouseEvent) : void
    {
      parent.stage.removeEventListener(MouseEvent.MOUSE_MOVE, moveDrag);
      parent.stage.removeEventListener(MouseEvent.MOUSE_UP, endDrag);
      if (trash.hitTestPoint(event.stageX, event.stageY, false))
      {
        changeSelect(NO_SELECTION);
        removeNode(dragging);
      }
      dragging = null;
    }

    function changeSelect(newIndex : int)
    {
      if (selected != NO_SELECTION)
      {
        nodes[selected].deselect();
      }
      if (newIndex != NO_SELECTION)
      {
        nodes[newIndex].select();
      }
      selected = newIndex;
      updateSelectText();
    }

    function updateSelectText() : void
    {
      if (selected != NO_SELECTION)
      {
        description.htmlText = nodes[selected].getStatusText();
      }
      else
      {
        description.htmlText = "";
      }
    }

    function beginAddLink(event : MouseEvent) : void
    {
      newLink = new Shape();
      parent.addChild(newLink);
      parent.stage.addEventListener(MouseEvent.MOUSE_MOVE, moveAddLink);
      parent.stage.addEventListener(MouseEvent.MOUSE_UP, endAddLink);
      newLinkSource = nodes[event.target.number];
      updateAddLink(event.stageX, event.stageY);
    }

    function moveAddLink(event : MouseEvent) : void
    {
      updateAddLink(event.stageX, event.stageY);
    }

    function updateAddLink(x : int, y : int) : void
    {
      var color = Link.INVALID_COLOR;
      if (validLinkTo(x, y))
      {
        color = Link.VALID_COLOR;
      }
      newLink.graphics.clear();
      newLink.graphics.lineStyle(Link.WIDTH, color, 1.0, true,
                                 LineScaleMode.NORMAL, CapsStyle.ROUND);
      newLink.graphics.moveTo(newLinkSource.centerX(),
                              newLinkSource.centerY());
      newLink.graphics.lineTo(x, y);
    }

    function endAddLink(event : MouseEvent) : void
    {
      if (validLinkTo(event.stageX, event.stageY))
      {
        var dest = findNode(event.stageX, event.stageY);
        addLink(newLinkSource, dest);
      }
      newLink.parent.removeChild(newLink);
      newLink = null;
      newLinkSource = null;
      parent.stage.removeEventListener(MouseEvent.MOUSE_MOVE, moveAddLink);
      parent.stage.removeEventListener(MouseEvent.MOUSE_UP, endAddLink);
    }

    function findNode(x : int, y : int) : Node
    {
      var result : Node = null;
      var i : int = 0;
      for (; i < nodes.length; ++i)
      {
        if (nodes[i].contains(x, y))
        {
          result = nodes[i];
          break;
        }
      }
      return result;
    }

    function addLink(source : Node, dest : Node) : void
    {
      var newLink = new Link(linkLayer, links.length, source, dest,
                             removeLinkEvent);
      links.push(newLink);
      source.getManager().setChanged();
      dest.getManager().setChanged();
    }

    function removeLinkEvent(event : MouseEvent)
    {
      removeLink(links[event.target.number]);
    }

    function removeLink(doomedLink : Link)
    {
      doomedLink.getLeft().getManager().setChanged();
      doomedLink.getRight().getManager().setChanged();
      doomedLink.cleanup();
      var index : int = links.indexOf(doomedLink);
      if (index != -1)
      {
        // Remove a node from the middle of the array. O(n)
        links.splice(index, 1);
        var i : int = 0;
        for (; i < links.length; ++i)
        {
          links[i].renumber(i);
        }
      }
    }

    function validLinkTo(x : int, y : int)
    {
      var destNode = findNode(x, y);
      return destNode != null
        && ! hasLink(newLinkSource, destNode)
        && newLinkSource != destNode;
    }

    function hasLink(source : Node, dest : Node) : Boolean
    {
      var result : Boolean = false;
      var i : int = 0;
      for (; i < links.length; ++i)
      {
        if (links[i].doesConnect(source, dest))
        {
          result = true;
          break;
        }
      }
      return result;
    }

    public function setSliverId(cm : ComponentManager, name : String,
                                sliverId : String) : void
    {
      var i : int = 0;
      for (; i < nodes.length; ++i)
      {
        if (nodes[i].getManager() == cm
            && nodes[i].getName() == name)
        {
          nodes[i].setSliverId(sliverId);
        }
      }
    }

    public function getXml(useTunnels : Boolean, cm : ComponentManager) : XML
    {
      var version = 2;
      if (cm != null)
      {
        version = cm.getVersion();
      }
      var result = XML("<?xml version=\"1.0\" encoding=\"UTF-8\"?> "
                       + "<rspec "
                       + "xmlns=\"http://www.protogeni.net/resources/rspec/0.1\" "
                       + "type=\"request\" />");
      var i : int = 0;

      for (i = 0; i < nodes.length; ++i)
      {
        if (version >= 1 || cm == nodes[i].getManager())
        {
          var currentNode : XML = nodes[i].getXml(useTunnels, version);
          if (currentNode != null)
          {
            result.appendChild(currentNode);
          }
        }
      }

      for (i = 0; i < links.length; ++i)
      {
        if ((version >= 3 || links[i].isConnectedTo(cm))
            && (cm == ComponentView.georgia
                || ! links[i].isConnectedTo(ComponentView.georgia)))
        {
          var currentLink : XML = links[i].getXml(useTunnels, version);
          if (currentLink != null)
          {
            result.appendChild(currentLink);
          }
        }
      }

      return result;
    }

    // Returns true if cmIndex represents a component manager whose
    // nodes are used by the current rspec.
    public function managerUsed(target : ComponentManager) : Boolean
    {
      var result : Boolean = false;
      for each (var node in nodes)
      {
        if (node.getManager() == target)
        {
          result = true;
          break;
        }
      }
      return result;
    }

    public function changeState(target : ComponentManager, oldState : int,
                                newState : int) : void
    {
      var i : int = 0;
      for (; i < nodes.length; ++i)
      {
        if (nodes[i].calculateState() == oldState)
        {
          nodes[i].changeState(target, newState);
        }
      }
      updateSelectText();
    }

    public function revertState(target : ComponentManager) : void
    {
      var i : int = 0;
      for (; i < nodes.length; ++i)
      {
        nodes[i].revertState(target);
      }
      updateSelectText();
    }

    public function commitState(target : ComponentManager) : void
    {
      var i : int = 0;
      for (; i < nodes.length; ++i)
      {
        nodes[i].commitState(target);
      }
      updateSelectText();
    }

    public function existsState(target : ComponentManager,
                                newState : int) : Boolean
    {
      var result : Boolean = false;
      var i : int = 0;
      for (; i < nodes.length && !result; ++i)
      {
        result = result || nodes[i].isState(target, newState);
      }
      return result;
    }

    public function hasTunnels(target : ComponentManager) : Boolean
    {
      var result : Boolean = false;
      var i : int = 0;
      for (; i < links.length && !result; ++i)
      {
        result = result || links[i].hasTunnelTo(target);
      }
      return result;
    }

    public function getSpeedUrl() : String
    {
      var result : String = null;
      var names : String = "";
      var cms : String = "";
      var hostnames : String = "";
      var isFirst : Boolean = true;
      var i : int = 0;
      for (; i < nodes.length; ++i)
      {
        if (nodes[i].isBooted())
        {
          if (!isFirst)
          {
            names += ",";
            cms += ",";
            hostnames += ",";
          }
          isFirst = false;
          names += nodes[i].getName();
          cms += nodes[i].getManager().getName();
          hostnames += nodes[i].getHostName();
        }
      }
      if (names != "")
      {
        result = "http://boss.emulab.net/speed-test.php?";
        result += "names=" + names + "&";
        result += "cms=" + cms + "&";
        result += "hostnames=" + hostnames;
      }
      return result;
    }

    public function mapRequest(request : String,
                               manager : ComponentManager) : void
    {
      var root : XML = XML(request);
      var nodeName : QName = new QName(root.namespace(), "node");
      var nodeXml = root.elements(nodeName);
      var i : int = 0;
      for (; i < nodes.length; ++i)
      {
        nodes[i].mapRequest(nodeXml[i], manager);
      }
      updateSelectText();
    }

    var nodes : Array;
    var links : Array;
    var parent : DisplayObjectContainer;
    var linkLayer : DisplayObjectContainer;
    var nodeLayer : DisplayObjectContainer;
    var trash : DisplayObject;
    var description : TextField;
    var dragging : Node;
    var dragX : int;
    var dragY : int;
    var newLink : Shape;
    var newLinkSource : Node;
    var selected : int;

    static var NO_SELECTION : int = -1;

    public static var PLANNED : int = 0;
    public static var CREATED : int = 1;
    public static var BOOTED : int = 2;
    public static var PENDING : int = 3;

    public static var statusText = new Array("Planned",
                                             "Created",
                                             "Booted",
                                             "Pending");
  }
}
