/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2010 University of Utah and the Flux Group.
 * All rights reserved.
 */

/*
 * Header for RSPEC parser files
 */

# ifdef WITH_XML

#ifndef __RSPEC_PARSER__
#define __RSPEC_PARSER__

#include <list>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <xercesc/dom/DOM.hpp>
#include "rspec_parser_helper.h"
#include "emulab_extensions_parser.h"

#define RSPEC_TYPE_ADVT 0
#define RSPEC_TYPE_REQ 1

#define RSPEC_ERROR_BAD_IFACE_COUNT -1
#define RSPEC_ERROR_UNSEEN_NODEIFACE_SRC -2
#define RSPEC_ERROR_UNSEEN_NODEIFACE_DST -3

struct node_type 
{
  std::string typeName;
  int typeSlots;
  bool isStatic;
};

struct link_type
{
  std::string name;
  std::string typeName;
};

struct link_characteristics
{
  int bandwidth;
  int latency;
  double packetLoss;
};

struct link_interface
{
  std::string virtualNodeId;
  std::string virtualIfaceId;
  std::string physicalNodeId;
  std::string physicalIfaceId;
};

struct node_interface
{
  std::string componentId;
  std::string clientId;
};

class rspec_parser : public rspec_parser_helper
{
 private:

 protected:
  // Extensions parser object
  rspec_emulab_extension::emulab_extensions_parser* emulabExtensions;
  
  int rspecType; 
  std::set< std::pair<std::string, std::string> >ifacesSeen;
  struct link_interface getIface (const xercesc::DOMElement*);
  
 public:
  // Constructors and destructors
  rspec_parser(int type);
  ~rspec_parser();
  
  // Common functions
  virtual std::string readPhysicalId (const xercesc::DOMElement*, bool&);
  virtual std::string readVirtualId (const xercesc::DOMElement*, bool&);
  virtual std::string readComponentManagerId (const xercesc::DOMElement*,
					      bool&);
  virtual std::string readVirtualizationType (const xercesc::DOMElement*,
					      bool&);
  
  // Functions for nodes
  virtual std::vector<std::string>readLocation(const xercesc::DOMElement*, 
					       int&);
  virtual std::vector<struct node_type> 
    readNodeTypes (const xercesc::DOMElement*,
		   int&, int unlimitedSlots=1000);
  
  // Reads subnode tag, if present
  virtual std::string readSubnodeOf (const xercesc::DOMElement*, bool&);
  
  // Reads the exclusive tag if present
  virtual std::string readExclusive (const xercesc::DOMElement*, bool&);
  
  // Reads the available tag, if present
  virtual std::string readAvailable (const xercesc::DOMElement*, bool&);

  // Functions for links
  virtual struct link_characteristics 
    readLinkCharacteristics (const xercesc::DOMElement*, 
			     int&,
			     int defaultBandwidth = -1, 
			     int unlimitedBandwidth = -1);
  // Reads the interfaces on a link		
  virtual std::vector< struct link_interface >
    readLinkInterface (const xercesc::DOMElement*, int&);
  
  // Reads the link types
  virtual std::vector< struct link_type >
    readLinkTypes (const xercesc::DOMElement*, int&);
  
  // Reads all the interfaces on a node
  // Returns the number of interfaces found. 
  // This only populates the internal data structures of the object.
  // Ideally, this ought to be done automatically,
  // but there doesn't seem to be a clean way of making it happen.
  virtual std::map< std::pair<std::string, std::string>, 
    std::pair<std::string, std::string> >
    readInterfacesOnNode (const xercesc::DOMElement*, bool&);

  virtual std::vector<struct rspec_emulab_extension::type_limit> 
    readTypeLimits (const xercesc::DOMElement*, int&);
};


#endif

#endif
