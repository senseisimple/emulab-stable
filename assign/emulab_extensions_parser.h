/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2008, 2009 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifndef __EMULAB_EXTENSIONS_PARSER_H__
#define __EMULAB_EXTENSIONS_PARSER_H__

#ifdef WITH_XML

#include <string>
#include <vector>
#include <xercesc/dom/DOM.hpp>
#include "rspec_parser_helper.h"

namespace rspec_emulab_extension {

#define LOCAL_OPERATOR 0
#define GLOBAL_OPERATOR 1
#define NORMAL_OPERATOR 2
  
#define HARD_VCLASS 0
#define SOFT_VCLASS 1

  struct policy {
    std::string type;
    std::string name;
    std::string limit;
  };
  
  struct emulab_operator {
    std::string op;
    int type;
  };
  
  struct fd 
  {
    std::string fd_name;
    float fd_weight;
    bool violatable;
    struct emulab_operator op;
  };
  
  struct node_flags
  {
    int trivialBandwidth;
    std::string subnodeOf;
    bool unique;
    bool disallowTrivialMix;
  };
  
  struct link_flags
  {
    bool noDelay;
    bool multiplexOk;
    bool trivialOk;
    std::string fixSrcIface;
    std::string fixDstIface;
  };
  
  struct hardness {
    int type;
    float weight;	
  };
  
  struct vclass {
    std::string name;
    struct hardness type;
    std::vector<std::string> physicalTypes;
  };
  
  struct property {
    std::string name;
    std::string value;
    float penalty;
    bool violatable;
    struct emulab_operator op;
  };

  struct type_limit {
    std::string typeClass;
    int typeCount;
  };
  
  class emulab_extensions_parser : public rspec_parser_helper {
  private:
    int type;
    struct emulab_operator readOperator(const xercesc::DOMElement* tag);
    struct hardness readHardness (const xercesc::DOMElement* tag);
    
  public:
    // Constructor
    emulab_extensions_parser(int type) { this->type = type; }
    
    // Functions
    std::vector<struct fd> readFeaturesDesires (const xercesc::DOMElement*, 
						int&);
    struct fd readFeatureDesire (const xercesc::DOMElement* tag);
    struct node_flags readNodeFlag (const xercesc::DOMElement* tag);
    struct link_flags readLinkFlag (const xercesc::DOMElement* tag);
    std::vector<struct property> readProperties
      (const xercesc::DOMElement* elem);
    struct property readProperty (const xercesc::DOMElement* tag);
    std::vector<struct vclass> readVClasses (const xercesc::DOMElement*);
    struct vclass readVClass (const xercesc::DOMElement* tag);
    std::string readAssignedTo (const xercesc::DOMElement* tag);
    std::string readHintTo (const xercesc::DOMElement* tag);
    std::string readTypeSlots (const xercesc::DOMElement* tag);
    bool readStaticType (const xercesc::DOMElement* tag);
    std::vector<struct type_limit> readTypeLimits(const xercesc::DOMElement*,
						  int& count);
    std::string readSubnodeOf (const xercesc::DOMElement* tag, bool&);
    virtual bool readDisallowTrivialMix (const xercesc::DOMElement* tag);
    virtual bool readUnique (const xercesc::DOMElement* tag);
    virtual int readTrivialBandwidth (const xercesc::DOMElement* tag, bool&);
    virtual std::string readHintTo (const xercesc::DOMElement* tag, bool&);
    virtual bool readNoDelay (const xercesc::DOMElement* tag);
    virtual bool readTrivialOk (const xercesc::DOMElement* tag);
    virtual bool readMultiplexOk (const xercesc::DOMElement* tag);
    virtual string readFixedInterface (const xercesc::DOMElement*, bool&);
    virtual string readShortInterfaceName (const xercesc::DOMElement*, bool&);
    virtual struct policy readPolicy (const xercesc::DOMElement*);
    virtual std::vector<struct policy>
      readPolicies (const xercesc::DOMElement*, int& count);
  };
  
} // namespace rspec_emulab_extension

#endif // WITH_XML

#endif // __EMULAB_EXTENSIONS_PARSER_H__
