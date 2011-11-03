/**  @file CalXtalToOverlayTool.cxx
    @brief implementation of class CalXtalToOverlayTool
    
  $Header: /nfs/slac/g/glast/ground/cvs/Overlay/src/Translation/CalXtalToOverlayTool.cxx,v 1.2 2011/06/27 17:45:57 usher Exp $  
*/

#include "IDigiToOverlayTool.h"

#include "GaudiKernel/ToolFactory.h"
#include "GaudiKernel/AlgTool.h"
#include "GaudiKernel/SmartDataPtr.h"
#include "GaudiKernel/GaudiException.h" 
#include "GaudiKernel/IDataProviderSvc.h"
#include "GaudiKernel/DataSvc.h"

#include "Event/TopLevel/EventModel.h"
#include "Event/Recon/CalRecon/CalXtalRecData.h"

#include "OverlayEvent/OverlayEventModel.h"
#include "OverlayEvent/CalOverlay.h"

/** @class BackgroundSelection
    @brief manage the selection of background events to merge with signal events
    @author Dan Flath

It uses the name of a tuple variable as a key to look up trigger and downlink rates of a background
source depending on the value of that variable. See the corresponding member functions. Given such
a value, it expects to find a source of such events generated with that value which can be inserted 
into the Gleam output tuple.

*/
class CalXtalToOverlayTool : public AlgTool, virtual public IDigiToOverlayTool
{
public:

    // Standard Gaudi Tool constructor
    CalXtalToOverlayTool(const std::string& type, const std::string& name, const IInterface* parent);

    // After building it, destroy it
    ~CalXtalToOverlayTool();

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

static ToolFactory<CalXtalToOverlayTool> s_factory;
const IToolFactory& CalXtalToOverlayToolFactory = s_factory;

//------------------------------------------------------------------------
CalXtalToOverlayTool::CalXtalToOverlayTool(const std::string& type, 
                                 const std::string& name, 
                                 const IInterface* parent) :
                                 AlgTool(type, name, parent)
{
    //Declare the additional interface
    declareInterface<IDigiToOverlayTool>(this);
}
//------------------------------------------------------------------------
CalXtalToOverlayTool::~CalXtalToOverlayTool()
{
    return;
}

StatusCode CalXtalToOverlayTool::initialize()
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

    sc = serviceLocator()->service("OverlayOutputSvc", iService, true);
    if ( sc.isFailure() ) {
        log << MSG::ERROR << "could not find EventDataSvc !" << endreq;
        return sc;
    }
    m_dataSvc = dynamic_cast<DataSvc*>(iService);

    return sc;
}

StatusCode CalXtalToOverlayTool::finalize ()
{
    StatusCode  status = StatusCode::SUCCESS;
    
    return status;
}

//------------------------------------------------------------------------
StatusCode CalXtalToOverlayTool::translate()
{
    MsgStream log(msgSvc(), name());
    StatusCode status = StatusCode::SUCCESS;

    // Recover the TkrDigi collection from the TDS
    SmartDataPtr<Event::CalXtalRecCol> calXtalRecCol(m_edSvc, EventModel::CalRecon::CalXtalRecCol);

    // Create a collection of TkrOverlays and register in the TDS
    SmartDataPtr<Event::CalOverlayCol> overlayCol(m_dataSvc, m_dataSvc->rootName() + OverlayEventModel::Overlay::CalOverlayCol);
    if(!overlayCol)
    {
        overlayCol = new Event::CalOverlayCol();

        status = m_dataSvc->registerObject(m_dataSvc->rootName() + OverlayEventModel::Overlay::CalOverlayCol, overlayCol);
        if( status.isFailure() ) 
        {
            log << MSG::ERROR << "could not register OverlayEventModel::Overlay::CalOverlayCol" << endreq;
            return status;
        }
    }

    // Loop over available TkrDigis
    for(Event::CalXtalRecCol::iterator calIter = calXtalRecCol->begin(); calIter != calXtalRecCol->end(); calIter++)
    {
        // Dereference our TkrDigi object
        Event::CalXtalRecData* calXtal = *calIter;

        // New CalOverlay object
        Event::CalOverlay* calOverlay = new Event::CalOverlay(calXtal->getPackedId(), calXtal->getPosition(), calXtal->getEnergy());

        // Set some status bits
        calOverlay->setStatus(Event::CalOverlay::DIGI_OVERLAY);

        // Add the finished product to our collection
        overlayCol->push_back(calOverlay);
    }

    return status;
}
