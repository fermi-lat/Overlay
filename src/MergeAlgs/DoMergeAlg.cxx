/*
 * @file DoMergeAlg.cxx
 *
 * @brief Decides whether or not to read an overlay event
 *
 * @author Tracy Usher
 *
 * $Header: /nfs/slac/g/glast/ground/cvs/GlastRelease-scons/Overlay/src/MergeAlgs/DoMergeAlg.cxx,v 1.4 2011/11/03 18:13:50 usher Exp $
 */


#include "GaudiKernel/Algorithm.h"
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/AlgFactory.h"
#include "GaudiKernel/SmartDataPtr.h"
#include "GaudiKernel/ConversionSvc.h"
#include "GaudiKernel/DataSvc.h"
#include "GaudiKernel/GenericAddress.h"

#include "Event/TopLevel/EventModel.h"
#include "Event/MonteCarlo/McPositionHit.h"
#include "Event/MonteCarlo/McIntegratingHit.h"

#include "OverlayEvent/OverlayEventModel.h"
#include "OverlayEvent/EventOverlay.h"

#include "Overlay/IOverlayDataSvc.h"

#include <map>

class DoMergeAlg : public Algorithm 
{
 public:

    DoMergeAlg(const std::string&, ISvcLocator*);
    StatusCode initialize();
    StatusCode execute();
    StatusCode finalize();

private:
    StatusCode setRootEvent();

    bool             m_mergeAll;

    IOverlayDataSvc* m_dataSvc;

};

// Used by Gaudi for identifying this algorithm
//static const AlgFactory<DoMergeAlg>    Factory;
//const IAlgFactory& DoMergeAlgFactory = Factory;
DECLARE_ALGORITHM_FACTORY(DoMergeAlg);

DoMergeAlg::DoMergeAlg(const std::string& name, ISvcLocator* pSvcLocator)
    : Algorithm(name, pSvcLocator) 
{
    // variable to bypass if not wanted
    declareProperty("MergeAll", m_mergeAll = false);
}


StatusCode DoMergeAlg::initialize() 
{
    // Purpose and Method: initializes DoMergeAlg
    // Inputs: none
    // Outputs: a status code
    // Dependencies: value of m_type determining the type of tool to run
    // Restrictions and Caveats: none

    StatusCode sc = StatusCode::SUCCESS;
    MsgStream log(msgSvc(), name());
    log << MSG::INFO << "initialize" << endreq;

    if ( setProperties().isFailure() ) 
    {
        log << MSG::ERROR << "setProperties() failed" << endreq;
        return StatusCode::FAILURE;
    }

    // Convention for multiple input overlay files is that there will be separate OverlayDataSvc's with 
    // names appended by "_xx", for example OverlayDataSvc_1 for the second input file. 
    // In order to ensure the data read in goes into a unique section of the TDS we need to modify the 
    // base root path, which we do by examining the name of the service
    std::string dataSvcName = "OverlayDataSvc";
    int         subPos      = name().rfind("_");
    std::string nameEnding  = subPos > 0 ? name().substr(subPos, name().length() - subPos) : "";

    if (nameEnding != "") dataSvcName += nameEnding;

    IService* dataSvc = 0;
    sc = service(dataSvcName, dataSvc);
    if (sc.isFailure() ) {
        log << MSG::ERROR << "  can't get OverlayDataSvc " << endreq;
        return sc;
    }

    // Caste back to the "correct" pointer
    m_dataSvc = dynamic_cast<IOverlayDataSvc*>(dataSvc);

    return sc;
}


StatusCode DoMergeAlg::execute() 
{
    // Purpose and Method: execution method (called once for every event)
    //                     Doesn't do anything but calls the chosen tool.
    // Inputs: none
    // Outputs: a status code
    // Dependencies: none
    // Restrictions and Caveats: none

    StatusCode sc = StatusCode::SUCCESS; 
    MsgStream log(msgSvc(), name());
    log << MSG::DEBUG << "execute" << endreq;

    // Since our overlay stuff is not stored in the /Event section of the TDS, we need 
    // to explicitly "set the root" each event - or risk it not getting cleared. 
    sc = setRootEvent();

    if (sc.isFailure())
    {
        log << MSG::ERROR << "Clearing of the Overlay section of TDS failed" << endreq;
        return sc;
    }

    if (m_mergeAll)
    {
        log << MSG::DEBUG << "Merging all events, skipping DoMergeAlg" << endreq;
        return sc;
    }

    // How many hits in ACD, TKR or CAL?
    int numPosHits = 0;
    int numIntHits = 0;

    // Recover the McPositionHits for this event
    SmartDataPtr<Event::McPositionHitCol> posHitCol(eventSvc(), EventModel::MC::McPositionHitCol);

    if (posHitCol) numPosHits = posHitCol->size();

    // Recover the McIntegratingHits for this event
    SmartDataPtr<Event::McIntegratingHitCol> intHitCol(eventSvc(), EventModel::MC::McIntegratingHitCol);

    if (intHitCol) numIntHits = intHitCol->size();

    // if there are no McPositionHits AND no McIntegratingHits then the simulated particle did not interact
    if (!(numPosHits > 0 || numIntHits > 0)) setFilterPassed(false); 

    return sc;
}

StatusCode DoMergeAlg::finalize() 
{
    MsgStream log(msgSvc(), name());
    log << MSG::INFO << "finalize" << endreq;
    return StatusCode::SUCCESS;
}
    
StatusCode DoMergeAlg::setRootEvent()
{
    StatusCode sc = StatusCode::SUCCESS;

    // Set up the root event object
    Event::EventOverlay overObj;

    // Caste the data service into the input data service pointer
    DataSvc* dataProviderSvc = dynamic_cast<DataSvc*>(m_dataSvc);

    // Create a "simple" generic opaque address to go with this
    IOpaqueAddress* refpAddress = new GenericAddress(EXCEL_StorageType,
                                                     overObj.clID(),
                                                     dataProviderSvc->rootName());

    sc = dataProviderSvc->setRoot(dataProviderSvc->rootName(), refpAddress);

    // This is the magic incantation to trigger the building of the directory tree...
    SmartDataPtr<Event::EventOverlay> overHeader(dataProviderSvc, dataProviderSvc->rootName());
    if (!overHeader) sc = StatusCode::FAILURE;

    return sc;
}
