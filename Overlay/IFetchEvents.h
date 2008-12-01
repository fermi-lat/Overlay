/** @file IFetchEvents.h

    @brief declaration of the IFetchEvents class

$Header: /nfs/slac/g/glast/ground/cvs/Overlay/src/IFetchEvents.h,v 1.1.1.1 2008/10/15 15:14:30 usher Exp $

*/

#ifndef IFetchEvents_h
#define IFetchEvents_h

#include <vector>
#include <string>

/** @class IFetchEvents
    @brief manage the retrieval of events using some specified parameter(s)
    @author Heather Kelly heather625@gmail.com


*/
class IFetchEvents
{
public:

    /** @brief ctor
        @param param parameter used for retrieving data from dataStore
        @param dataStore name of the file containing the event store
    */
    IFetchEvents(const std::string& dataStore, const std::string& param) : 
        m_dataStore(dataStore), m_param(param) {};

    virtual ~IFetchEvents() { };

    /// Returns the value of the requested elemName, based on the bin associated with binVal
    virtual double getAttributeValue(const std::string& elemName, double binVal) = 0;

    /// adds TTree's to a TChain
    virtual std::vector<std::string> getFiles(double binVal, bool verbose=false) = 0;


    virtual double minValFullRange()    const {return -1e30;}  ///< return minimum value allowed
    virtual double maxValFullRange()    const {return +1e30;}  ///< return maximum value allowed

    virtual double minVal()             const {return -1e30; } ///< return minimum value in current range
    virtual double maxVal()             const {return +1e30; } ///< return maximum value in current range

    virtual const std::string& name()   const = 0;

    /// test if value is valid in current range
    virtual bool isCurrent(double val)  const {return val>=minVal() && val< maxVal();}
    virtual bool isValid(double val)    const {return val>=minValFullRange() && val< maxValFullRange();}

    /// Retrieve the Tree and Branch names
    virtual std::string getTreeName()   const = 0;
    virtual std::string getBranchName() const = 0;

private:
    friend class XmlFetchEvents;

    /// path or file name associated with the dataStore which contains the information we wish to extract
    std::string m_dataStore; 
    /// name of the variable that we will use to search the dataStore
    std::string m_param;

};


#endif
