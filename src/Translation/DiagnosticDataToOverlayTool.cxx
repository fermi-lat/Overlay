/**  @file DiagnosticDataToOverlayTool.cxx
    @brief implementation of class DiagnosticDataToOverlayTool
    
  $Header: /nfs/slac/g/glast/ground/cvs/Overlay/src/Translation/DiagnosticDataToOverlayTool.cxx,v 1.2 2010/04/27 17:43:15 usher Exp $  
*/

#include "IDigiToOverlayTool.h"

#include "GaudiKernel/ToolFactory.h"
#include "GaudiKernel/AlgTool.h"
#include "GaudiKernel/SmartDataPtr.h"
#include "GaudiKernel/GaudiException.h" 
#include "GaudiKernel/IDataProviderSvc.h"
#include "GaudiKernel/DataSvc.h"

#include "LdfEvent/DiagnosticData.h"

#include "OverlayEvent/OverlayEventModel.h"
#include "OverlayEvent/DiagDataOverlay.h"

/** @class BackgroundSelection
    @brief manage the selection of background events to merge with signal events
    @author Dan Flath

It uses the name of a tuple variable as a key to look up trigger and downlink rates of a background
source depending on the value of that variable. See the corresponding member functions. Given such
a value, it expects to find a source of such events generated with that value which can be inserted 
into the Gleam output tuple.

*/
class DiagnosticDataToOverlayTool : public AlgTool, virtual public IDigiToOverlayTool
{
public:

    // Standard Gaudi Tool constructor
    DiagnosticDataToOverlayTool(const std::string& type, const std::string& name, const IInterface* parent);

    // After building it, destroy it
    ~DiagnosticDataToOverlayTool();

    /// @brief Intialization of the tool
    StatusCode initialize();

    /// @brief Finalize method for the tool
    StatusCode finalize();

    ///! The current value of the quantity that we are selecting on
    StatusCode translate();

private:

    /// Pointer to the event data service (aka "eventSvc")
    IDataProviderSvc*      m_edSvc;

    /// Pointer to the Overlay data service
    DataSvc*          m_dataSvc;
};

static ToolFactory<DiagnosticDataToOverlayTool> s_factory;
const IToolFactory& DiagnosticDataToOverlayToolFactory = s_factory;

//------------------------------------------------------------------------
DiagnosticDataToOverlayTool::DiagnosticDataToOverlayTool(const std::string& type, 
                                 const std::string& name, 
                                 const IInterface* parent) :
                                 AlgTool(type, name, parent)
{
    //Declare the additional interface
    declareInterface<IDigiToOverlayTool>(this);

    return;
}
//------------------------------------------------------------------------
DiagnosticDataToOverlayTool::~DiagnosticDataToOverlayTool()
{
    return;
}

StatusCode DiagnosticDataToOverlayTool::initialize()
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

StatusCode DiagnosticDataToOverlayTool::finalize ()
{
    StatusCode  status = StatusCode::SUCCESS;
    
    return status;
}

//------------------------------------------------------------------------
StatusCode DiagnosticDataToOverlayTool::translate()
{
    MsgStream log(msgSvc(), name());
    StatusCode status = StatusCode::SUCCESS;

    // Now recover the hit collection
    SmartDataPtr<LdfEvent::DiagnosticData> diagnosticData(m_edSvc, "/Event/Diagnostic");

    // Create a collection of AcdOverlays and register in the TDS
    SmartDataPtr<Event::DiagDataOverlay> diagDataOverlay(m_dataSvc, m_dataSvc->rootName() + OverlayEventModel::Overlay::DiagDataOverlay);

    if(!diagDataOverlay)
    {
        diagDataOverlay = new Event::DiagDataOverlay();

        status = m_dataSvc->registerObject(m_dataSvc->rootName() + OverlayEventModel::Overlay::DiagDataOverlay, diagDataOverlay);
        if( status.isFailure() ) 
        {
            log << MSG::ERROR << "could not register OverlayEventModel::Overlay::DiagDataOverlay" << endreq;
            return status;
        }
    }

    // Copy over Cal diagnostic info first
    int numCalDiags = diagnosticData->getNumCalDiagnostic();

    for(int idx = 0; idx < numCalDiags; idx++)
    {
        const LdfEvent::CalDiagnosticData& calDiagnosticData = diagnosticData->getCalDiagnosticByIndex(idx);

        // Only store if data word is non-zero
        if (calDiagnosticData.dataWord())
        {
            Event::CalDiagDataOverlay cal(calDiagnosticData.dataWord(), calDiagnosticData.tower(), calDiagnosticData.layer());

            diagDataOverlay->addCalDiagnostic(cal);
        }
    }

    // Now come back and copy the Tkr diagnostic info
    int numTkrDiags = diagnosticData->getNumTkrDiagnostic();

    for(int idx = 0; idx < numTkrDiags; idx++)
    {
        const LdfEvent::TkrDiagnosticData& tkrDiagnosticData = diagnosticData->getTkrDiagnosticByIndex(idx);

        // Only store if data word is non-zero
        if (tkrDiagnosticData.dataWord())
        {
            Event::TkrDiagDataOverlay tkr(tkrDiagnosticData.dataWord(), tkrDiagnosticData.tower(), tkrDiagnosticData.gtcc());

            diagDataOverlay->addTkrDiagnostic(tkr);
        }
    }

    return status;
}
