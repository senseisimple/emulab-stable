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
  import flash.text.TextField;

  class MenuSliceDetail extends MenuState
  {
    public function MenuSliceDetail(newSliceName : String,
                                    newSliceId : String,
                                    newCredential : Credential) : void
    {
      sliceName = newSliceName;
      sliceId = newSliceId;
      credential = newCredential;
    }

    override public function init(newParent : DisplayObjectContainer) : void
    {
      parent = newParent;
      clip = new SliceDetailClip();
      parent.addChild(clip);

      clip.sliceName.text = sliceName;
      nodes = new ActiveNodes(parent, clip.nodeList, clip.description);
      cm = new ComponentManager(clip.cmSelect, clip.nodeList, nodes);
      credential.setupSlivers(cm.getCmCount());
      console = new Console(parent, nodes, cm, clip,
                            credential, Main.getText());
      console.discoverResources();
    }

    override public function cleanup() : void
    {
      console.cleanup();
      cm.cleanup();
      nodes.cleanup();
      clip.parent.removeChild(clip);
    }

    override public function getConsole() : TextField
    {
      return console.getConsole();
    }

    var sliceName : String;
    var sliceId : String;
    var credential : Credential;
    var parent : DisplayObjectContainer;
    var clip : SliceDetailClip;
    var nodes : ActiveNodes;
    var cm : ComponentManager;
    var console : Console;
  }
}
