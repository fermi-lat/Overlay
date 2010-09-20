// $Header: /nfs/slac/g/glast/ground/cvs/Overlay/src/DataServices/OverlayInputSvc.cxx,v 1.6 2009/09/15 19:20:04 usher Exp $

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
    virtual StatusCode registerOutputPath(const std::string&) {return StatusCode::SUCCESS;}

    /// For output service, set store events flag
    virtual void storeEvent(bool) {return;}

    /// For output service, return current value of store events flag
    virtual bool getStoreEventFlag() {return true;}

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

    IFetchEvents*       m_fetch;       ///< abstract guy that processes the xml file

    // Pointer to the object which determines which bin we are in
    IBackgroundBinTool* m_binTool;

    // Use a map to keep track of the input files... we'll keep them open until the 
    // end of the job
    std::map<std::string, std::string> m_inputFileMap;

    // Hopefully a temporary kludge until RootIo can handle this
    std::map<std::string, long long>   m_inputIndexMap;
    
    std::string         m_curFileType;

    // Pointer to input data
    EventOverlay*       m_eventOverlay;
    EventOverlay        m_myOverlay;
    EventOverlay*       m_myOverlayPtr;

    // flag to signal that we need to read the current event
    bool                m_needToReadEvent;

    StringProperty      m_inputXmlFilePath;

    /// Option string which will be passed to McEvent::Clear
    StringProperty      m_clearOption;

    /// Name of the Background Overlay tool to use
    StringProperty      m_overlay; 
};


// Instantiation of a static factory class used by clients to create
// instances of this service
//static SvcFactory<OverlayInputSvc> s_factory;
//const ISvcFactory& OverlayInputSvcFactory = s_factory;
DECLARE_SERVICE_FACTORY(OverlayInputSvc);

/// Standard Constructor
OverlayInputSvc::OverlayInputSvc(const std::string& name,ISvcLocator* svc) : Service(name,svc),
                               m_rootIoSvc(0), m_curFileType(""), m_eventOverlay(0), m_myOverlayPtr(&m_myOverlay), m_needToReadEvent(true)
{
    // Input pararmeters that may be set via the jobOptions file
    // Input ROOT file name  provided for backward compatibility, digiRootFileList is preferred
    // The files will be ignored if RootIoSvc is provided a meta (event collection) ROOT file

    declareProperty("OverlayTool",      m_overlay="McIlwain_L");
    declareProperty("InputXmlFilePath", m_inputXmlFilePath="$(OVERLAYROOT)/xml");
    declareProperty("clearOption",      m_clearOption="");

    m_inputFileMap.clear();
    m_inputIndexMap.clear();

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

    // Loop through any open files and close them
    for(std::map<std::string,std::string>::iterator fileMapItr = m_inputFileMap.begin();
        fileMapItr != m_inputFileMap.end(); fileMapItr++)
    {
        m_rootIoSvc->closeInput(fileMapItr->second);
    }

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
    if (m_needToReadEvent) selectNextEvent();

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
    if (!m_needToReadEvent)
    {
        log << MSG::ERROR << "Found non-zero pointer to EventOverlay during event loop!" << endreq;
        return StatusCode::FAILURE;
    }

    // Retrieve and increment the index (and, by definition, it exists!)
    std::map<std::string, long long>::iterator inputIndexIter = m_inputIndexMap.find(m_curFileType);

    long long inputIndex = inputIndexIter->second;

    // update the input index
    inputIndexIter->second = inputIndex + 1;

    // Try reading the event this way... 
    // using treename as the key
    m_eventOverlay = dynamic_cast<EventOverlay*>(m_rootIoSvc->getNextEvent(m_curFileType, inputIndex));

    // If the call returns a null pointer then most likely we have hit the end of file
    // Try to wrap back to first event and try again
    if( m_eventOverlay == 0)
    { 
        long long inputIndex = 0;

        m_inputIndexMap[m_curFileType] = inputIndex;

        m_rootIoSvc->setIndex(inputIndex);
        
        m_eventOverlay = dynamic_cast<EventOverlay*>(m_rootIoSvc->getNextEvent(m_curFileType));
    }

    // Set flag to indicate we have read the event
    m_needToReadEvent = false;

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
    // At beginning of event we need to clear our EventOverlay object
    m_myOverlay.Clear(m_clearOption.value().c_str());

    // Set the flag to indicate the need to input the next event
    m_needToReadEvent = true;

    return;
}

void OverlayInputSvc::endEvent()  // must be called at the end of an event to update, allow pause
{        
    return;
}

void OverlayInputSvc::setNewInputBin(double x) 
{
    MsgStream log(msgSvc(), name());

    // Zero the pointer to the input data
    m_eventOverlay = 0;

    // Close the curent input file(s)
    //m_rootIoSvc->closeInput("overlay"); // Not closing files here

    // Grab the new input file list
    std::vector<std::string> fileList = m_fetch->getFiles(x);

    std::string fileName = fileList[0];

    if (m_inputFileMap.find(fileName) == m_inputFileMap.end())
    {
        try 
        {
            // Create a rootIoSvc "type" name to identify this file to RootIoSvc
            std::stringstream rootType;

            rootType << "overlay_" << m_inputFileMap.size();

            // Set it as our "current" file type
            m_curFileType = rootType.str();

            // And store this away in our map of opened files
            m_inputFileMap[fileName] = m_curFileType;

            // Open the new input files
            m_rootIoSvc->prepareRootInput(m_curFileType, 
                                          m_fetch->getTreeName(), 
                                          m_fetch->getBranchName(),
                                         (TObject**)&m_myOverlayPtr,
                                          fileList);

            // Select a random starting position within the allowed number of events
            long long numEventsLong = m_rootIoSvc->getRootEvtMax(m_curFileType);
            double    numEvents     = numEventsLong;
            long long startEvent    = (long long)(RandFlat::shoot() * (numEvents - 1));
            //Long64_t startEvent    = (Long64_t)(RandFlat::shoot() * (numEvents - 1));

            // Set the index in our local map
            m_inputIndexMap[m_curFileType] = startEvent;

            // Set that as the starting event
            //m_rootIoSvc->setIndex(startEvent);
        } 
        catch(...) 
        {
            log << MSG::WARNING << "exception thrown" << endreq;
            throw;
        }
    }
    else
    {
        m_curFileType = m_inputFileMap[fileName];
    }

    return;
}
