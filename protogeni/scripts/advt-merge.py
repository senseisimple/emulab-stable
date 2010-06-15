#! /usr/bin/env python

#
# EMULAB-COPYRIGHT
# Copyright (c) 2010 University of Utah and the Flux Group.
# All rights reserved.
#

#
# Merges two RSPEC advertisements
#

import datetime
import os
import sys 
import time
import xml.dom.minidom

CREATE_NEW_DESTINATION = 1
MERGE_INTO_EXISTING = 2

SUCCESS = 0
ERROR_INCONSISTENT_LINK_DATA = -1
ERROR_INVALID_INPUT = -2
ERROR_INVALID_OUTPUT = -3

TBROOT = "/home/tarunp/git/emulab-devel"
PGENI_DOMAIN = "@PROTOGENI_DOMAIN@"
# A few other variables also ought to be added
# RSPEC_VER = "@RSPEC_VERSION@"

PGENI_ROOT = TBROOT + "/protogeni"
PATH_TO_SCHEMA = PGENI_ROOT + "/rspec/rspec-ad.xsd"

######## XXX: THIS HAS TO BE CHANGED! #########
#
# At some point, it should be able to use PGENI_DOMAIN and RSPEC_VER
XMLNS = "http://www.protogeni.net/resources/rspec/0.1"
# ------------------------------------------------------------------

DATETIME_FMT = '%Y-%m-%dT%H:%M:%S'


def printMessageAndReturnError (returnCode):
	if (returnCode == 0):
		print("Files merged successfully (" + str(returnCode) + ")")
	else:
		print("File merging failed (" + str(returnCode) + ")")
	sys.exit(returnCode)

# Returns the earliest date as a string from among the given datetimes
# ---
# String getEarliestDateTimes ([datetime])
def getEarliestDateTime (dateTimes):
	maxDateTime = datetime.datetime(datetime.MAXYEAR, 12, 31, 23, 59, 59, 999999)
	earliestDateTime = maxDateTime
	# We can't use a comparison operator directly between two datetime objects
	# but we can subtract two datetimes and compare them against a resulting
	# timedelta object. datetime.timedelta() returns 
	zeroDateTime = datetime.timedelta()
	for dt in dateTimes:
		if (dt - earliestDateTime < zeroDateTime):
			earliestDateTime = dt
	# This can realistically happen in only one case, the input list is empty
	if (earliestDateTime == maxDateTime):
		return ""
	return earliestDateTime.strftime(DATETIME_FMT)

# Returns the current date and time in international format as a string
def getCurrentDateTime ():
	return datetime.datetime.now().strftime(DATETIME_FMT)

# Returns the command to execute to validate the XML input
# ---
# String getValidationCmd (String)
def getValidationCmd (fileToValidate):
	return "xmllint --noout --schema " + PATH_TO_SCHEMA + " " + fileToValidate

# Returns the name of the uuid attribute, either (prefix_uuid, prefix_urn)
# ---
# String getUUIDAttr (xml.dom.Element, String)
def getUUIDAttr (element, prefix):
	if element.hasAttribute(prefix + "_urn"):
		return prefix + "_urn" 
	return prefix + "_uuid"

# Returns the name of the uuid attribute (either component_uuid, component_urn) 
# ---
# String getUUID (xml.dom.Element, String, String)
def getUUID (element, prefix, default=""):
	rv = element.getAttribute(getUUIDAttr(element, prefix))
	if rv != default:
		return rv
	return default

# Returns the contents of the child text node of element
# XXX: This will not return the expected value if the first child text node is a newline
# ---
# String getDataField (xml.dom.Element)
def getDataField (element):
	# Element could be a single element or null
	if (element == None):
		return ""
	# XXX: I don't like having to depend on the childNodes list to get the
	# value of the text node, but I don't see any reasonable alternative
	return (element.childNodes[0].nodeValue.strip())

# Returns the child node elementName of parent
# ---
# xml.dom.Element getElement (xml.dom.Element, String)
def getElement (parent, elementName):
	elements = parent.getElementsByTagName(elementName)
	if (len(elements) == 0):
		return None
	return elements[0]

# Boolean compareFields (xml.dom.Element, xml.dom.Element, String)
def compareFields (link1, link2, fieldName):
	link1FieldValue = getDataField(getElement(link1, fieldName))
	link2FieldValue = getDataField(getElement(link2, fieldName))
	if (link1FieldValue == link2FieldValue):
		return True
	return False

# Boolean compareCharacteristics (xml.dom.Element, xml.dom.Element)
def compareCharacteristics (link1, link2):
	
	if (compareFields(link1, link2, "bandwidth")
		and compareFields(link1, link2, "latency")
		and compareFields(link1, link2, "packet_loss") == True):
		return True
	return False
	
# Integer mergeRSpecs (String, String, Integer)
def mergeRSpecs(destFileName, srcFileNames, destFileAction):
	
	xmlDocument = xml.dom.minidom.Document()

	# This dictionary keeps track of the links that have been seen so far
	dLinks = {}
	# This dictionary keeps track of the external references that have been seen so far
	dExternal_refs = {}
	# This dictionary keeps track of all the nodes that have been seen so far
	dNodes = {}

	# This list keeps track of all the expiration times on the input files
	# Eventually, the expiration time of the merged advertisement,
	# will be the earliest among these.
	expirationTimes = []

	if (destFileAction == CREATE_NEW_DESTINATION):
		root = xmlDocument.createElement("rspec")
		root.setAttribute ("type", "advertisement")
		root.setAttribute ("generated", getCurrentDateTime())
		root.setAttribute ("xmlns", XMLNS)
	
	elif (destFileAction == MERGE_INTO_EXISTING):
		xmlDocument = xml.dom.minidom.parse(destFileName)
		
		# We know that there will be only one root 
		root = xmlDocument.getElementsByTagName("rspec")[0]
		
		expirationTime = root.getAttribute("valid_until")
		if (expirationTime != ""):
			expirationTimes.append(time.strptime(expirationTime, DATETIME_FMT))
		
		links = root.getElementsByTagName("link")
		for link in links:
			component_uuid = getUUID(link, "component")
			if not dLinks.has_key(component_uuid):                   
				dLinks[link.getAttribute(component_uuid)] = link
			else:
				if (not compareCharacteristics(link, dLinks.get(component_uuid))):
					print "ERROR: The links are not consistent. Mismatch found in " + component_uuid
					return ERROR_INCONSISTENT_LINK_DATA
				
		nodes = xmlDocument.getElementsByTagName("node")
		for node in nodes:
			dNodes[getUUID(node, "component")] = node
			#root.appendChild(node)
			
		refs = xmlDocument.getElementsByTagName("external_ref")
		for ref in refs:
			ref_key = getUUID(ref, "component_node")
			dExternal_refs[ref_key] = ref

	else: # This should never happen
		print "ERROR: Something bad happened. An invalid parameter was passed" 
		return False
	
	xmlDocument.appendChild(root)
	for srcFileName in srcFileNames:
		print "Opening file " + srcFileName
		srcXmlDocument = xml.dom.minidom.parse(srcFileName)

		srcRoot = srcXmlDocument.getElementsByTagName("rspec")[0]
		expirationTime = srcRoot.getAttribute("valid_until")
		if (expirationTime != ""):
			expirationTimes.append(time.strptime(expirationTime, DATETIME_FMT))

		nodes = srcXmlDocument.getElementsByTagName("node")
		print "Found " + str(len(nodes)) + " nodes"
		for node in nodes:
			component_uuid = getUUID(node, "component")
			if (dExternal_refs.has_key(component_uuid)):
				del dExternal_refs[component_uuid]
			dNodes[component_uuid] = node
			root.appendChild(node)
		
		refs = srcXmlDocument.getElementsByTagName("external_ref")
		for ref in refs:
			ref_key = getUUID(ref, "component_node")
			dExternal_refs[ref_key] = ref
		
		links = srcXmlDocument.getElementsByTagName("link")
		for link in links:
			component_uuid = getUUID(link, "component")
			if not dLinks.has_key(component_uuid):
				dLinks[component_uuid] = link
			else:
				if(not compareCharacteristics(link, dLinks.get(component_uuid))):
					print "ERROR: The links are not consistent. Mismatch found in " + component_uuid
					return ERROR_INCONSISTENT_LINK_DATA
			root.appendChild(link)

	for key in dExternal_refs.keys():
		root.appendChild(dExternal_refs[key])
		
	expirationTime = getEarliestDateTime(expirationTimes)
	if (expirationTime != ""):
		root.setAttribute("valid_until", expirationTime)
		
	print "Writing to " + destFileName
	rspecFile = open (destFileName, "w")
	# -----------------------------------------------
	# Use this to write a prettily-formatted XML file
	# xmlDocument.writexml (rspecFile,"\t","\t","\n") 
	# ***********************************************
	# Use this to actually do work. 
	# Xerces has trouble reading prettily formatted XML files. It includes the tab and newline characters as part of the element names
	xmlDocument.writexml (rspecFile)
	# -----------------------------------------------
	rspecFile.close ()
	return SUCCESS

def main ():
	
	rv = False
	
	helpString = "[USAGE] advt-merge.py <destination_file> <source_file> [<source_file> ...]\nIf <desination_file> does not exist, it will be created. If destination file does exist, all the source files will be merged into it"
	
	# At least two files should be specified
	if len(sys.argv) < 3:
		print helpString
		return 
	else:
		# Validate all rspecs
		destFileName = sys.argv[1]
		if (os.path.exists(destFileName)):
			returnCode = os.system(getValidationCmd(destFileName))
			if (returnCode != SUCCESS):
				print "ERROR: " + destFileName + " does not validate"
				return printMessageAndReturnError(ERROR_INVALID_INPUT)
			destFileAction = MERGE_INTO_EXISTING
		else:
			destFileAction = CREATE_NEW_DESTINATION
			
		srcFileNames = []
		for index in range(2, len(sys.argv)):
			returnCode = os.system(getValidationCmd(sys.argv[index]))
			# A non-zero return value indicates that the command failed
			if (returnCode != SUCCESS):
				print "ERROR: " + sys.argv[index] + " does not validate"
				return printMessageAndReturnError(ERROR_INVALID_INPUT)
			srcFileNames.append(sys.argv[index])
			
		# Merge the advertisements
		rv = mergeRSpecs(destFileName, srcFileNames, destFileAction)
		
		# Check if the destination validates. It should, but just in case
		returnCode = os.system(getValidationCmd(destFileName))
		if (returnCode != SUCCESS):
			print "ERROR: " + destFileName + " does not validate"
			return printMessageAndReturnError(ERROR_INVALID_OUTPUT)
		
	return printMessageAndReturnError(rv)
		
# Call the main function if this is called from the command line
if __name__ == "__main__":
	main ()
	