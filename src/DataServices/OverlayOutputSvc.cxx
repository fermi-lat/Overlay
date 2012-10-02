// $Header: /nfs/slac/g/glast/ground/cvs/Overlay/src/DataServices/OverlayOutputSvc.cxx,v 1.4 2011/12/12 20:54:54 heather Exp $

// Include files
#include "GaudiKernel/Service.h"
#include "GaudiKernel/SvcFactory.h"
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/IIncidentSvc.h"
#include "GaudiKernel/IIncidentListener.h"
#include "GaudiKernel/ISvcLocator.h"
#include "GaudiKernel/IToolSvc.h"
#include "GaudiKernel/IDataProviderSvc.h"
#include "GaudiKernel/IConverter.h"


#include "OverlayEvent/OverlayEventModel.h"
#include "OverlayEvent/EventOverlay.h"

#include "RootIo/IRootIoSvc.h"
#include "overlayRootData/EventOverlay.h"
#include "facilities/Util.h"

#include "Overlay/IOverlayDataSvc.h"

class OverlayOutputSvc : virtual public IOverlayDataSvc, virtual public IIncidentListener, public Service
{    
public:
    OverlayOutputSvc(const std::string& name, ISvcLocator* svc);
    virtual ~OverlayOutputSvc();

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
    virtual StatusCode registerOutputPath(const std::string& path);

    /// For output service, set store events flag
    virtual void storeEvent(bool flag) {m_saveEvent = flag;}

    /// For output service, return current value of store events flag
    virtual bool getStoreEventFlag() {return m_saveEvent;}

    /// Handles incidents, implementing IIncidentListener interface
    virtual void handle( const Incident & ) ;    

private:
    void beginEvent() ;
    void endEvent() ;

    /// access the RootIoSvc to get the CompositeEventList ptr
    IRootIoSvc *      m_rootIoSvc;

    /// Event data service
    IDataProviderSvc* m_eventDataSvc;

    /// List of objects to store (from converters
    std::vector<std::string> m_objectList;

    // Pointer to input data
    EventOverlay*     m_eventOverlay;

    /// name of the output ROOT file
    std::string       m_fileName;
    /// name of the TTree in the ROOT file
    std::string       m_treeName;
    /// ROOT split mode
    int               m_splitMode;
    /// Buffer Size for the ROOT file
    int               m_bufSize;
    /// Compression level for the ROOT file
    int               m_compressionLevel;
    /// Flag to specify whether event is to be written at end of event
    bool              m_saveEvent;
};


// Instantiation of a static factory class used by clients to create
// instances of this service
//static SvcFactory<OverlayOutputSvc> s_factory;
//const ISvcFactory& OverlayOutputSvcFactory = s_factory;
DECLARE_SERVICE_FACTORY(OverlayOutputSvc);

/// Standard Constructor
OverlayOutputSvc::OverlayOutputSvc(const std::string& name,ISvcLocator* svc) : Service(name,svc),
                               m_rootIoSvc(0), m_eventOverlay(0), m_saveEvent(false)
{
    // Input pararmeters that may be set via the jobOptions file
    // Input ROOT file name  provided for backward compatibility, digiRootFileList is preferred
    // The files will be ignored if RootIoSvc is provided a meta (event collection) ROOT file
    // Input parameters available to be set via the jobOptions file
    declareProperty("overlayRootFile",  m_fileName         = "$(GLEAMDATAPATH)/overlay.root");
    declareProperty("splitMode",        m_splitMode        = 1);
    declareProperty("bufferSize",       m_bufSize          = 64000);
    // ROOT default compression
    declareProperty("compressionLevel", m_compressionLevel = 1);
    declareProperty("treeName",         m_treeName         = "Overlay");

    m_objectList.clear();

    return;
}

/// Standard Destructor
OverlayOutputSvc::~OverlayOutputSvc()  
{
}

// Service initialization
StatusCode OverlayOutputSvc::initialize()   
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

    // Use the RootIoSvc to setup our output ROOT files
    m_rootIoSvc->prepareRootOutput("OverlayOut", m_fileName, m_treeName, m_compressionLevel, "GLAST Digitization Data");
    m_eventOverlay = new EventOverlay();
    m_rootIoSvc->setupBranch("OverlayOut", "EventOverlay", "EventOverlay", &m_eventOverlay, m_bufSize, m_splitMode);

    // Get pointer to the ToolSvc
    IService* tmpToolSvc = 0;
    status = service("ToolSvc", tmpToolSvc, true);
    if (status.isFailure())
    {
        log << MSG::ERROR << "Can't find the tool service! " << endreq;
        return status;
    }

    // Find a pointer to the event data service
    status = service("EventDataSvc", m_eventDataSvc, true);
    if (status .isFailure() ) 
    {
        log << MSG::ERROR << "Unable to find EventDataSvc " << endreq;
        return status;
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
StatusCode OverlayOutputSvc::finalize()
{
    MsgStream log(msgSvc(), name());

    log << MSG::DEBUG << "Finalizing" << endreq;

    m_rootIoSvc->closeFile("OverlayOut");

    // Finalize the base class
    return StatusCode::SUCCESS;
}

StatusCode OverlayOutputSvc::queryInterface(const InterfaceID& riid, void** ppvInterface)
{
    StatusCode status = StatusCode::SUCCESS;

//    if ( IID_IOverlayDataSvc.versionMatch(riid) )  {
//        *ppvInterface = (IOverlayDataSvc*)this;
//    }
//    else status = Service::queryInterface(riid, ppvInterface);
	status = Service::queryInterface(riid, ppvInterface);

    return status;
}

EventOverlay* OverlayOutputSvc::getRootEventOverlay()
{
    return m_eventOverlay;
}

StatusCode OverlayOutputSvc::selectNextEvent()
{
    MsgStream log(msgSvc(), name());

    return StatusCode::SUCCESS;
}

StatusCode OverlayOutputSvc::registerOutputPath(const std::string& path)
{
    m_objectList.push_back(path);

    return StatusCode::SUCCESS;
}

// handle "incidents"
void OverlayOutputSvc::handle(const Incident &inc)
{
    if      ( inc.type()=="BeginEvent") beginEvent();
    else if (inc.type()=="EndEvent")    endEvent();
}

void OverlayOutputSvc::beginEvent() // should be called at the beginning of an event
{ 
    // At beginning of event free up the DigiEvent if we have one
    m_eventOverlay->Clear();

    // Assume all events are saved
    m_saveEvent = true;

    return;
}

void OverlayOutputSvc::endEvent()  // must be called at the end of an event to update, allow pause
{
    // Should event be saved?
    if (m_saveEvent)
    {
        // Get the converstion service with an IConverter* interface
        IConverter* cnvService = 0;
        if (serviceLocator()->getService("EventCnvSvc", IConverter::interfaceID(), (IInterface*&)cnvService).isFailure())
        {
            MsgStream log(msgSvc(), name());
            log << MSG::ERROR << "Could not retrieve the EventCnvSvc! " << endreq;
            return;
        }

        // For now, go through the list and make sure the objects have been converted...
        for(std::vector<std::string>::iterator dataIter = m_objectList.begin();
                                               dataIter != m_objectList.end();
                                               dataIter++)
        {
            std::string path = *dataIter;

            DataObject* object = 0;
            StatusCode status = m_eventDataSvc->retrieveObject(path, object);
        }

        // Now do a clear of the root DigiEvent root object
        m_eventOverlay->Clear();

        // Go through list of data objects and "convert" them...
        for(std::vector<std::string>::iterator dataIter = m_objectList.begin();
                                               dataIter != m_objectList.end();
                                               dataIter++)
        {
            std::string path = *dataIter;

            DataObject* object = 0;
            StatusCode status = m_eventDataSvc->retrieveObject(path, object);

            IOpaqueAddress* address = 0;
            status = cnvService->createRep(object, address);
        }

        // Now fill the root tree for this event
        m_rootIoSvc->fillTree("OverlayOut");
    }

    return;
}
