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
                         newName : String, newId : String,
                         newCmIndex : int, newNodeIndex : int,
                         newCleanupMethod : Function, newNumber : int,
                         newMouseDownNode : Function,
                         newMouseDownLink : Function) : void
    {
      name = newName;
      id = newId;
      cmIndex = newCmIndex;
      nodeIndex = newNodeIndex;
      cleanupMethod = newCleanupMethod;
      mouseDownNode = newMouseDownNode;
      mouseDownLink = newMouseDownLink;

      clip = new NodeClip();
      parent.addChild(clip);
      clip.width = WIDTH;
      clip.height = HEIGHT;
      clip.node.nameField.text = name;
      clip.node.mouseChildren = false;
      clip.node.addEventListener(MouseEvent.MOUSE_DOWN, mouseDownNode);
      clip.node.selected.visible = false;
      clip.addLink.addEventListener(MouseEvent.MOUSE_DOWN, mouseDownLink);
      renumber(newNumber);

      links = new Array();
      changeState(cmIndex, ActiveNodes.PLANNED);
    }

    public function cleanup() : void
    {
      cleanupMethod(cmIndex, nodeIndex);
      clip.node.removeEventListener(MouseEvent.MOUSE_DOWN, mouseDownNode);
      clip.addLink.removeEventListener(MouseEvent.MOUSE_DOWN, mouseDownLink);
      clip.parent.removeChild(clip);
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

    static var maxY : int = 255;

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

    public function getCmIndex() : int
    {
      return cmIndex;
    }

    public function changeState(index : int, newState : int) : void
    {
      if (index == cmIndex)
      {
        clip.node.nameField.backgroundColor = stateColor[newState];
        state = newState;
      }
    }

    public function getXml(targetIndex : int) : XML
    {
      var result : XML = null;
      if (cmIndex == targetIndex)
      {
//        result = <node> </node>;
//        result.@uuid = id;
//        result.@nickname = name;
//        result.@virtualization_type = virtType;
        result = XML("<node uuid=\"" + id + "\" nickname=\"" + name + "\" virtualization_type=\"" + virtType + "\"> </node>");
      }
      return result;
    }

    public function isState(index : int, exemplar : int) : Boolean
    {
      return index == cmIndex && state == exemplar;
    }

    public function getStatusText() : String
    {
      var result : String = "";
      result += "<font color=\"#7777ff\">Name:</font> " + name + "\n";
      result += "<font color=\"#7777ff\">UUID:</font> " + id + "\n";
      result += "<font color=\"#7777ff\">Component Manager:</font> "
        + ComponentManager.cmNames[cmIndex] + "\n";
      if (state != ActiveNodes.PLANNED)
      {
        result += "<font color=\"#7777ff\">Status:</font> "
          + ActiveNodes.statusText[state];
      }
      return result;
    }

    var clip : NodeClip;
    var name : String;
    var id : String;
    var cmIndex : int;
    var nodeIndex : int;
    var cleanupMethod : Function;
    var links : Array;
    var mouseDownNode : Function;
    var mouseDownLink : Function;
    var state : int;

    static var virtType = "emulab-vnode";

    public static var SCALE : Number = 1.0;
    public static var WIDTH : int = Math.floor(137*SCALE);
    public static var HEIGHT : int = Math.floor(23*SCALE);
    public static var CENTER_X : int = Math.floor(WIDTH/2);
    public static var CENTER_Y : int = Math.floor(HEIGHT/2);

    static var stateColor = new Array(0xaaaaff,
                                      0xaaaaaa,
                                      0xaaffaa,
                                      0xaaffff,
                                      0xffaaaa);
  }
}
