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
  import flash.display.DisplayObjectContainer;
  import flash.display.Shape;
  import flash.display.CapsStyle;
  import flash.display.LineScaleMode;
  import flash.events.MouseEvent;

  class Link
  {
    public function Link(parent : DisplayObjectContainer,
                         newNumber : int, newLeft : Node, newRight : Node,
                         newRemoveClick : Function) : void
    {
      number = newNumber;
      left = newLeft;
      left.addLink(this);
      right = newRight;
      right.addLink(this);
      leftInterface = "";
      rightInterface = "";
      tunnelIp = 0;
      if (isTunnel())
      {
        tunnelIp = getNextTunnel();
      }
      else
      {
        leftInterface = left.allocateInterface();
        rightInterface = right.allocateInterface();
        if (leftInterface == "*" || rightInterface == "*")
        {
          Main.getConsole().appendText("\n\nWARNING: Interface not found\n\n");
        }
      }

      removeClick = newRemoveClick;
      canvas = new Shape();
      parent.addChild(canvas);
      removeClip = new RemoveLinkClip();
      parent.addChild(removeClip);
      renumber(number);
      removeClip.addEventListener(MouseEvent.CLICK, removeClick);
      update();
    }

    public function cleanup() : void
    {
      removeClip.removeEventListener(MouseEvent.CLICK, removeClick);
      removeClip.parent.removeChild(removeClip);
      removeClip = null;
      removeClick = null;
      canvas.parent.removeChild(canvas);
      canvas = null;
      left.freeInterface(leftInterface);
      left.removeLink(this);
      right.freeInterface(rightInterface);
      right.removeLink(this);
    }

    public function renumber(number : int) : void
    {
      removeClip.number = number;
    }

    public function update() : void
    {
      canvas.graphics.clear();
      var color = ESTABLISHED_COLOR;
      if (isTunnel())
      {
        color = TUNNEL_COLOR;
      }
      canvas.graphics.lineStyle(Link.WIDTH, color, 1.0, true,
                                 LineScaleMode.NORMAL, CapsStyle.ROUND);
      canvas.graphics.moveTo(left.centerX(), left.centerY());
      canvas.graphics.lineTo(right.centerX(), right.centerY());
      removeClip.x = (left.centerX() + right.centerX())/2;
      removeClip.y = (left.centerY() + right.centerY())/2;
    }

    public function doesConnect(otherLeft : Node, otherRight : Node)
    {
      return (left == otherLeft && right == otherRight)
        || (left == otherRight && right == otherLeft);
    }

    public function getXml(useTunnels : Boolean) : XML
    {
      var result : XML = null;
      if (!isTunnel() || useTunnels)
      {
        result = <link />;
//          result.@name = "link" + String(number);
        result.@virtual_id = "link" + String(number);
        if (isTunnel())
        {
          result.@link_type= "tunnel";
        }

        result.appendChild(getInterfaceXml(left, leftInterface, 0));
        result.appendChild(getInterfaceXml(right, rightInterface, 1));
      }
      return result;
    }

    function getInterfaceXml(node : Node, interfaceName : String,
                             ipOffset : int) : XML
    {
      var result = <interface_ref />;
      result.@virtual_node_id = node.getName();
      if (isTunnel())
      {
        result.@tunnel_ip = ipToString(tunnelIp + ipOffset);
        result.@virtual_interface_id = "control";
      }
      else
      {
        result.@virtual_interface_id = interfaceName;
      }
      return result;
    }

    function ipToString(ip : int) : String
    {
      var first : int = ((ip >> 8) & 0xff);
      var second : int = (ip & 0xff);
      return "192.168." + String(first) + "." + String(second);
    }

    public function isTunnel() : Boolean
    {
      return left.getManager() != right.getManager();
    }

    public function hasTunnelTo(target : ComponentManager) : Boolean
    {
      return isTunnel() && (left.getManager() == target
                            || right.getManager() == target);
    }

    var number : int;
    var left : Node;
    var leftInterface : String;
    var right : Node;
    var rightInterface : String;
    var tunnelIp : int;
    var canvas : Shape;
    var removeClick : Function;
    var removeClip : RemoveLinkClip;

    static var tunnelNext : int = 1;

    static function getNextTunnel() : int
    {
      var result = tunnelNext;
      tunnelNext += 2;
      return result;
    }

    public static var WIDTH = 4;
    public static var ESTABLISHED_COLOR = 0x0000ff;
    public static var TUNNEL_COLOR = 0x00ffff;
    public static var INVALID_COLOR = 0xff0000;
    public static var VALID_COLOR = 0x00ff00;
  }
}
