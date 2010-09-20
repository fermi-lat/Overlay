/** @file SkimOverlayEventsAlg.cxx
    @brief declartion, implementaion of the class SkimOverlayEventsAlg

    $Header: /nfs/slac/g/glast/ground/cvs/Overlay/src/Tasks/SkimOverlayEventsAlg.cxx,v 1.1 2009/01/16 04:04:04 lsrea Exp $
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


    /// access the ntupleWriter service to write out to ROOT ntuples
    //INTupleWriterSvc *m_rootTupleSvc;

    /// parameter to store the logical name of the ROOT file to write to
    //std::string m_treeName;

    /// geometry propagator
    //IPropagator* m_prop;

    /// distance of conversion point and nearest ACD along the vertex
    /// aka completely useless variable
    //double m_cuv;
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
    
    // get the Gui service (not required)
    //if (service("GuiSvc", m_guiSvc).isFailure ()){
    //    log << MSG::WARNING << "No GuiSvc, so no display" << endreq;
    //}else{
    //    //m_guiSvc->guiMgr()->display().add(new Rep, "User rep");
    //}
    

    IService* tmpService = 0;

    // Now look up the output data service
    if (service("OverlayOutputSvc", tmpService, false).isFailure())
    {
        log << MSG::INFO << "No OverlayOutputSvc available, no input conversion will be performed" << endreq;
        m_overlayOutputSvc = 0;
    }
    else m_overlayOutputSvc = SmartIF<IOverlayDataSvc>(tmpService);
    //else m_overlayOutputSvc = SmartIF<IOverlayDataSvc>(IID_IOverlayDataSvc, tmpService);
    
    // example code to create a ROOT tuple and schedule it for filling 

    // get a pointer to RootTupleSvc
    //sc = service("RootTupleSvc", m_rootTupleSvc);

    //if( sc.isFailure() ) {
    //    log << MSG::ERROR << "SkimOverlayEventsAlg failed to get the RootTupleSvc" << endreq;
    //    return sc;
    //}
    //m_rootTupleSvc->addItem(m_treeName, "count", &m_count);
    //m_rootTupleSvc->addItem(m_treeName, "completely_useless", &m_cuv);

    //IToolSvc* toolSvc = 0;
    //if ( service("ToolSvc", toolSvc, true).isFailure() ) {
    //    log << MSG::ERROR << "Couldn't find the ToolSvc!" << endreq;
    //    return StatusCode::FAILURE;
    //}
    //if ( !toolSvc->retrieveTool("G4PropagationTool", m_prop) ) {
    //    log << MSG::ERROR << "Couldn't find the G4PropagationTool!" << endreq;
    //    return StatusCode::FAILURE;
    //}

    return sc;
}

StatusCode SkimOverlayEventsAlg::execute()
{
    StatusCode  sc = StatusCode::SUCCESS;
    MsgStream   log( msgSvc(), name() );
    log << MSG::DEBUG << "executing " << ++m_count << " time" << endreq;

    //if( m_rootTupleSvc!=0) {
    //    // necessary to store the tuple entry
    //    m_rootTupleSvc->storeRowFlag(m_treeName, true);
    //}
    

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
     
    // *************************************************************************
    // exercise 1:
    // Apply cuts using the TDS data to determine we want this event to be
    // written to output or not
    // For example we could use the number of vertices to determine if we 
    // want this event written to output.  
    // If not, we setFilterPassed false and return - 
    // and in addition we must set up the
    // jobOptions such that this algorithm is used to filter - which means
    // it determines what algorithms are run after it    
    // One should redefine the Triggered sequence such that SkimOverlayEventsAlg appears
    // before either meritAlg or the Output sequence.
    //  Triggered.Members={
    //    "Sequencer/Trigger",
    //    "Sequencer/Reconstruction",
    //    "Sequencer/RecoDisplay",
    //    "SkimOverlayEventsAlg",
    //    "Sequencer/Output",
    //    "FilterTracks","FilterAlg",
    //    "meritAlg" };

    //if ((!vertices) || (vertices.size() <= 0)) {
    //    setFilterPassed( false );
    //    return sc;
    //}


    // *************************************************************************
    // exercise 2:
    // 1) get the first vertex of the TkrVertexCol.
    //    We need a pointer to Event::TkrVertexCol in the TDS.  No new code, was
    //    here anyway.
    // 2) retrieve position and direction.
    // 3) use a propagator to propagate back to the ACD.
    //    We need the IPropagator* m_prop. The pointer to it has to be retrieved
    //    in initialize().
    // 4) determine the distance of the vertex (conversion point) and the ACD.
    // 5) store this completely useless variable in MeritTuple.
    //    Not exactly true, actually.  It is stored in the tree specified by
    //    SkimOverlayEventsAlg.treeName in jobOptions.txt.  We need m_ntupleSvc and the
    //    pointer to it.  No new code, was here anyway.

    //m_cuv = 0.;

    
#if 0 // enable to use pause with the display
    // An example of pausing the display 
    bool pause = false;
    // calculate some condition here...
    // pause = .....;
    if (pause) {
        log << MSG::INFO << "Pausing at event " << m_count <<  endreq;
        m_guiSvc->guiMgr()->pause();
    } 
#endif
#if 0
    log << MSG::FATAL << "test of a fatal error " << endreq;
    sc = StatusCode::FAILURE;
#endif
    return sc;
}

StatusCode SkimOverlayEventsAlg::finalize(){
    StatusCode  sc = StatusCode::SUCCESS;
    MsgStream log(msgSvc(), name());
    log << MSG::INFO << "finalize after " << m_count << " calls." << endreq;
    
    return sc;
}

