/**  @file EventToOverlayTool.cxx
    @brief implementation of class EventToOverlayTool
    
  $Header: /nfs/slac/g/glast/ground/cvs/Overlay/src/Translation/EventToOverlayTool.cxx,v 1.1 2008/12/01 22:50:12 usher Exp $  
*/

#include "IDigiToOverlayTool.h"

#include "GaudiKernel/ToolFactory.h"
#include "GaudiKernel/AlgTool.h"
#include "GaudiKernel/SmartDataPtr.h"
#include "GaudiKernel/GaudiException.h" 
#include "GaudiKernel/IDataProviderSvc.h"
#include "GaudiKernel/DataSvc.h"
#include "GaudiKernel/GenericAddress.h"

#include "Event/TopLevel/EventModel.h"
#include "Event/TopLevel/Event.h"

#include "OverlayEvent/OverlayEventModel.h"
#include "OverlayEvent/EventOverlay.h"

/** @class BackgroundSelection
    @brief manage the selection of background events to merge with signal events
    @author Dan Flath

It uses the name of a tuple variable as a key to look up trigger and downlink rates of a background
source depending on the value of that variable. See the corresponding member functions. Given such
a value, it expects to find a source of such events generated with that value which can be inserted 
into the Gleam output tuple.

*/
class EventToOverlayTool : public AlgTool, virtual public IDigiToOverlayTool
{
public:

    // Standard Gaudi Tool constructor
    EventToOverlayTool(const std::string& type, const std::string& name, const IInterface* parent);

    // After building it, destroy it
    ~EventToOverlayTool();

    /// @brief Intialization of the tool
    StatusCode initialize();

    /// @brief Finalize method for the tool
    StatusCode finalize();

    ///! The current value of the quantity that we are selecting on
    StatusCode translate();

private:

    /// Pointer to the event data service (aka "eventSvc")
    IDataProviderSvc*   m_edSvc;

    /// Pointer to the Overlay data service
    DataSvc*          m_dataSvc;
};

static ToolFactory<EventToOverlayTool> s_factory;
const IToolFactory& EventToOverlayToolFactory = s_factory;

//------------------------------------------------------------------------
EventToOverlayTool::EventToOverlayTool(const std::string& type, 
                                 const std::string& name, 
                                 const IInterface* parent) :
                                 AlgTool(type, name, parent)
{
    //Declare the additional interface
    declareInterface<IDigiToOverlayTool>(this);
}
//------------------------------------------------------------------------
EventToOverlayTool::~EventToOverlayTool()
{
    return;
}

StatusCode EventToOverlayTool::initialize()
{
    StatusCode sc   = StatusCode::SUCCESS;
    MsgStream log(msgSvc(), name());

    // Set the properties
    setProperties();

    IService* iService = 0;
    sc = serviceLocator()->service("EventDataSvc", iService, true);
    if ( sc.isFailure() ) {
        log << MSG::ERROR << "could not find EventDataSvc !" << endreq;
        return sc;
    }
    m_edSvc = dynamic_cast<IDataProviderSvc*>(iService);

    sc = serviceLocator()->service("OverlayDataSvc", iService, true);
    if ( sc.isFailure() ) {
        log << MSG::ERROR << "could not find EventDataSvc !" << endreq;
        return sc;
    }
    m_dataSvc = dynamic_cast<DataSvc*>(iService);

    return sc;
}

StatusCode EventToOverlayTool::finalize ()
{
    StatusCode  status = StatusCode::SUCCESS;
    
    return status;
}

//------------------------------------------------------------------------
StatusCode EventToOverlayTool::translate()
{
    MsgStream log(msgSvc(), name());
    StatusCode status = StatusCode::SUCCESS;

    // Recover the TkrDigi collection from the TDS
    SmartDataPtr<Event::EventHeader> event(m_edSvc, EventModel::EventHeader);

    // Create a collection of TkrOverlays and register in the TDS
    SmartDataPtr<Event::EventOverlay> overlay(m_dataSvc, m_dataSvc->rootName() + OverlayEventModel::Overlay::EventOverlay);
    if(!overlay)
    {
        // Create the overlay Event object
        overlay = new Event::EventOverlay();

        // Set it as the root of the Overlay section in the TDS
        status = m_dataSvc->setRoot(m_dataSvc->rootName(), overlay);
        if( status.isFailure() ) 
        {
            log << MSG::ERROR << "could not set root for OverlayEventModel::Overlay::EventOverlay" << endreq;
            return status;
        }
    }

    // Check to see if the event and run ids have already been set.
    overlay->setEvent(event->event());
    overlay->setRun(event->run());
    overlay->setTime(event->time());
    overlay->setTrigger(event->trigger());
    overlay->setGemPrescale(event->gemPrescale());
    overlay->setGltPrescale(event->gltPrescale());
    overlay->setPrescaleExpired(event->prescaleExpired());
    overlay->setLivetime(event->livetime());
    overlay->setGleamEventFlags(event->gleamEventFlags());

    return status;
}
