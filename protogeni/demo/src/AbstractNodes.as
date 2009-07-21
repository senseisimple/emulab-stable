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
  import flash.events.MouseEvent;

  public class AbstractNodes
  {
    public function AbstractNodes(clip : SliceDetailClip,
                                  newNodes : ActiveNodes,
                                  newManagers : ComponentView) : void
    {
      exclusive = clip.exclusiveNode;
      shared = clip.sharedNode;
      nodes = newNodes;
      managers = newManagers;

      setupNode(exclusive, "Exclusive");
      setupNode(shared, "Shared");

      exclusive.addEventListener(MouseEvent.MOUSE_DOWN, createExclusive);
      shared.addEventListener(MouseEvent.MOUSE_DOWN, createShared);
    }

    public function cleanup() : void
    {
      exclusive.removeEventListener(MouseEvent.MOUSE_DOWN, createExclusive);
      shared.removeEventListener(MouseEvent.MOUSE_DOWN, createShared);
    }

    function setupNode(node : NodeClip, name : String) : void
    {
      node.node.nameField.text = name;
      node.node.cmField.text = "*";
      node.node.mouseChildren = false;
      node.node.selected.visible = false;
    }

    function createExclusive(event : MouseEvent) : void
    {
      createNode(event, "exclusive", false);
    }

    function createShared(event : MouseEvent) : void
    {
      createNode(event, "shared", true);
    }

    function createNode(event : MouseEvent, namePrefix : String,
                        isShared : Boolean) : void
    {
      if (managers.isValidManager())
      {
        var currentManager = managers.getCurrentManager();
        var component = new Component();
        component.name = namePrefix + nextNumber();
        component.isVirtual = true;
        component.isShared = isShared;
        nodes.addNode(component, currentManager, -1,
                      event.stageX, event.stageY);
      }
    }

    static function nextNumber() : int
    {
      ++nameSuffix;
      return nameSuffix;
    }

    static var nameSuffix = 0;

    var exclusive : NodeClip;
    var shared : NodeClip;
    var nodes : ActiveNodes;
    var managers : ComponentView;
  }
}
