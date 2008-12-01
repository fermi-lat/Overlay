/**  @file GemToOverlayTool.cxx
    @brief implementation of class GemToOverlayTool
    
  $Header: /nfs/slac/g/glast/ground/cvs/Overlay/src/GemToOverlayTool.cxx,v 1.0 2008/10/15 15:14:30 usher Exp $  
*/

#include "IDigiToOverlayTool.h"

#include "GaudiKernel/ToolFactory.h"
#include "GaudiKernel/AlgTool.h"
#include "GaudiKernel/SmartDataPtr.h"
#include "GaudiKernel/GaudiException.h" 
#include "GaudiKernel/IDataProviderSvc.h"

#include "LdfEvent/Gem.h"

#include "OverlayEvent/OverlayEventModel.h"
#include "OverlayEvent/GemOverlay.h"

/** @class BackgroundSelection
    @brief manage the selection of background events to merge with signal events
    @author Dan Flath

It uses the name of a tuple variable as a key to look up trigger and downlink rates of a background
source depending on the value of that variable. See the corresponding member functions. Given such
a value, it expects to find a source of such events generated with that value which can be inserted 
into the Gleam output tuple.

*/
class GemToOverlayTool : public AlgTool, virtual public IDigiToOverlayTool
{
public:

    // Standard Gaudi Tool constructor
    GemToOverlayTool(const std::string& type, const std::string& name, const IInterface* parent);

    // After building it, destroy it
    ~GemToOverlayTool();

    /// @brief Intialization of the tool
    StatusCode initialize();

    /// @brief Finalize method for the tool
    StatusCode finalize();

    ///! The current value of the quantity that we are selecting on
    StatusCode translate();

private:

    /// Pointer to the event data service (aka "eventSvc")
    IDataProviderSvc*      m_edSvc;
};

static ToolFactory<GemToOverlayTool> s_factory;
const IToolFactory& GemToOverlayToolFactory = s_factory;

//------------------------------------------------------------------------
GemToOverlayTool::GemToOverlayTool(const std::string& type, 
                                 const std::string& name, 
                                 const IInterface* parent) :
                                 AlgTool(type, name, parent)
{
    //Declare the additional interface
    declareInterface<IDigiToOverlayTool>(this);

    return;
}
//------------------------------------------------------------------------
GemToOverlayTool::~GemToOverlayTool()
{
    return;
}

StatusCode GemToOverlayTool::initialize()
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

    return sc;
}

StatusCode GemToOverlayTool::finalize ()
{
    StatusCode  status = StatusCode::SUCCESS;
    
    return status;
}

//------------------------------------------------------------------------
StatusCode GemToOverlayTool::translate()
{
    MsgStream log(msgSvc(), name());
    StatusCode status = StatusCode::SUCCESS;

    // Now recover the hit collection
    SmartDataPtr<LdfEvent::Gem> gem(m_edSvc, "/Event/Gem");

    // Create a collection of AcdOverlays and register in the TDS
    SmartDataPtr<Event::GemOverlay> gemOverlay(m_edSvc, OverlayEventModel::Overlay::GemOverlay);
    if(!gemOverlay)
    {
        gemOverlay = new Event::GemOverlay();

        status = m_edSvc->registerObject(OverlayEventModel::Overlay::GemOverlay, gemOverlay);
        if( status.isFailure() ) 
        {
            log << MSG::ERROR << "could not register OverlayEventModel::Overlay::GemOverlay" << endreq;
            return status;
        }
    }

    // Copy information from LdfEvent's Gem to Overlay's
    const LdfEvent::GemTileList gemTileList = gem->tileList();
    Event::GemOverlayTileList tileList(gemTileList.xzm(), 
                                       gemTileList.xzp(), 
                                       gemTileList.yzm(), 
                                       gemTileList.yzp(), 
                                       gemTileList.xy(), 
                                       gemTileList.rbn(), 
                                       gemTileList.na());

    gemOverlay->initTrigger(gem->tkrVector(), 
                            gem->roiVector(),
                            gem->calLEvector(), 
                            gem->calHEvector(),
                            gem->cnoVector(), 
                            gem->conditionSummary(),
                            gem->missed(), 
                            tileList);

    Event::GemOverlayOnePpsTime ppsTime(gem->onePpsTime().timebase(),
                                        gem->onePpsTime().seconds());

    Event::GemOverlayDataCondArrivalTime dataTime;
    dataTime.init(gem->condArrTime().condArr());

    gemOverlay->initSummary(gem->liveTime(), 
                            gem->prescaled(),
                            gem->discarded(), 
                            dataTime,
                            gem->triggerTime(), 
                            ppsTime, 
                            gem->deltaEventTime(), 
                            gem->deltaWindowOpenTime());

    return status;
}
