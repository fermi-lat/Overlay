// $Header: /nfs/slac/g/glast/ground/cvs/Overlay/src/test/OverlayTestAlg.cxx,v 1.5 2011/12/12 20:54:56 heather Exp $

// Include files

#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/AlgFactory.h"
#include "GaudiKernel/Algorithm.h"
#include "GaudiKernel/GaudiException.h" 
#include "GaudiKernel/SmartIF.h"
#include "GaudiKernel/SmartDataPtr.h"
#include "GaudiKernel/DataSvc.h"
#include "GaudiKernel/GenericAddress.h"

#include "Overlay/IOverlayDataSvc.h"

#include "OverlayEvent/OverlayEventModel.h"
#include "OverlayEvent/EventOverlay.h"
#include "OverlayEvent/AcdOverlay.h"
#include "OverlayEvent/CalOverlay.h"
#include "OverlayEvent/TkrOverlay.h"

#include "Event/TopLevel/Event.h"
#include "Event/TopLevel/EventModel.h"

#include <string>

/*! \class OverlayTestAlg
\brief 
  This is a place to put test code to examine what G4Generator has done.
  */

class OverlayTestAlg : public Algorithm {
    
public:
    //! Constructor of this form must be provided
    OverlayTestAlg(const std::string& name, ISvcLocator* pSvcLocator); 
    
    StatusCode initialize();
    StatusCode execute();
    StatusCode finalize();
    
private:
    StatusCode setRootEvent();

    IOverlayDataSvc* m_overlayInputSvc;

    /// Pointer to the overlay's data provider service
    DataSvc*         m_dataSvc;
};


//static const AlgFactory<OverlayTestAlg>  Factory;
//const IAlgFactory& OverlayTestAlgFactory = Factory;
DECLARE_ALGORITHM_FACTORY(OverlayTestAlg);

//
OverlayTestAlg::OverlayTestAlg(const std::string& name, ISvcLocator* pSvcLocator) :
Algorithm(name, pSvcLocator)
{
//    declareProperty("StartX", m_startx = 200.); 
//    declareProperty("StartX", m_starty = 200.); 
}


/*! */
StatusCode OverlayTestAlg::initialize() 
{
    MsgStream log(msgSvc(), name());
    log << MSG::INFO << "initializing..." << endreq;
    StatusCode  sc = StatusCode::SUCCESS;
    
    // Use the Job options service to set the Algorithm's parameters
    setProperties();

    // Try to retrieve the input data service
    IService* tmpService = 0;
    if (service("OverlayInputSvc", tmpService, true).isFailure())
    {
        log << MSG::INFO << "No OverlayInputSvc available, no input conversion will be performed" << endreq;
    }
    else 
    {
        m_overlayInputSvc = SmartIF<IOverlayDataSvc>(tmpService);
        //m_overlayInputSvc = SmartIF<IOverlayDataSvc>(IID_IOverlayDataSvc, tmpService);

        if (!m_overlayInputSvc) log << MSG::INFO << "Could not find input data service interface" << endreq;
        else                  log << MSG::INFO << "Input data service successfully retrieved" << endreq;
    }

    IService* dataSvc = 0;
    sc = service("OverlayDataSvc", dataSvc);
    if (sc.isFailure() ) {
        log << MSG::ERROR << "  can't get OverlayDataSvc " << endreq;
        return sc;
    }

    // Caste back to the "correct" pointer
    m_dataSvc = dynamic_cast<DataSvc*>(dataSvc);

    // Now look for the output data service
    if (service("OverlayOutputSvc", tmpService).isFailure())
    {
        log << MSG::INFO << "No OverlayOutputSvc available, no output conversion will be performed" << endreq;
    }
    else 
    {
        IOverlayDataSvc* overlayInputSvc = SmartIF<IOverlayDataSvc>(tmpService);
        //IOverlayDataSvc* overlayInputSvc = SmartIF<IOverlayDataSvc>(IID_IOverlayDataSvc, tmpService);

        if (!overlayInputSvc) log << MSG::INFO << "Could not find output data service interface" << endreq;
        else                  log << MSG::INFO << "Output data service successfully retrieved" << endreq;
    }

    return sc;
}


StatusCode OverlayTestAlg::execute() 
{
    StatusCode  sc = StatusCode::SUCCESS;
    MsgStream   log( msgSvc(), name() );

    // Retrieve the Event data for this event
    SmartDataPtr<Event::EventHeader> eventHeader(eventSvc(), EventModel::EventHeader);

    // For the test job it is necessary to set the time
    eventHeader->setTime(0);

    // Since our overlay stuff is not stored in the /Event section of the TDS, we need 
    // to explicitly "set the root" each event - or risk it not getting cleared. 
    sc = setRootEvent();

    if (sc.isFailure())
    {
        log << MSG::ERROR << "Clearing of the Overlay section of TDS failed" << endreq;
        return sc;
    }

    // Look up the EventOverlay object in the TDS
    SmartDataPtr<Event::EventOverlay> event(m_dataSvc, m_dataSvc->rootName() + OverlayEventModel::Overlay::EventOverlay);
    if(!event)
    {
        log << MSG::INFO << "Could not find a EventOverlay object in the TDS" << endreq;
    }

    // First, recover CalOverlay objects, to see if we have anything to do
    SmartDataPtr<Event::CalOverlayCol> calOverlayCol(m_dataSvc, m_dataSvc->rootName() + OverlayEventModel::Overlay::CalOverlayCol);
    if(!calOverlayCol)
    {
        log << MSG::INFO << "Could not find a CalOverlay object in the TDS" << endreq;
    }

    // Now recover any TkrOverlay objects, to see if we have anything to do
    SmartDataPtr<Event::TkrOverlayCol> tkrOverlayCol(m_dataSvc, m_dataSvc->rootName() + OverlayEventModel::Overlay::TkrOverlayCol);
    if(!tkrOverlayCol)
    {
        log << MSG::INFO << "Could not find a CalOverlay object in the TDS" << endreq;
    }

    // Finally, recover any AcdOverlay objects, to see if we have anything to do
    SmartDataPtr<Event::AcdOverlayCol> acdOverlayCol(m_dataSvc, m_dataSvc->rootName() + OverlayEventModel::Overlay::AcdOverlayCol);
    if(!acdOverlayCol)
    {
        log << MSG::INFO << "Could not find a CalOverlay object in the TDS" << endreq;
    }

    // Ask service to update event
//    sc = m_overlayInputSvc->selectNextEvent();

    return sc;
}


StatusCode OverlayTestAlg::finalize() 
{    
    return StatusCode::SUCCESS;
}
    
StatusCode OverlayTestAlg::setRootEvent()
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






