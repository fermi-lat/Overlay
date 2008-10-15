/**  @file XmlFetchEvents.cxx
@brief implementation of class XmlFetchEvents

$Header: /nfs/slac/g/glast/ground/cvs/Interleave/src/XmlFetchEvents.cxx,v 1.24 2008/03/27 15:23:25 heather Exp $  
*/

#include "XmlFetchEvents.h"
#include "xmlBase/Dom.h"
#include "facilities/Util.h"
#include <xercesc/dom/DOMNodeList.hpp>

#include <stdexcept>
#include <sstream>
#include <cmath>
#include <cassert>
#include <ctime>


using XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument;
using XERCES_CPP_NAMESPACE_QUALIFIER DOMElement;
using xmlBase::Dom;

XmlFetchEvents::XmlFetchEvents(const std::string& xmlFile, const std::string& param)
: IFetchEvents(xmlFile,param),
  m_minval(+1e30),
  m_maxval(-1e30)
{

    XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument* doc;
    m_xmlParser.doSchema(true);

    try {
        doc = m_xmlParser.parse(m_dataStore.c_str());
    } catch (std::exception ex) {
        std::cerr << ex.what();
        std::cerr.flush();
        throw ex;
    }
    if (doc == NULL) {
        throw std::runtime_error("Archive does not have proper myHost.xml file");
    }

    // retrieve the top-level element in our XML 
    DOMElement* elem = doc->getDocumentElement();
    //xmlBase::Dom::prettyPrintElement(elem, std::cout, "");

    // Assuming "source" is the most basic element in our XML file
    xmlBase::Dom::getChildrenByTagName(elem, "source", m_children);
    m_paramChildren.clear();
    m_paramChildren.reserve(m_children.size()); // reserve enough space for all possible children

    // Save all children elements that pertain to our search parameter
    std::vector<DOMElement*>::const_iterator domElemIt; 
    for (domElemIt = m_children.begin(); domElemIt != m_children.end(); domElemIt++) { 
        std::string typeAttr = xmlBase::Dom::getAttribute(*domElemIt, "type");
        if (typeAttr == m_param) {
            // found one save it
            m_name = xmlBase::Dom::getAttribute(*domElemIt, "name");
            m_paramChildren.push_back(*domElemIt);
            double minval(facilities::Util::stringToDouble(xmlBase::Dom::getAttribute(*domElemIt, "rangeMin"))),
                maxval(facilities::Util::stringToDouble(xmlBase::Dom::getAttribute(*domElemIt, "rangeMax")));
            if(minval< m_minval) m_minval = minval;
            if(maxval>m_maxval) m_maxval = maxval;
        }
    }

    // Save all the "bin" elements associated with our parameter
    m_binChildren.clear();
    for(domElemIt=m_paramChildren.begin(); domElemIt != m_paramChildren.end(); domElemIt++) {
        // tell method not to clear our binChildren vector by setting last param to false
        xmlBase::Dom::getChildrenByTagName(*domElemIt, "bin", m_binChildren, false);
    }

    if( m_binChildren.empty() ){
        throw std::invalid_argument("XmlFetchEvents: did not find entries for "+param);
    }
    m_lastBinIndex = -1;
    m_lastBinMin   = -99999.;
    m_lastBinMax   = -99999.;
}


XmlFetchEvents::~XmlFetchEvents()
{
    m_children.clear();
    m_paramChildren.clear();
    m_binChildren.clear();
}

double XmlFetchEvents::getAttributeValue(const std::string& elemName, double binVal) 
{
    /// Purpose and Method:  Extracts any attribute associated with the name elemName from
    /// our vector of bins.  Uses the binVal to determine which bin to use.
    // Check to see if we're accessing the same bin we have previously
    double retVal = -99999.;

    if (m_lastBinIndex >= 0) 
    {
        if ((binVal >= m_lastBinMin) && (binVal <= m_lastBinMax)) {
            std::string retValStr = xmlBase::Dom::getAttribute(m_binChildren[m_lastBinIndex], elemName);
            retVal = facilities::Util::stringToDouble(retValStr);
        }
    }
    else
    {
        // Otherwise search the whole vector
        std::vector<DOMElement*>::const_iterator domElemIt; 
        int curIndex = 0;

        for (domElemIt = m_binChildren.begin(); domElemIt != m_binChildren.end(); domElemIt++) 
        { 
            std::string minBinStr = xmlBase::Dom::getAttribute(*domElemIt, "min");
            std::string maxBinStr = xmlBase::Dom::getAttribute(*domElemIt, "max");
            double minBin = facilities::Util::stringToDouble(minBinStr);
            double maxBin = facilities::Util::stringToDouble(maxBinStr);
            if ((binVal >= minBin) && (binVal <= maxBin)) 
            {
                m_lastBinIndex = curIndex;
                m_lastBinMin = minBin;
                m_lastBinMax = maxBin;
                std::string retValStr = xmlBase::Dom::getAttribute(*domElemIt, elemName);
                retVal = facilities::Util::stringToDouble(retValStr);
            }
            ++curIndex;
        }
    }
    
    return retVal;
}


std::vector<std::string> XmlFetchEvents::getFiles(double binVal, bool verbose) 
{
    /// Purpose and Method:  Returns a "fileList" associated with the bin found using binVal.
    /// Returns the input file list if completely successful
    /// Returns a null list for an error

    std::vector<DOMElement*> domElemList;
    domElemList.clear();

    // Check to see if we're accessing the same bin we have previously
    if ((m_lastBinIndex >= 0) && (binVal >= m_lastBinMin) && (binVal <= m_lastBinMax) ) {
            DOMElement* fileListElem = xmlBase::Dom::findFirstChildByName(m_binChildren[m_lastBinIndex], "fileList");
            xmlBase::Dom::getChildrenByTagName(fileListElem, "file", domElemList);
    } else {
        // Otherwise search the whole vector
        std::vector<DOMElement*>::const_iterator domElemIt; 
        int curIndex = 0;

        for (domElemIt = m_binChildren.begin(); domElemIt != m_binChildren.end(); domElemIt++) { 
            std::string minBinStr = xmlBase::Dom::getAttribute(*domElemIt, "min");
            std::string maxBinStr = xmlBase::Dom::getAttribute(*domElemIt, "max");
            double minBin = facilities::Util::stringToDouble(minBinStr);
            double maxBin = facilities::Util::stringToDouble(maxBinStr);
            if ((binVal >= minBin) && (binVal <= maxBin)) {
                m_lastBinIndex = curIndex;
                m_lastBinMin = minBin;
                m_lastBinMax = maxBin;
                DOMElement* fileListElem = xmlBase::Dom::findFirstChildByName(*domElemIt, "fileList");
                xmlBase::Dom::getChildrenByTagName(fileListElem, "file", domElemList);
            }
            ++curIndex;
        }
    }

    // Create the output file list
    std::vector<std::string> fileList;

    for (std::vector<DOMElement*>::iterator domIter = domElemList.begin(); domIter != domElemList.end(); domIter++)
    {
        std::string fileName = xmlBase::Dom::getAttribute(*domIter, "filePath");
        
        fileList.push_back(fileName);

        // Retrieve the tree name
        m_treeName = xmlBase::Dom::getAttribute(*domIter, "treeName");

        // Retrieve the tree name
        m_branchName = xmlBase::Dom::getAttribute(*domIter, "branchName");
    }

    return fileList;
}

