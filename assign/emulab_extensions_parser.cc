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

emulab_operator emulab_extensions_parser::readOperator(const DOMElement* tag)
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
emulab_extensions_parser::readFeaturesDesires (const DOMElement* ele, int& count)
{
  vector<struct fd> fds;
  DOMNodeList* fdNodes = ele->getElementsByTagName(XStr("emulab:fd").x());
  count = fdNodes->getLength();
  for (int i = 0; i < fdNodes->getLength(); i++) 	{
    fds.push_back(this->readFeatureDesire
		  (dynamic_cast<DOMElement*>(fdNodes->item(i))));
  }
  return fds;
}

struct fd emulab_extensions_parser::readFeatureDesire (const DOMElement* tag)
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

vector<struct property> emulab_extensions_parser::readProperties
(const DOMElement* elem)
{
  DOMNodeList* propNodes = elem->getElementsByTagName(XStr("property").x());
  vector<struct property> properties;
  for (int i = 0; i < propNodes->getLength(); i++) {
    properties.push_back(this->readProperty
			 (dynamic_cast<DOMElement*>(propNodes->item(i))));
  }
  return properties;
}

struct property emulab_extensions_parser::readProperty (const DOMElement* tag)
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

struct hardness emulab_extensions_parser::readHardness (const DOMElement* tag)
{
  struct hardness hardnessObject;
  string strWeight = this->getAttribute(tag, "weight");

  if (strWeight == "hard") {
    hardnessObject.type = HARD_VCLASS;
  }
  else {
    hardnessObject.type = SOFT_VCLASS;
    hardnessObject.weight = rspec_parser_helper::stringToNum(strWeight);
  }
  return hardnessObject;
}

vector<struct vclass> 
emulab_extensions_parser::readVClasses (const DOMElement* elem)
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

struct vclass 
emulab_extensions_parser::readVClass (const DOMElement* tag)
{
  vector<string> physTypes;
  DOMNodeList* physNodes
    = tag->getElementsByTagName(XStr("emulab:physical_type").x());
  for (int i = 0; i < physNodes->getLength(); i++) {
    DOMElement* physNode = dynamic_cast<DOMElement*>(physNodes->item(i));
    // XXX: This is nasty because the type name that we give assign
    // has to be the concatation of the hardware type and sliver type
    // It's not clear which is the best place to do this
    // i.e. whether to generate this concatenated type here in the parser
    // (in which case any time the formula to convert the single type to 
    // a hardware type and sliver type changes, it will have to be updated
    // both here and in libvtop) or to do it just once in libvtop in which
    // case the reverse holds true i.e. when we change the way we concatenate
    // the types and present it to assign, we will have to change the code 
    // in libvtop
    string physNodeName  = this->getAttribute(physNode, "name");
    physTypes.push_back(rspec_parser_helper::convertType(physNodeName));
  }
    
  struct vclass vclassObject = {
    rspec_parser_helper::convertType(this->getAttribute(tag, "name")),
    this->readHardness(tag),
    physTypes
  };
  return vclassObject;
}

string emulab_extensions_parser::readTypeSlots (const DOMElement* tag)
{
  DOMNodeList* typeSlotsNodes 
    = tag->getElementsByTagName(XStr("emulab:node_type").x());
  if (typeSlotsNodes->getLength() == 0) {
    return "";
  }
  DOMElement* typeSlotsNode = dynamic_cast<DOMElement*>(typeSlotsNodes->item(0));
  return (this->getAttribute(typeSlotsNode, "type_slots"));
}

bool emulab_extensions_parser::readStaticType (const DOMElement* tag)
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

string 
emulab_extensions_parser::readFixedInterface(const DOMElement* tag,bool& fixed)
{
  string rv = "";
  DOMNodeList* fixIfaces 
    = tag->getElementsByTagName(XStr("emulab:fixedinterface").x());
  fixed = (fixIfaces->getLength() == 1);
  if (fixed) {
    DOMElement* fixedIface = dynamic_cast<DOMElement*>(fixIfaces->item(0));
    rv = this->getAttribute(fixedIface, "name");
  }
  return rv;
}

string
emulab_extensions_parser::readShortInterfaceName (const DOMElement* tag,
						  bool& hasShortName)
{
  string rv = "";
  DOMNodeList* interfaces 
    = tag->getElementsByTagName(XStr("emulab:interface").x());
  hasShortName = (interfaces->getLength() == 1);
  if (hasShortName) {
    DOMElement* interface = dynamic_cast<DOMElement*>(interfaces->item(0));
    rv = this->getAttribute(interface, "name");
  }
  return rv;
}

string emulab_extensions_parser::readSubnodeOf (const DOMElement* tag, 
						bool& isSubnode)
{
  string rv = "";
  DOMNodeList* subnodes
    = tag->getElementsByTagName(XStr("emulab:subnode_of").x());
  isSubnode = (subnodes->getLength() > 0);
  if (isSubnode) {
    DOMElement* subnode = dynamic_cast<DOMElement*>(subnodes->item(0));
    rv = this->getAttribute(subnode, "parent");
  }
  return rv;
}

bool emulab_extensions_parser::readDisallowTrivialMix (const DOMElement* tag) 
{
  DOMNodeList* trivialMix 
    = tag->getElementsByTagName(XStr("emulab:disallow_trivial_mix").x());
  return (trivialMix->getLength() > 0);
}

bool emulab_extensions_parser::readUnique (const DOMElement* tag) 
{
  DOMNodeList* uniques = tag->getElementsByTagName(XStr("emulab:unique").x());
  return (uniques->getLength() > 0);
}

int emulab_extensions_parser::readTrivialBandwidth(const DOMElement* tag,
						   bool& hasTrivialBw) 
{
  int trivialBw = 0;
  DOMNodeList* bws 
    = tag->getElementsByTagName(XStr("emulab:trivial_bandwidth").x());
  hasTrivialBw = (bws->getLength() > 0);
  if (hasTrivialBw) {
    trivialBw =(int)this->stringToNum(this->getAttribute
				      (dynamic_cast<DOMElement*>(bws->item(0)),
				       "value"));
  }
  return trivialBw;
}

string emulab_extensions_parser::readHintTo (const DOMElement* tag, 
					     bool& hasHint)
{
  string hint = "";
  DOMNodeList* hints = tag->getElementsByTagName(XStr("emulab:hint_to").x());
  hasHint = (hints->getLength() > 0);
  if (hasHint) {
    hint = this->getAttribute(dynamic_cast<DOMElement*>(hints->item(0)), 
			      "value");
  }
  return hint;
}

bool emulab_extensions_parser::readNoDelay (const DOMElement* tag)
{
  DOMNodeList* nodelays= tag->getElementsByTagName(XStr("emulab:nodelay").x());
  return (nodelays->getLength() > 0);
}

bool emulab_extensions_parser::readTrivialOk (const DOMElement* tag)
{
  DOMNodeList* trivial_oks
    = tag->getElementsByTagName(XStr("emulab:trivial_ok").x());
  return (trivial_oks->getLength() > 0);
}

bool emulab_extensions_parser::readMultiplexOk (const DOMElement* tag)
{
  DOMNodeList* multiplexOks
    = tag->getElementsByTagName(XStr("emulab:multiplex_ok").x());
  return (multiplexOks->getLength() > 0);
}

struct policy emulab_extensions_parser::readPolicy(const DOMElement* tag) 
{
  struct policy policy = {
    this->getAttribute(tag, "type"),
    this->getAttribute(tag, "name"),
    this->getAttribute(tag, "limit")
  };
  return policy;
}

vector<struct policy> 
emulab_extensions_parser::readPolicies (const DOMElement* tag, int& count)
{
  vector<struct policy> rv;
  DOMNodeList* policies = tag->getElementsByTagName(XStr("emulab:policy").x());
  count = policies->getLength();
  for (int i = 0; i < count; i++) {
    rv.push_back
      (this->readPolicy(dynamic_cast<DOMElement*>(policies->item(0))));
  }
  return rv;
}

#endif // WITH_XML
