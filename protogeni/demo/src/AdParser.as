package
{
  import flash.utils.Dictionary;

  public class AdParser
  {
    public function AdParser() : void
    {
    }

    public function getComponents() : Array
    {
      return components;
    }

    public function getIdToNode() : Dictionary
    {
      return idToNode;
    }

    public function parse(str : String) : void
    {
      components = new Array();
      idToNode = new Dictionary();
      idToIface = new Dictionary();
      subNodeOf = new Dictionary();
      var rspec : XML = XML(str);
      ns = rspec.namespace();
      setVersion();

      var xmlNodes = getSortedNodes(rspec);
      var i : int = 0;
      for (; i < xmlNodes.length; ++i)
      {
        var newComponent = parseNode(xmlNodes[i]);
        components.push(newComponent);
      }

      setSubNodes();
      parseLinks(rspec);
      parseBgpPrefixes(rspec);
    }

    private function getSortedNodes(rspec : XML) : Array
    {
      var result : Array = new Array();
      for each (var element in rspec.elements(toName("node")))
      {
        var uuid : String = null;
        if (version == 1)
        {
          uuid = element.attribute("component_uuid");
        }
        else
        {
          uuid = element.attribute("component_id");
        }
        if (uuid != "" && isAvailable(element))
        {
          result.push(element);
        }
      }
      result.sort(xmlSort);
      return result;
    }

    private function parseNode(node : XML) : Component
    {
      var com : Component = new Component();
      var uuid : String = null;
      if (version == 1)
      {
        uuid = node.attribute("component_uuid");
      }
      else
      {
        uuid = node.attribute("component_id");
      }
      idToNode[uuid] = com;
      com.name = node.attribute("component_name");
      com.uuid = uuid;
      if (version == 1)
      {
        com.managerId = node.attribute("component_manager_uuid");
      }
      else
      {
        com.managerId = node.attribute("component_manager_id");
      }
      parseNodeTypes(node, com);
      parseSubNode(node, com.uuid);
      var interfaceNumber = 0;
      for each (var inter in node.elements(toName("interface")))
      {
        var interName = null;
        interName = inter.attribute("component_id");
        var newInterface = new Interface("virt-"
                                         + String(interfaceNumber),
                                         interName);
        idToIface[interName] = newInterface;
        var interRole = inter.attribute("role");
        if (interRole == "control")
        {
          newInterface.role = Interface.UNUSED_CONTROL;
        }
        else if (interRole == "experimental" || interRole == "mixed")
        {
          newInterface.role = Interface.UNUSED_EXPERIMENTAL;
        }
        com.interfaces.push(newInterface);
        ++interfaceNumber;
      }
      return com;
    }


    private function parseNodeTypes(node : XML, com : Component) : void
    {
/*
      for each (var currentType in node.elements(toName("node_type")))
      {
        if (currentType.attribute("type_name") == "bgpmux")
        {
          com.isBgpMux = true;
          for each (var currentField in currentType.elements(toName("field")))
          {
            if (currentField.attribute("key") == "upstream_as")
            {
              com.upstreamAs = currentField.attribute("value");
            }
          }
        }
      }
*/
    }

    private function parseSubNode(node : XML,
                                  uuid : String) : void
    {
/*
      for each (var current in node.elements(toName("subnode_of")))
      {
        var text = current.text();
        var parent = text.toString();
        subNodeOf[uuid] = parent;
      }
*/
    }

    private function setSubNodes() : void
    {
/*
      for each (var current in components)
      {
        if (subNodeOf[current.uuid] != null)
        {
          current.superNode = findId(subNodeOf[current.uuid]);
        }
      }
*/
    }
/*
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
*/
    private function parseLinks(rspec : XML) : void
    {
      for each (var link in rspec.elements(toName("link")))
      {
        var bandwidth : int = 100000;
        if (version == 1)
        {
          for each (var current in link.elements(toName("bandwidth")))
          {
            bandwidth = int(current.text());
          }
        }
        for each (var inter in link.descendants(toName("interface_ref")))
        {
          var iface = getInterface(inter);
          if (iface != null)
          {
            if (iface.role == Interface.UNUSED_EXPERIMENTAL)
            {
              iface.role = Interface.EXPERIMENTAL;
            }
            else if (iface.role == Interface.UNUSED_CONTROL)
            {
              iface.role = Interface.CONTROL;
            }
            if (version == 1)
            {
              iface.bandwidth = bandwidth;
            }
            else
            {
              iface.bandwidth = findBandwidth(link, iface.uuid);
            }
          }
        }
      }
    }

    private function getInterface(xmlNode : XML) : Interface
    {
      var result = null;
      if (version == 1)
      {
        var interName : String = null;
        var uuid : String = xmlNode.attribute("component_node_uuid");
        interName = xmlNode.attribute("component_interface_id");
        var node : Component = idToNode[uuid];
        if (node != null)
        {
          for each (var nodeInter in xmlNode.interfaces)
          {
            if (interName == nodeInter.name)
            {
              result = nodeInter;
              break;
            }
          }
        }
      }
      else
      {
        var id : String = xmlNode.attribute("component_id");
        result = idToIface[id];
      }
      return result;
    }

    private function findBandwidth(link : XML, name : String) : int
    {
      var result : int = 100000;
      for each (var prop in link.elements(toName("property")))
      {
        var source_id = String(prop.@source_id);
        var bandwidth = String(prop.@capacity);
        if (String(prop.@source_id) == name)
        {
          result = parseInt(bandwidth);
          break;
        }
      }
      return result;
    }

    private function parseBgpPrefixes(rspec : XML) : void
    {
/*
      for each (var prefix in rspec.elements(toName("bgp_prefix")))
      {
        bgpAddress = prefix.attribute("address");
        bgpNetmask = prefix.attribute("netmask");
      }
*/
    }

    private function isAvailable(node : XML) : Boolean
    {
      var result : Boolean = false;
      var available = node.elements(toName("available"));
      for each (var element in available)
      {
        var text : String = "";
        if (version == 1)
        {
          text = element.text();
        }
        else
        {
          text = element.@now;
        }
        if (text.toString() == "true")
        {
          result = true;
          break;
        }
      }
      return result;
    }


    private function xmlSort(left : XML, right : XML) : int
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

    private function toName(str : String) : QName
    {
      return new QName(ns, str);
    }

    private function setVersion() : void
    {
      if (ns == null
          || ns.uri == "http://www.protogeni.net/resources/rspec/0.1"
          || ns.uri == "http://www.protogeni.net/resources/rspec/0.2")
      {
        version = 1;
      }
      else if (ns.uri == "http://www.protogeni.net/resources/rspec/2")
      {
        version = 2;
      }
      else
      {
        throw new Error("Unsupported rspec version: " + ns.uri);
      }
    }

    private var version : int;
    private var components : Array;
    private var idToNode : Dictionary;
    private var idToIface : Dictionary;
    private var subNodeOf : Dictionary;
    private var ns : Namespace;
  }
}
