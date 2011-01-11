/*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2008-2010 University of Utah and the Flux Group.
 * All rights reserved.
 */

#ifdef WITH_XML

#include "xmlhelpers.h"
#include "xstr.h"
#include <string>
#include <vector>


const XMLCh* getChildValue(const DOMElement *tag, const char *name) {
    DOMNodeList *list = tag->getElementsByTagName(XStr(name).x());
    if (list->getLength() != 1) {
        throw "Incorrect number of child elements in getChildValue()";
    } else {
	return dynamic_cast<DOMElement*>(list->item(0))->getFirstChild()->getNodeValue();
    }
}

/* This will only work if there is only one element with that tag within the root
 * It is the callers responsibility to ensure that this is the case before calling this function
 */ 
void setChildValue(const DOMElement* parent, const char* tag, const char* value)
{
	DOMElement* ele = getElementByTagName (parent, tag);
	if (ele == NULL)
		throw "Error finding tag in parent in setChildValue ()";
	else
		ele->getFirstChild()->setNodeValue (XStr(value).x());	
}

DOMElement* getNodeByName (const DOMElement *root, const char* name)
{
	DOMNodeList *list = root -> getElementsByTagName(XStr("node").x());
	for (unsigned int i = 0; i < list -> getLength() ; ++i)
	{	
		DOMElement *ele = dynamic_cast<DOMElement*>(list->item(i)); 
		if (strcmp(XStr(ele->getAttribute(XStr("name").x())).c(),name) == 0)
			return ele;
	}
	return NULL;
}

DOMElement* getElementByAttributeValue (const DOMElement* root, const char* tag, const char* attribute_name, const char* attribute_value)
{
	DOMNodeList *list = root -> getElementsByTagName(XStr(tag).x());
	for (unsigned int i = 0; i < list -> getLength() ; ++i)
	{	
		DOMElement *ele = dynamic_cast<DOMElement*>(list->item(i)); 
		if (strcmp(XStr(ele->getAttribute(XStr(attribute_name).x())).c(),attribute_value) == 0)
			return ele;
	}
	return NULL;
}

DOMElement* getElementByAttributeValue (vector<const DOMElement*> roots, const char* tag, const char* attribute_name, const char* attribute_value)
{
	for (vector<const DOMElement*>::iterator it = roots.begin(); it != roots.end(); ++it)
	{	
		DOMNodeList *list = (*it)->getElementsByTagName(XStr(tag).x());
		for (unsigned int i = 0; i < list -> getLength() ; ++i)
		{
			DOMElement *ele = dynamic_cast<DOMElement*>(list->item(i)); 
			if (strcmp(XStr(ele->getAttribute(XStr(attribute_name).x())).c(),attribute_value) == 0)
				return ele;
		}
	}
	return NULL;
}

vector<const DOMElement*> getElementsByAttributeValue (const DOMElement* root, const char* tag, const char* attribute_name, const char* attribute_value)
{
	DOMNodeList *list = root -> getElementsByTagName(XStr(tag).x());
	vector<const DOMElement*> elements;
	for (unsigned int i = 0; i < list -> getLength() ; ++i)
	{	
		DOMElement *ele = dynamic_cast<DOMElement*>(list->item(i)); 
		if (strcmp(XStr(ele->getAttribute(XStr(attribute_name).x())).c(),attribute_value) == 0)
			elements.push_back(ele);
	}
	return elements;
}

vector<DOMElement*> getElementsHavingAttribute(const DOMElement* root, const char* tag, const char* attribute_name)
{
	DOMNodeList* list = root->getElementsByTagName(XStr(tag).x());
	vector<DOMElement*> elements;
	for (unsigned int i = 0; i < list->getLength(); ++i)
	{
		DOMElement* ele = dynamic_cast<DOMElement*>(list->item(i));
		if (ele->hasAttribute(XStr(attribute_name).x()))
			elements.push_back(ele);
	}
	return elements;
}

/* This will only work if there is only one element with that tag within the root
 * It is the callers responsibility to ensure that this is the case before calling this function
 */ 
DOMElement* getElementByTagName (const DOMElement* root, const char* tag)
{
	DOMNodeList *list = root -> getElementsByTagName(XStr(tag).x());
	if (list->getLength() != 1) {
		throw "More than one child with this tag in getElementByTagName()";
	} else {
		return dynamic_cast<DOMElement*>(list->item(0));
	}
}

/* Returns the nth interface in a link (can be used for a node as well only if n is 0 */
DOMElement* getNthInterface (const DOMElement* root, int n)
{
	DOMNodeList* interfaces = root->getElementsByTagName(XStr("interface").x());
	if (interfaces->getLength() <= n) {
		throw "Too few interfaces found";
	}
	return (dynamic_cast<DOMElement*>(interfaces->item(n)));	
}

/*
 * TODO: Better error handling
 */
bool hasChildTag(const DOMElement *tag, const char *name) {
    return (tag->getElementsByTagName(XStr(name).x())->getLength() > 0);
}

int parse_fds_xml(const DOMElement *tag, node_fd_set *fd_set) {
    DOMNodeList *fds = tag->getElementsByTagName(XStr("fd").x());
    for (int i = 0; i < fds->getLength(); i++) 
	{
		DOMElement *elt = dynamic_cast<DOMElement*>(fds->item(i));
	
		XStr fd_name (elt->getAttribute(XStr("fd_name").x()));
		XStr fd_weight (elt->getAttribute(XStr("fd_weight").x()));
	
		bool violatable = elt->hasAttribute(XStr("violatable").x());
		featuredesire::fd_type fd_type;
		if (elt->hasAttribute(XStr("local_operator").x())) 
		{
			/*
			* Right now, there is only one type of local feature
			*/
			fd_type = featuredesire::FD_TYPE_LOCAL_ADDITIVE;
		} 
		else if (elt->hasAttribute(XStr("global_operator").x())) 
		{	
			XStr fd_operator(elt->getAttribute(XStr("global_operator").x()));
			if (fd_operator == "OnceOnly") {
				fd_type = featuredesire::FD_TYPE_GLOBAL_ONE_IS_OKAY;
			} else {
				fd_type = featuredesire::FD_TYPE_GLOBAL_MORE_THAN_ONE;
			}
		} 
		else {
			fd_type = featuredesire::FD_TYPE_NORMAL;
		}
	
		fd_set->push_front(tb_node_featuredesire(fd_name.c(), fd_weight.d(),
						 violatable, fd_type));
	
    }
    return fds->getLength();
}

/*
 * TODO: Better error handling
 */
node_interface_pair parse_interface_xml(const DOMElement *tag) {
	const XMLCh* node_name = tag->getAttribute(XStr("node_name").x());
	const XMLCh* interface_name = tag->getAttribute(XStr("interface_name").x());
	
	node_interface_pair rv(node_name,interface_name);
	return rv;
}

/* Parse the features and desires on a virtual node */
int parse_fds_vnode_xml (const DOMElement *tag, node_fd_set *fd_set)
{
	DOMNodeList *fds = tag->getElementsByTagName(XStr("fd").x());
    for (int i = 0; i < fds->getLength(); i++) 
    {
		DOMElement *elt = dynamic_cast<DOMElement*>(fds->item(i));
		XStr fd_name (elt->getAttribute(XStr("fd_name").x()));
		XStr fd_weight (elt->getAttribute(XStr("fd_weight").x()));
	
		bool violatable = elt->hasAttribute(XStr("violatable").x());
		featuredesire::fd_type fd_type;
		if (elt->hasAttribute(XStr("local_operator").x())) {
			/*
			* Right now, there is only one type of local feature
			*/
			fd_type = featuredesire::FD_TYPE_LOCAL_ADDITIVE;
		} else if (elt->hasAttribute(XStr("global_operator").x())) {	
			XStr fd_operator(elt->getAttribute(XStr("global_operator").x()));
			if (fd_operator == "OnceOnly") {
				fd_type = featuredesire::FD_TYPE_GLOBAL_ONE_IS_OKAY;
			} else {
				fd_type = featuredesire::FD_TYPE_GLOBAL_MORE_THAN_ONE;
			}
		} else {
			fd_type = featuredesire::FD_TYPE_NORMAL;
		}
		
		tb_node_featuredesire node_fd (fd_name.c(), fd_weight.d(), violatable, fd_type);
		node_fd.add_desire_user(fd_weight.d());
		
		fd_set->push_front(node_fd);
    }
    return fds->getLength();
}

interface_spec parse_interface_rspec_xml(const DOMElement *tag) 
{
	interface_spec rv =
	{	
		string(XStr(tag->getAttribute(XStr("virtual_node_id").x())).c()),
		string(XStr(tag->getAttribute(XStr("virtual_interface_id").x())).c()),
		string(XStr(find_urn(tag, "component_node"))),
		string(XStr(tag->getAttribute(XStr("component_interface_id").x())).c())
	};
    return rv;
}

component_spec parse_component_spec (const DOMElement *elt) {
	component_spec rv = {string(""), string(""), string(""), string("")};
	rv.component_manager_uuid = string(XStr(find_urn(elt, "component_manager")));
	if (elt->hasAttribute(XStr("component_name").x()))
		rv.component_name = string (XStr(elt->getAttribute(XStr("component_name").x())).c());
	rv.component_uuid = string(XStr(find_urn(elt, "component")));
	if (elt->hasAttribute(XStr("sliver_uuid").x()))
		rv.sliver_uuid = string (XStr(elt->getAttribute(XStr("sliver_uuid").x())).c());
	return rv;
}

XMLCh const * find_urn(const xercesc::DOMElement* element,
                       string const & prefix)
{
  string uuid = prefix + "_uuid";
  string urn = prefix + "_urn";
  if (element->hasAttribute(XStr(uuid.c_str()).x()))
  {
    return element->getAttribute(XStr(uuid.c_str()).x());
  }
  else //if (element->hasAttribute(XStr(urn.c_str()).x()))
  {
    return element->getAttribute(XStr(urn.c_str()).x());
  }
}

// Check if the component spec is present. We check if the aggregate UUID and the component UUID are both present
bool hasComponentSpec (DOMElement* elt)
{
  return ((elt->hasAttribute(XStr("component_manager_uuid").x())
           || elt->hasAttribute(XStr("component_manager_urn").x()))
          && (elt->hasAttribute(XStr("component_uuid").x())
              || elt->hasAttribute(XStr("component_urn").x())));
}

#endif
