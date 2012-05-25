// File and Version Information:
//      $Header: /nfs/slac/g/glast/ground/cvs/Overlay/src/DataServices/OverlayCnvSvc.cxx,v 1.3 2011/12/12 20:54:54 heather Exp $
//
// Description:
//      OverlayCnvSvc is the GLAST converter service.
//
// Author(s):
#define OverlayCnvSvc_CPP

#include <iostream>
#include "GaudiKernel/ConversionSvc.h"
#include "GaudiKernel/SvcFactory.h"
#include "GaudiKernel/CnvFactory.h"
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/SmartIF.h"
//#include "GaudiKernel/ICnvManager.h"
#include "GaudiKernel/ISvcLocator.h"
#include "GaudiKernel/IDataProviderSvc.h"
#include "GaudiKernel/IDataManagerSvc.h"
#include "GaudiKernel/DataSvc.h"
#include "GaudiKernel/RegistryEntry.h"
#include "GaudiKernel/GenericAddress.h"

#include "GlastSvc/EventSelector/IGlastCnv.h"

#include "OverlayEvent/EventOverlay.h"
#include "OverlayEvent/AcdOverlay.h"
#include "OverlayEvent/CalOverlay.h"
#include "OverlayEvent/DiagDataOverlay.h"
#include "OverlayEvent/GemOverlay.h"
#include "OverlayEvent/PtOverlay.h"
#include "OverlayEvent/SrcOverlay.h"
#include "OverlayEvent/TkrOverlay.h"

#include <map>
#include <vector>

template <class TYPE> class SvcFactory;

/** @class OverlayCnvSvc
 * @brief GLAST Event Conversion Service which coordinates all of our converters.
 *
 * When a component requests an item not yet available on the TDS, 
 * the OverlayCnvSvc is called to find the appropriiate converter to gain
 * access to the data and put it on the TDS.
 * Based on SICb service written by Markus Frank.
 *
 * $Header: /usr/local/CVS/SLAC/Overlay/src/DataServices/OverlayCnvSvc.cxx,v 1.2 2011/06/29 15:32:00 usher Exp $
 */

class OverlayCnvSvc  : virtual public ConversionSvc	
{
    friend class SvcFactory<OverlayCnvSvc>;

public:
    virtual StatusCode initialize();

    virtual StatusCode finalize();

    virtual StatusCode updateServiceState(IOpaqueAddress* pAddress);

    /// Override inherited queryInterface due to enhanced interface
    virtual StatusCode queryInterface(const InterfaceID& riid, void** ppvInterface);

    /** IAddressCreator implementation: Address creation.
      Create an address using the link infotmation together with
      the triple (database name/container name/object name).

      @param refLink        Reference to abstract link information
      @param dbName         Database name
      @param containerName  Object container name
      @param refpAddress    Opaque address information to retrieve object
      @return               StatusCode indicating SUCCESS or failure
    */
    virtual StatusCode createAddress( long  svc_type,
                                      const CLID& clid,
                                      const std::string* par, 
                                      const unsigned long* ip,
                                      IOpaqueAddress*& refpAddress);

protected:

  OverlayCnvSvc(const std::string& name, ISvcLocator* svc);

  virtual ~OverlayCnvSvc() { };

private:
    typedef std::vector<CLID>                                CLIDvector;

    typedef std::map<std::string, IGlastCnv*>                PathToCnvMap;
    typedef std::map<std::string, std::vector<std::string> > SubPathMap;

    typedef std::map<const CLID,  IGlastCnv*>                CLIDToCnvMap;
    typedef std::map<const CLID,  std::vector<CLID> >        SubCLIDMap;

    CLIDvector          m_clidVector;

    PathToCnvMap        m_pathToCnvMap;
    SubPathMap          m_subPathMap;
    CLIDToCnvMap        m_clidToCnvMap;
    SubCLIDMap          m_subClidMap;
};

#include "GlastSvc/EventSelector/IGlastCnv.h"

// Instantiation of a static factory class used by clients to create
// instances of this service
DECLARE_SERVICE_FACTORY( OverlayCnvSvc );

OverlayCnvSvc::OverlayCnvSvc(const std::string& name, ISvcLocator* svc)
             : ConversionSvc(name, svc, EXCEL_StorageType)              
{
    m_pathToCnvMap.clear();
    m_subPathMap.clear();
    m_clidToCnvMap.clear();
    m_subClidMap.clear();

    // Hardwired list of Class ID's 
    m_clidVector.push_back(Event::EventOverlay::classID());
    m_clidVector.push_back(ObjectVector<Event::AcdOverlay>::classID());
    m_clidVector.push_back(ObjectVector<Event::CalOverlay>::classID());
    m_clidVector.push_back(Event::DiagDataOverlay::classID());
    m_clidVector.push_back(Event::GemOverlay::classID());
    m_clidVector.push_back(Event::PtOverlay::classID());
    m_clidVector.push_back(Event::SrcOverlay::classID());
    m_clidVector.push_back(ObjectVector<Event::TkrOverlay>::classID());
}

StatusCode OverlayCnvSvc::initialize()     
{
    // Purpose and Method:  Setup GLAST's Event Converter Service.
    //   Associate OverlayCnvSvc with the EventDataSvc
    //   Associate the list of known converters with this OverlayCnvSvc.
    MsgStream log(msgSvc(), name());

    StatusCode status = ConversionSvc::initialize();

    // First loop through the hardwired list of objects we need to register converters for
    for(CLIDvector::iterator clidItr = m_clidVector.begin(); clidItr != m_clidVector.end(); clidItr++)
    {
        CLID& clid = *clidItr;

        // This call will register the converter if it is not yet known and
        // then return a pointer to it
        IGlastCnv* glastCnv = dynamic_cast<IGlastCnv*>(converter(clid));

        // Store the pointer to the converter in our map
        m_pathToCnvMap[glastCnv->getPath()] = glastCnv;
        m_clidToCnvMap[glastCnv->objType()] = glastCnv;
    }

    // Loop back through the map to build the list of immediate daughter paths
    for(PathToCnvMap::iterator mapIter = m_pathToCnvMap.begin(); mapIter != m_pathToCnvMap.end(); mapIter++)
    {
        bool               orphan = true;
        const std::string& path   = mapIter->first;
        IGlastCnv*         topCnv = mapIter->second;

        // Another loop through to find the matches
        for(PathToCnvMap::iterator inIter = m_pathToCnvMap.begin(); inIter != m_pathToCnvMap.end(); inIter++)
        {
            // Skip if our self
            if (path == inIter->first) continue;

            // Find the parent path of this path
            std::string prntPath = inIter->first.substr(0, inIter->first.rfind("/"));

            // If a match then add this as a daughter of the path
            if (path == prntPath)
            {
                m_subPathMap[path].push_back(inIter->first);
                m_subClidMap[topCnv->objType()].push_back(inIter->second->objType());
                orphan = false;
            }
        } 
    }
    
    return status;
}

StatusCode OverlayCnvSvc::finalize()     
{
    MsgStream log(msgSvc(), name());

    log << MSG::DEBUG << "Finalizing" << endreq;

    return StatusCode::SUCCESS;
}

/// Update state of the service
StatusCode OverlayCnvSvc::updateServiceState(IOpaqueAddress* pAddress)    
{
    // Initialize for output and return
    MsgStream log(msgSvc(), name());
    StatusCode status = INVALID_ADDRESS;

    // Need a valid opaque address entry to proceed
    if ( 0 != pAddress )   
    {
        status = StatusCode::SUCCESS;

        // Recover class id we are starting with
        const CLID& clID = pAddress->clID();

        // We need to recover the data service that is being using to supply our data
        // Start by getting a pointer to the registry
        IRegistry* registry = pAddress->registry();

        if (!registry) return StatusCode::FAILURE;

        // Caste back to the actual DataSvc...
        DataSvc* dataSvc  = dynamic_cast<DataSvc*>(registry->dataSvc());

        // If caste failed then simply return as if nothing has happened
        if (!dataSvc) return status;

        // Now recover the root path name
        std::string rootPath = dataSvc->rootName();
        
        // From the data service, recover the address manager
//        SmartIF<IDataManagerSvc> iaddrReg(IID_IDataManagerSvc, dataProvider());
//        SmartIF<IDataManagerSvc> iaddrReg(IID_IDataManagerSvc, registry->dataSvc());
        SmartIF<IDataManagerSvc> iaddrReg(registry->dataSvc());

        // Find the list of related converters
        SubCLIDMap::iterator clidIter = m_subClidMap.find(clID);

        // Check to be sure something was found
        if (clidIter != m_subClidMap.end())
        {
            // Loop over the list of daughter converters and activate them
            for(std::vector<CLID>::iterator subIter = clidIter->second.begin(); subIter != clidIter->second.end(); subIter++)
            {
                // Now look up the pointer to the converter
                CLIDToCnvMap::iterator cnvIter  = m_clidToCnvMap.find(*subIter);
                IGlastCnv*             glastCnv = cnvIter->second;

                if (glastCnv)
                {
                    // Avoid self reference, though this should not happen here
                    if (glastCnv->objType() == clID) continue;

                    IOpaqueAddress*   newAddr  = 0;
                    unsigned long     ipars[2] = {0, 0};
                    const std::string spars[2] = {rootPath + glastCnv->getPath(), ""}; 
                    
                    StatusCode ir = addressCreator()->createAddress(EXCEL_StorageType, 
                                                                    glastCnv->objType(), 
                                                                    spars, 
                                                                    ipars,
                                                                    newAddr);
                    if (ir.isSuccess())
                    {
                        ir = iaddrReg->registerAddress(rootPath + glastCnv->getPath(), newAddr);
//                        ir = dataSvc->registerAddress(rootPath + glastCnv->getPath(), newAddr);
                        if ( !ir.isSuccess() )    
                        {
                            newAddr->release();
                            status = ir;
                        }
                    }
                }
            }
        }
    }
    return status;
}

StatusCode OverlayCnvSvc::queryInterface(const InterfaceID& riid, void** ppvInterface)  
{
    // Return the results of querying the interface
    return ConversionSvc::queryInterface(riid, ppvInterface);
}

StatusCode OverlayCnvSvc::createAddress( long svc_type,
                                       const CLID& clid,
                                       const std::string* par, 
                                       const unsigned long* ip,
                                       IOpaqueAddress*& refpAddress)
{
    if ( svc_type == repSvcType() )   
    {
        GenericAddress* pAdd = new GenericAddress(svc_type, clid, par[0], par[1], ip[0], ip[1]);
        if ( pAdd )
        {
            refpAddress = pAdd;
            return StatusCode::SUCCESS;
        }
        // Call to GenericAddress routine
        pAdd->release();
    }
    return StatusCode::FAILURE;
}