/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2008, 2009 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * XML Parser for emulab extensions for rspec
 */

#ifdef WITH_XML

#include "emulab_extensions_parser.h"

XERCES_CPP_NAMESPACE_USE

using namespace std;
using namespace rspec_emulab_extension;

emulab_operator emulab_extensions_parser::readOperator(DOMElement* tag)
{
  struct emulab_operator op = { "", NORMAL_OPERATOR };
  if (this->hasAttribute(tag, "global_operator")) {
    op.op = this->getAttribute(tag, "global_operator");
    op.type = GLOBAL_OPERATOR;
  }
  else if (this->hasAttribute(tag, "local_operator")) {
    op.op = this->getAttribute(tag, "local_operator");
    op.type = LOCAL_OPERATOR;
  }
  return op;
}

vector<struct fd> 
emulab_extensions_parser::readAllFeaturesDesires (DOMElement* elem)
{
  DOMNodeList* fdNodes = elem->getElementsByTagName(XStr("emulab:fd").x());
  vector<struct fd> fds;
  for (int i = 0; i < fdNodes->getLength(); i++) 	{
    fds.push_back(this->readFeatureDesire
		  (dynamic_cast<DOMElement*>(fdNodes->item(i))));
  }
  return fds;
}

struct fd emulab_extensions_parser::readFeatureDesire (DOMElement* tag)
{
  struct fd fdObject = {
    this->getAttribute(tag, "name"),
    rspec_parser_helper::stringToNum
    (this->getAttribute(tag, "weight")),
    this->hasAttribute(tag, "violatable"),
    this->readOperator(tag)
  };
  return fdObject;
}

vector<struct property> emulab_extensions_parser::readAllProperties
(DOMElement* elem)
{
  DOMNodeList* propNodes = elem->getElementsByTagName(XStr("property").x());
  vector<struct property> properties;
  for (int i = 0; i < propNodes->getLength(); i++) {
    properties.push_back(this->readProperty
			 (dynamic_cast<DOMElement*>(propNodes->item(i))));
  }
  return properties;
}

struct property emulab_extensions_parser::readProperty (DOMElement* tag)
{
  struct property propertyObject = {
    this->getAttribute(tag, "property_name"),
    this->getAttribute(tag, "property_value"),
    rspec_parser_helper::stringToNum 
    (this->getAttribute(tag, "property_penalty")),
    this->hasAttribute(tag, "violatable"),
    this->readOperator(tag)
  };
  return propertyObject;
}

struct hardness emulab_extensions_parser::readHardness (DOMElement* tag)
{
  struct hardness hardnessObject;
  if (this->hasChild(tag, "hard")) {
    // XXX: This is a temporary fix. We will need to deal with hard
    // vclasses correctly at some point
    hardnessObject.type = HARD_VCLASS;
  }
  else /* (this->hasChildTag(tag, "soft"))  */ {
    hardnessObject.weight
      = rspec_parser_helper::stringToNum (this->readChild(tag, "weight"));
    hardnessObject.type = SOFT_VCLASS;
  }
  return hardnessObject;
}

vector<struct vclass> 
emulab_extensions_parser::readAllVClasses (DOMElement* elem)
{
  DOMNodeList* vclassNodes 
    = elem->getElementsByTagName(XStr("emulab:vclass").x());
  vector<struct vclass> vclasses;
  for (int i = 0; i < vclassNodes->getLength(); i++) {
    vclasses.push_back(this->readVClass
		       (dynamic_cast<DOMElement*>
			(vclassNodes->item(i))));
  }
  return vclasses;
}

struct vclass emulab_extensions_parser::readVClass (DOMElement* tag)
{
  struct vclass vclassObject = {
    this->getAttribute(tag, "name"),
    this->readHardness(tag),
    this->readChild(tag, "physical_type")
  };
  return vclassObject;
}

string emulab_extensions_parser::readTypeSlots (DOMElement* tag)
{
  DOMNodeList* typeSlotsNodes 
    = tag->getElementsByTagName(XStr("emulab:node_type").x());
  if (typeSlotsNodes->getLength() == 0) {
    return "";
  }
  DOMElement* typeSlotsNode = dynamic_cast<DOMElement*>(typeSlotsNodes->item(0));
  return (this->getAttribute(typeSlotsNode, "type_slots"));
}

bool emulab_extensions_parser::readStaticType (DOMElement* tag)
{
  DOMNodeList* typeSlotsNodes
    = tag->getElementsByTagName(XStr("emulab:node_type").x());
  if (typeSlotsNodes->getLength() == 0) {
    return false;
  }
  DOMElement* typeSlotsNode = dynamic_cast<DOMElement*>(typeSlotsNodes->item(0));
  return (typeSlotsNode->hasAttribute(XStr("static").x()));
}

vector<struct type_limit> 
emulab_extensions_parser::readTypeLimits (const DOMElement* tag, int& count)
{
  vector<struct type_limit> rv;
  DOMNodeList* typeLimitNodes
    = tag->getElementsByTagName(XStr("emulab:set_type_limit").x());
  count = typeLimitNodes->getLength();
  for (int i = 0; i < count; i++) {
    DOMElement* limitNode = dynamic_cast<DOMElement*>(typeLimitNodes->item(i));
    string typeClass = this->getAttribute(limitNode, "typeclass");
    string typeCount = this->getAttribute(limitNode, "count");
    struct type_limit limit = { 
      typeClass , 
      (int)rspec_parser_helper::stringToNum(typeCount) 
    };
    rv.push_back(limit);
  }
  return rv;
}

#endif // WITH_XML
