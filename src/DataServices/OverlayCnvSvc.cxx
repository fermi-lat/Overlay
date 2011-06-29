// File and Version Information:
//      $Header: /nfs/slac/g/glast/ground/cvs/GlastSvc/src/EventSelector/OverlayCnvSvc.cpp,v 1.10 2009/09/11 03:19:37 heather Exp $
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
#include "GaudiKernel/ICnvManager.h"
#include "GaudiKernel/ISvcLocator.h"
#include "GaudiKernel/IDataProviderSvc.h"
#include "GaudiKernel/IDataManagerSvc.h"
#include "GaudiKernel/DataSvc.h"
#include "GaudiKernel/RegistryEntry.h"
#include "GaudiKernel/GenericAddress.h"

#include "GlastSvc/EventSelector/IGlastCnv.h"

#include <map>

static const InterfaceID IID_OverlayCnvSvc("OverlayCnvSvc", 1, 0);

/** @class OverlayCnvSvc
 * @brief GLAST Event Conversion Service which coordinates all of our converters.
 *
 * When a component requests an item not yet available on the TDS, 
 * the OverlayCnvSvc is called to find the appropriiate converter to gain
 * access to the data and put it on the TDS.
 * Based on SICb service written by Markus Frank.
 *
 * $Header: /nfs/slac/g/glast/ground/cvs/GlastSvc/src/EventSelector/OverlayCnvSvc.cpp,v 1.10 2009/09/11 03:19:37 heather Exp $
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
    typedef std::map<std::string, IGlastCnv*>                PathToCnvMap;
    typedef std::map<std::string, std::vector<std::string> > SubPathMap;

    typedef std::map<const CLID,  IGlastCnv*>                CLIDToCnvMap;
    typedef std::map<const CLID,  std::vector<const CLID> >  SubCLIDMap;


    PathToCnvMap        m_pathToCnvMap;
    SubPathMap          m_subPathMap;
    CLIDToCnvMap        m_clidToCnvMap;
    SubCLIDMap          m_subClidMap;
};

#include "GlastSvc/EventSelector/IGlastCnv.h"

//static const InterfaceID IID_IBaseCnv(902, 1 , 0); 
// RCS Id for identification of object version
static const char* rcsid = "$Id: OverlayCnvSvc.cpp,v 1.10 2009/09/11 03:19:37 heather Exp $";


// Instantiation of a static factory class used by clients to create
// instances of this service
static const SvcFactory<OverlayCnvSvc> s_OverlayCnvSvcFactory;
const ISvcFactory& OverlayCnvSvcFactory = s_OverlayCnvSvcFactory;

OverlayCnvSvc::OverlayCnvSvc(const std::string& name, ISvcLocator* svc)
             : ConversionSvc(name, svc, EXCEL_StorageType)              
{
    m_pathToCnvMap.clear();
    m_subPathMap.clear();
    m_clidToCnvMap.clear();
    m_subClidMap.clear();
}

StatusCode OverlayCnvSvc::initialize()     
{
    // Purpose and Method:  Setup GLAST's Event Converter Service.
    //   Associate OverlayCnvSvc with the EventDataSvc
    //   Associate the list of known converters with this OverlayCnvSvc.
    MsgStream log(msgSvc(), name());

    StatusCode status = ConversionSvc::initialize();
    
    // Add known converters to the service: 
    for (ICnvManager::CnvIterator i = cnvManager()->cnvBegin(); i != cnvManager()->cnvEnd(); i++ )   
    {
        // Make sure the converters are of our "type" (EXCEL_StorageType)
        if ( repSvcType() == (*i)->repSvcType() )   
        {
            // Add the converter
            StatusCode iret = addConverter( (*i)->objType() );  

            // Was there a problem?
            if ( iret.isFailure() )   
            {
                log << MSG::ERROR << "Unable to add converter! " << endreq;
                return iret;
            }

            // Caste back to our conversion type
            IGlastCnv* glastCnv = dynamic_cast<IGlastCnv*>(converter((*i)->objType()));

            // Store the pointer to the converter in our map
            m_pathToCnvMap[glastCnv->getPath()] = glastCnv;
            m_clidToCnvMap[glastCnv->objType()] = glastCnv;
        }
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

        // An orphan?
        if (orphan)
        {
            // HMK Unused int j = 0;
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
        SmartIF<IDataManagerSvc> iaddrReg(IID_IDataManagerSvc, registry->dataSvc());

        // Find the list of related converters
        SubCLIDMap::iterator clidIter = m_subClidMap.find(clID);

        // Check to be sure something was found
        if (clidIter != m_subClidMap.end())
        {
            // Loop over the list of daughter converters and activate them
            for(std::vector<const CLID>::iterator subIter = clidIter->second.begin(); subIter != clidIter->second.end(); subIter++)
            {
                // Now look up the pointer to the converter
                CLIDToCnvMap::iterator cnvIter  = m_clidToCnvMap.find(*subIter);
                IGlastCnv*             glastCnv = cnvIter->second;

                if (glastCnv)
                {
                    // Avoid self reference, though this should not happen here
                    if (glastCnv->objType() == clID) continue;

                    const CLID& clID2 = glastCnv->objType();

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
    if ( IID_OverlayCnvSvc == riid )  
    {
        *ppvInterface = (OverlayCnvSvc*)this;
    }
    else  
    {
        // Interface is not directly availible: try out a base class
        return ConversionSvc::queryInterface(riid, ppvInterface);
    }
    addRef();
    return StatusCode::SUCCESS;
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
