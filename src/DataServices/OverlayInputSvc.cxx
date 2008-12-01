// $Header: /nfs/slac/g/glast/ground/cvs/CalibSvc/src/OverlayInputSvc/OverlayInputSvc.cxx,v 1.0 2008/07/23 18:11:46 jrb Exp $

// Include files
#include "GaudiKernel/Service.h"
#include "GaudiKernel/SvcFactory.h"
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/IIncidentSvc.h"
#include "GaudiKernel/IIncidentListener.h"
#include "GaudiKernel/ISvcLocator.h"
#include "GaudiKernel/IToolSvc.h"

#include "OverlayEvent/OverlayEventModel.h"
#include "OverlayEvent/EventOverlay.h"

#include "RootIo/IRootIoSvc.h"
#include "overlayRootData/EventOverlay.h"
#include "facilities/Util.h"

#include "CLHEP/Random/RandFlat.h"

#include "../InputControl/XmlFetchEvents.h"
#include "Overlay/IOverlayDataSvc.h"
#include "Overlay/IBackgroundBinTool.h"
#include "Overlay/IFetchEvents.h"

class OverlayInputSvc : virtual public IOverlayDataSvc, virtual public IIncidentListener, public Service
{    
public:
    OverlayInputSvc(const std::string& name, ISvcLocator* svc);
    virtual ~OverlayInputSvc();

    // Overloaded DataSvc methods
    virtual StatusCode initialize();
  
    virtual StatusCode finalize();

    /// Query the interface of the service
    virtual StatusCode queryInterface( const InterfaceID& riid, void** ppvInterface );  

    /// Get pointer to a Root DigiEvent object
    virtual EventOverlay* getRootEventOverlay();

    /// Select the next event
    virtual StatusCode selectNextEvent();

    /// Register an output path with us
    virtual StatusCode registerOutputPath(const std::string& path) {return StatusCode::SUCCESS;}

    /// Handles incidents, implementing IIncidentListener interface
    virtual void handle( const Incident & ) ;    

private:
    void beginEvent() ;
    void endEvent() ;

    /**@brief Retrieve the correct tree for current value of the variable(s) so copyEvent comes from correct sample bin
    */
    void setNewInputBin(double val);

    /// access the RootIoSvc to get the CompositeEventList ptr
    IRootIoSvc *        m_rootIoSvc;

    unsigned int        m_eventOffset;

    IFetchEvents*       m_fetch;       ///< abstract guy that processes the xml file

    // Pointer to the object which determines which bin we are in
    IBackgroundBinTool* m_binTool;

    // Pointer to input data
    EventOverlay*       m_eventOverlay;

    StringProperty      m_inputXmlFilePath;

    /// Option string which will be passed to McEvent::Clear
    StringProperty      m_clearOption;

    /// Name of the Background Overlay tool to use
    StringProperty      m_overlay; 
};


// Instantiation of a static factory class used by clients to create
// instances of this service
static SvcFactory<OverlayInputSvc> s_factory;
const ISvcFactory& OverlayInputSvcFactory = s_factory;

/// Standard Constructor
OverlayInputSvc::OverlayInputSvc(const std::string& name,ISvcLocator* svc) : Service(name,svc),
                               m_rootIoSvc(0), m_eventOverlay(0)
{
    // Input pararmeters that may be set via the jobOptions file
    // Input ROOT file name  provided for backward compatibility, digiRootFileList is preferred
    // The files will be ignored if RootIoSvc is provided a meta (event collection) ROOT file

    declareProperty("OverlayTool",      m_overlay="McIlwain_L");
    declareProperty("InputXmlFilePath", m_inputXmlFilePath="$(OVERLAYROOT)/xml");
    declareProperty("clearOption",      m_clearOption="");

    return;
}

/// Standard Destructor
OverlayInputSvc::~OverlayInputSvc()  
{
}

// Service initialization
StatusCode OverlayInputSvc::initialize()   
{
    StatusCode status = StatusCode::SUCCESS;

    // Set up MsgSvc
    MsgStream log(msgSvc(), name());
    
    // Use the Job options service to set the Algorithm's parameters
    // This will retrieve parameters set in the job options file
    setProperties();

    // Get pointer to RootIoSvc
    status = service("RootIoSvc", m_rootIoSvc, true);       
    if( status.isFailure() ) 
    {
        log << MSG::ERROR << "failed to get the RootIoSvc" << endreq;
        return status;
    }

    // Get pointer to the ToolSvc
    IService* tmpToolSvc = 0;
    status = service("ToolSvc", tmpToolSvc, true);
    if (status.isFailure())
    {
        log << MSG::ERROR << "Can't find the tool service! " << endreq;
        return status;
    }

    IToolSvc* toolSvc = dynamic_cast<IToolSvc*>(tmpToolSvc);

    // Now set up the input binning tool
    if( !m_overlay.value().empty())
    {
        // Start with input xml file name
        std::string xmlFile = m_inputXmlFilePath.value() + "/" + m_overlay.value() + ".xml";

        facilities::Util::expandEnvVar(&xmlFile);
        log << MSG::INFO << "Using xml file " << xmlFile 
            << " for " + name() + "." << endreq;

        // Ok, set up the xml reading object
        m_fetch = new XmlFetchEvents(xmlFile, m_overlay.value());

        // Set up the name of the input bin tool
        std::string toolName = m_overlay.value() + "_Tool";

        // Now look it up
        if (toolSvc->retrieveTool(toolName, "BinTool", m_binTool).isFailure())
        {
            log << MSG::INFO << "Couldn't find the BinTool: " << toolName << endreq;
            return StatusCode::FAILURE;
        }
    }

    // use the incident service to register begin, end events
    IIncidentSvc* incsvc = 0;
    status = service ("IncidentSvc", incsvc, true);

    if( status.isFailure() ) return status;

    incsvc->addListener(this, "BeginEvent", 100);
    incsvc->addListener(this, "EndEvent", 0);

    return status;
}

/// Finalize the service.
StatusCode OverlayInputSvc::finalize()
{
    MsgStream log(msgSvc(), name());

    log << MSG::DEBUG << "Finalizing" << endreq;

    delete m_fetch;

    // Finalize the base class
    return StatusCode::SUCCESS;
}

StatusCode OverlayInputSvc::queryInterface(const InterfaceID& riid, void** ppvInterface)
{
    StatusCode status = StatusCode::SUCCESS;

    if ( IID_IOverlayDataSvc.versionMatch(riid) )  
    {
        *ppvInterface = (IOverlayDataSvc*)this;
    }
    else status = Service::queryInterface(riid, ppvInterface);

    return status;
}

EventOverlay* OverlayInputSvc::getRootEventOverlay()
{
    if (m_eventOverlay == 0) selectNextEvent();

    return m_eventOverlay;
}

StatusCode OverlayInputSvc::selectNextEvent()
{
    MsgStream log(msgSvc(), name());

    // Get the desired bin
    double x = m_binTool->value();

    // First check that the returned value is within range
    if( !m_fetch->isValid(x) )
    {
        log << MSG::ERROR 
            << "selectEvent: called with " << name() 
            <<" = "<< x << " is not in range " 
            <<  m_fetch->minValFullRange() << ", to " 
            <<  m_fetch->maxValFullRange() 
            << endreq;
        return StatusCode::FAILURE;
    }

    // make sure we have the right tree selected for new value    
    // if still valid, do not change
    if( !m_fetch->isCurrent(x) )
    {
        // New bin, set new tree
        setNewInputBin(x);   
    }

    // If m_eventOverlay is not null then we have a problem
    if (m_eventOverlay)
    {
        log << MSG::ERROR << "Found non-zero pointer to DigiEvent during event loop!" << endreq;
        return StatusCode::FAILURE;
    }

    // Try reading the event this way... 
    // using treename as the key
    m_eventOverlay = dynamic_cast<EventOverlay*>(m_rootIoSvc->getNextEvent("overlay"));

    // If the call returns a null pointer then most likely we have hit the end of file
    // Try to wrap back to first event and try again
    if( m_eventOverlay == 0)
    { 
        m_eventOffset = 0; 

        m_rootIoSvc->setIndex(m_eventOffset);
        
        m_eventOverlay = dynamic_cast<EventOverlay*>(m_rootIoSvc->getNextEvent("overlay"));
    }

    return StatusCode::SUCCESS;
}

// handle "incidents"
void OverlayInputSvc::handle(const Incident &inc)
{
    if      ( inc.type()=="BeginEvent") beginEvent();
    else if (inc.type()=="EndEvent")    endEvent();
}

void OverlayInputSvc::beginEvent() // should be called at the beginning of an event
{ 
    // At beginning of event free up the DigiEvent if we have one
    if (m_eventOverlay) m_eventOverlay->Clear(m_clearOption.value().c_str());

    m_eventOverlay = 0;

    return;
}

void OverlayInputSvc::endEvent()  // must be called at the end of an event to update, allow pause
{        
    return;
}

void OverlayInputSvc::setNewInputBin(double x) 
{
    MsgStream log(msgSvc(), name());

   try 
   {
        // Close the curent input file(s)
        m_rootIoSvc->closeInput("overlay");

        // Grab the new input file list
        std::vector<std::string> fileList = m_fetch->getFiles(x);

        // Open the new input files
        m_rootIoSvc->prepareRootInput("overlay", 
                                      m_fetch->getTreeName(), 
                                      m_fetch->getBranchName(), 
                                      fileList);

        // Select a random starting position within the allowed number of events
        Long64_t numEventsLong = m_rootIoSvc->getRootEvtMax();
        double   numEvents     = numEventsLong;
        Long64_t startEvent    = (Long64_t)(RandFlat::shoot() * (numEvents - 1));

        // Set that as the starting event
        m_rootIoSvc->setIndex(startEvent);
   } 
   catch(...) 
   {
      log << MSG::WARNING << "exception thrown" << endreq;
      throw;
   }

    return;
}
