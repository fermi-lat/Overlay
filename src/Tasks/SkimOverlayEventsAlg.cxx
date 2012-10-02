/** @file SkimOverlayEventsAlg.cxx
    @brief declartion, implementaion of the class SkimOverlayEventsAlg

    $Header: /nfs/slac/g/glast/ground/cvs/Overlay/src/Tasks/SkimOverlayEventsAlg.cxx,v 1.2 2011/12/12 20:54:55 heather Exp $
*/
// Gaudi system includes
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/AlgFactory.h"
#include "GaudiKernel/IDataProviderSvc.h"
#include "GaudiKernel/SmartDataPtr.h"
#include "GaudiKernel/Algorithm.h"
#include "GaudiKernel/IService.h"
#include "GaudiKernel/SmartIF.h"



// if using the gui
//#include "GuiSvc/IGuiSvc.h"
//#include "gui/GuiMgr.h"
//#include "gui/DisplayControl.h"

// TDS class declarations: input data, and McParticle tree
//#include "Event/MonteCarlo/McParticle.h"
//#include "Event/MonteCarlo/McIntegratingHit.h"
//#include "Event/MonteCarlo/McPositionHit.h"

#include "Event/TopLevel/Event.h"
#include "Event/TopLevel/EventModel.h"
//#include "Event/TopLevel/MCEvent.h"

//#include "Event/Recon/TkrRecon/TkrVertex.h"
//#include "Event/Recon/CalRecon/CalCluster.h"
//#include "Event/Recon/CalRecon/CalXtalRecData.h"
//#include "Event/Recon/AcdRecon/AcdRecon.h"

#include "LdfEvent/Gem.h"
#include "enums/TriggerBits.h"

#include "Overlay/IOverlayDataSvc.h"

//#include "ntupleWriterSvc/INTupleWriterSvc.h"

// for access to geometry perhaps
#include "GlastSvc/GlastDetSvc/IGlastDetSvc.h"

// for the propagator
//#include "GlastSvc/Reco/IPropagator.h"

#include <cassert>
#include <sstream>

/** @class SkimOverlayEventsAlg
@brief A simple algorithm.
*/
class SkimOverlayEventsAlg : public Algorithm {
public:
    SkimOverlayEventsAlg(const std::string& name, ISvcLocator* pSvcLocator);
    /// set parameters and attach to various perhaps useful services.
    StatusCode initialize();
    /// process one event
    StatusCode execute();
    /// clean up
    StatusCode finalize();
    
private: 
    /// number of times called
    double m_count; 
    /// the GlastDetSvc used for access to detector info
    IGlastDetSvc*    m_detSvc; 
    /// access to the Gui Service for display of 3-d objects
    //IGuiSvc*    m_guiSvc;
    /// access to the overlay data Service for display of 3-d objects
    IOverlayDataSvc*    m_overlayOutputSvc;


};


//static const AlgFactory<SkimOverlayEventsAlg>  Factory;
//const IAlgFactory& SkimOverlayEventsAlgFactory = Factory;
DECLARE_ALGORITHM_FACTORY(SkimOverlayEventsAlg);

SkimOverlayEventsAlg::SkimOverlayEventsAlg(const std::string& name, ISvcLocator* pSvcLocator)
:Algorithm(name, pSvcLocator)
,m_count(0)
{
    // declare properties with setProperties calls
    //declareProperty("treeName",  m_treeName="");
    
}

StatusCode SkimOverlayEventsAlg::initialize(){
    StatusCode  sc = StatusCode::SUCCESS;
    MsgStream log(msgSvc(), name());
    log << MSG::INFO << "initialize" << endreq;
    
    // Use the Job options service to set the Algorithm's parameters
    setProperties();
    
    //if( m_treeName.empty()) {
    //    log << MSG::INFO << "tupleName property not set!  No ntuple output"<<endreq;
    //}
    
    // now try to find the GlastDevSvc service
    if (service("GlastDetSvc", m_detSvc).isFailure()){
        log << MSG::ERROR << "Couldn't find the GlastDetSvc!" << endreq;
        return StatusCode::FAILURE;
    }
    
    

    IService* tmpService = 0;

    // Now look up the output data service
    if (service("OverlayOutputSvc", tmpService, false).isFailure())
    {
        log << MSG::INFO << "No OverlayOutputSvc available, no input conversion will be performed" << endreq;
        m_overlayOutputSvc = 0;
    }
    else m_overlayOutputSvc = SmartIF<IOverlayDataSvc>(tmpService);

    return sc;
}

StatusCode SkimOverlayEventsAlg::execute()
{
    StatusCode  sc = StatusCode::SUCCESS;
    MsgStream   log( msgSvc(), name() );
    log << MSG::DEBUG << "executing " << ++m_count << " time" << endreq;

    

    // Retrieving pointers from the TDS
    
    SmartDataPtr<Event::EventHeader>   header(eventSvc(),    EventModel::EventHeader);
    //SmartDataPtr<Event::MCEvent>     mcheader(eventSvc(),    EventModel::MC::Event);
    //SmartDataPtr<Event::McParticleCol> particles(eventSvc(), EventModel::MC::McParticleCol);
    //SmartDataPtr<Event::TkrVertexCol>  vertices(eventSvc(),    EventModel::TkrRecon::TkrVertexCol);
    //SmartDataPtr<Event::CalClusterCol> clusters(eventSvc(),  EventModel::CalRecon::CalClusterCol);
    //SmartDataPtr<Event::CalXtalRecCol> xtalrecs(eventSvc(),  EventModel::CalRecon::CalXtalRecCol);
    //SmartDataPtr<Event::AcdRecon> acdrec(eventSvc(),         EventModel::AcdRecon::Event);

        // Now recover the hit collection
    //SmartDataPtr<LdfEvent::Gem> gem(eventSvc(), "/Event/Gem");

    int WordTwo = header->triggerWordTwo();
    int gemEngine = ((WordTwo >> enums::ENGINE_offset) & enums::ENGINE_mask);

    if (gemEngine!=3) {
        setFilterPassed( false );
        m_overlayOutputSvc->storeEvent(false);
        return sc;
    }
     
    return sc;
}

StatusCode SkimOverlayEventsAlg::finalize(){
    StatusCode  sc = StatusCode::SUCCESS;
    MsgStream log(msgSvc(), name());
    log << MSG::INFO << "finalize after " << m_count << " calls." << endreq;
    
    return sc;
}

