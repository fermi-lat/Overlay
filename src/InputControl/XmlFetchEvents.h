/** @file XmlFetchEvents.h

    @brief declaration of the FetchEvents class

$Header: /nfs/slac/g/glast/ground/cvs/Overlay/src/XmlFetchEvents.h,v 1.1.1.1 2008/10/15 15:14:31 usher Exp $

*/

#ifndef XmlFetchEvents_h
#define XmlFetchEvents_h


#include "Overlay/IFetchEvents.h" 

#include <vector>
#include <xercesc/dom/DOMElement.hpp>

#include "xmlBase/XmlParser.h"


/** @class XmlFetchEvents
    @brief manage the retrieval of events using some specified parameter(s) from an XML file
    @author Heather Kelly heather625@gmail.com


*/
class XmlFetchEvents : public IFetchEvents
{
public:

    XmlFetchEvents(const std::string& xmlFile, const std::string& param);

    virtual ~XmlFetchEvents();

    double getAttributeValue(const std::string& elemName, double binVal);

    std::vector<std::string> getFiles(double binVal, bool verbose=false);

    virtual double minValFullRange()    const{return m_minval;}      ///< return minimum value allowed
    virtual double maxValFullRange()    const{return m_maxval;}      ///< return maximum value allowed

    virtual double minVal()             const{return m_lastBinMin;}  ///< return minimum value in current range
    virtual double maxVal()             const{return m_lastBinMax;}  ///< return maximum value in current range

    virtual const std::string& name()   const {return m_name;}

    virtual std::string getTreeName()   const {return m_treeName;}
    virtual std::string getBranchName() const {return m_branchName;}

private:

    /// actually handles the XML reading
    xmlBase::XmlParser m_xmlParser;

    /// stores all children of the top element in the XML file
    std::vector<xmlBase::DOMElement* > m_children;  

    /// children that contain the parameter we're interested in
    std::vector<xmlBase::DOMElement*> m_paramChildren; 

    /// children that contain the bins for the parameter we're interested in
    std::vector<xmlBase::DOMElement*> m_binChildren;

    /// Store the most recently accessed DOMElement in the m_binChildren vector;
    int         m_lastBinIndex;
    double      m_lastBinMin;
    double      m_lastBinMax;
    double      m_minval;
    double      m_maxval;
    std::string m_name;
    std::string m_treeName;
    std::string m_branchName;

};


#endif
