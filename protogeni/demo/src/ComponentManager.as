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

  public class ComponentManager
  {

    public static var NORMAL = 0;
    public static var LOADING = 1;
    public static var FAILED = 2;

    public function ComponentManager(newId : String,
                                     newName : String,
                                     newHostName : String,
                                     newUrl : String,
                                     newUpdate : Function,
                                     newVersion : int,
                                                                         newHrn : String = "") : void
    {
      id = newId;
      name = newName;
      hostName = newHostName;
      url = newUrl;
      ad = "";
      manifest = "";
      update = newUpdate;
      version = newVersion;
      changed = false;

      bgpAddress = "";
      bgpNetmask = "";

      components = new Array();
      used = new Array();
      ticket = null;
      sliver = null;
      state = LOADING;
    }

    public function setUpdate(newUpdate : Function) : void
    {
      update = newUpdate;
    }

    public function getBgpAddress() : String
    {
      return bgpAddress;
    }

    public function getBgpNetmask() : String
    {
      return bgpNetmask;
    }

    public function getName() : String
    {
      return name;
    }

    public function getUrl() : String
    {
      return url;
    }

    public function getHrn() : String
    {
      return url;
    }

    public function getAd() : String
    {
      return ad;
    }

    public function getManifest() : String
    {
      return manifest;
    }

    public function setManifest(newManifest : String) : void
    {
      manifest = newManifest;
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

    public function getComponentFromUuid(uuid : String) : Component
    {
      return uuidToNode[uuid];
    }

    public function getIndexFromUuid(uuid : String) : int
    {
                return components.indexOf(uuidToNode[uuid]);
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

    private function findId(id : String) : int
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
      try
      {
        ad = str;
        if (str != null)
        {
          var parser = new AdParser();
          parser.parse(str);
          uuidToNode = parser.getIdToNode();
          components = parser.getComponents();
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

    private var id : String;
    private var name : String;
    private var hostName : String;
    private var url : String;
    private var ad : String;
    private var manifest : String;

    private var components : Array;
    private var uuidToNode : Dictionary;
    private var used : Array;
    private var ticket : String;
    // Credential for the sliver
    private var sliver : String;
    private var state : int;
    private var update : Function;

    private var changed : Boolean;

    var bgpAddress : String;
    var bgpNetmask : String;

    // Version 0 is baseline from GEC 4
    // Version 1 is request converted but advertisement the same
    // Version 2 is both ad and request converted
    private var version : int;
  }
}
