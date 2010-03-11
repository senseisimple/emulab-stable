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
      managers = new ComponentView(clip.cmSelect, clip.nodeList, nodes);
      console = new Console(parent, nodes, managers, clip,
                            credential, Main.getText(), sliceName);
      console.discoverResources();
      wait = new SliceWait(clip.wait, clip.opText, console);
      abstract = new AbstractNodes(clip, nodes, managers);
    }

        public override function getComponent(urn:String, cm:String):String
        {
                var compm : ComponentManager = null;
                var cms : Array = managers.getManagers();

                // Get CM
                var length:int = cms.length;
                for(var i:int = 0; i < length; i++)
                {
                        if( cms[i].getName() == cm )
                        {
                                compm = ComponentManager(cms[i]);
                                break;
                        }
                }

                // Get component
                if(compm != null)
                {
                        var index:int;
                        try
                        {
                                index = compm.getIndexFromUuid(urn);
                        } catch(e:Error)
                        {
                                return "Node not found, most likely because it isn't available.";
                        }
                        if(compm.isUsed(index))
                                return "Component is being used already";
                        var component = compm.getComponent(index);
                        var superNode = null;
                        var superNodeName = null;
                        if (component.superNode != -1)
                        {
                                superNode = compm.getComponent(component.superNode);
                                superNodeName = superNode.name;
                        }
                        var randomX:Number = Math.round(Math.random() * 601 + 180);
                        var randomY:Number = Math.round(Math.random() * 385 + 75);
                        nodes.addNode(component, compm, index,
                                                          randomX, randomY, false,
                                                          superNodeName);
                compm.addUsed(index);
                        if (superNode != null)
                        {
                                randomX = Math.round(Math.random() * 601 + 180);
                                randomY = Math.round(Math.random() * 385 + 75);
                                nodes.addNode(superNode, compm, component.superNode,
                                                        randomX, randomY, false, null);
                                compm.addUsed(component.superNode);
                        }
                return "Successfully added " + urn + " on " + cm + "!";
                } else {
                        return "Could not find component manager named " + cm;
                }
        }

    override public function cleanup() : void
    {
      abstract.cleanup();
      wait.cleanup();
      console.cleanup();
      managers.cleanup();
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
    var managers : ComponentView;
    var console : Console;
    var wait : SliceWait;
    var abstract : AbstractNodes;
  }
}
