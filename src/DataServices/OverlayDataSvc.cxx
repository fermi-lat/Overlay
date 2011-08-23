//====================================================================
//	OverlayDataSvc.cpp
//--------------------------------------------------------------------
//
//	Package    : System ( The LHCb Offline System)
//
//  Description: implementation of the Transient event data service.
//
//	Author     : M.Frank
//  History    :
// +---------+----------------------------------------------+---------
// |    Date |                 Comment                      | Who     
// +---------+----------------------------------------------+---------
// | 29/10/98| Initial version                              | MF
// +---------+----------------------------------------------+---------
//
//====================================================================
#define  DATASVC_OverlayDataSvc_CPP

#include "GaudiKernel/SvcFactory.h"
#include "GaudiKernel/ISvcLocator.h"
#include "GaudiKernel/IAddressCreator.h"
#include "GaudiKernel/IOpaqueAddress.h"
#include "GaudiKernel/IIncidentSvc.h"
#include "GaudiKernel/IIncidentListener.h"
#include "GaudiKernel/IToolSvc.h"
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/DataSvc.h"
#include "GaudiKernel/ConversionSvc.h"

#include "OverlayEvent/OverlayEventModel.h"

#include "RootIo/IRootIoSvc.h"
#include "OverlayEvent/OverlayEventModel.h"
#include "overlayRootData/EventOverlay.h"
#include "facilities/Util.h"

#include "CLHEP/Random/RandFlat.h"

#include "../InputControl/XmlFetchEvents.h"
#include "Overlay/IOverlayDataSvc.h"
#include "Overlay/IBackgroundBinTool.h"
#include "Overlay/IFetchEvents.h"

/** @class OverlayDataSvc OverlayDataSvc.h
 * 
 *   A OverlayDataSvc is the base class for event services
 *
 *  @author M.Frank
 */
class OverlayDataSvc  : virtual public IOverlayDataSvc, virtual public IIncidentListener, public DataSvc   
{
    friend class SvcFactory<OverlayDataSvc>;

public:
    virtual StatusCode initialize();
    virtual StatusCode reinitialize();
    virtual StatusCode finalize();

    /// Get pointer to a Root DigiEvent object
    virtual EventOverlay* getRootEventOverlay();

    /// Select the next event
    virtual StatusCode selectNextEvent();

    /// Register an output path with us
    virtual StatusCode registerOutputPath(const std::string&);

    /// For output service, set store events flag
    virtual void storeEvent(bool) {return;}

    /// For output service, return current value of store events flag
    virtual bool getStoreEventFlag() {return true;}

    /// Handles incidents, implementing IIncidentListener interface
    virtual void handle( const Incident & ) ; 

protected:
    /// Standard Constructor
    OverlayDataSvc(const std::string& name, ISvcLocator* svc);

    /// Standard Destructor
    virtual ~OverlayDataSvc();
private:

    void beginEvent() ;
    void endEvent() ;

    /**@brief Retrieve the correct tree for current value of the variable(s) so copyEvent comes from correct sample bin
    */
    void setNewInputBin(double val);

    /// access the RootIoSvc to get the CompositeEventList ptr
    IRootIoSvc *                       m_rootIoSvc;

    // Flags to indicate whether to configure for input or output (but not both)
    bool                               m_configureForInput;
    bool                               m_configureForOutput;

    // Use a map to keep track of the input files... we'll keep them open until the 
    // end of the job
    std::map<std::string, std::string> m_inputFileMap;

    // Hopefully a temporary kludge until RootIo can handle this
    std::map<std::string, long long>   m_inputIndexMap;
    
    std::string                        m_curFileType;

    // Pointer to input data
    EventOverlay*                      m_eventOverlay;
    EventOverlay                       m_myOverlay;
    EventOverlay*                      m_myOverlayPtr;

    //***** INPUT SPECIFIC VARIABLES HERE *****
    // flag to signal that we need to read the current event
    IFetchEvents*                      m_fetch;       ///< abstract guy that processes the xml file

    // Pointer to the object which determines which bin we are in
    IBackgroundBinTool*                m_binTool;

    bool                               m_needToReadEvent;

    StringProperty                     m_inputXmlFilePath;

    /// Option string which will be passed to McEvent::Clear
    StringProperty                     m_clearOption;

    /// Name of the Background Overlay tool to use
    StringProperty                     m_overlay; 

    //***** OUTPUT SPECIFIC VARIABLES HERE *****

    /// List of objects to store (from converters
    std::vector<std::string>           m_objectList;

    /// name of the output ROOT file
    std::string                        m_outputFileName;
    /// name of the TTree in the ROOT file
    std::string                        m_treeName;
    /// ROOT split mode
    int                                m_splitMode;
    /// Buffer Size for the ROOT file
    int                                m_bufSize;
    /// Compression level for the ROOT file
    int                                m_compressionLevel;
    /// Flag to specify whether event is to be written at end of event
    bool                               m_saveEvent;

    IConversionSvc*                    m_cnvSvc;
    std::string                        m_persistencySvcName;

    /// Define a map to relate the class ID to the path for this data service
    typedef std::map<const CLID, std::string> ClidToPathMap;

    ClidToPathMap                      m_clidToPathMap;
};

// Instantiation of a static factory class used by clients to create
// instances of this service
static SvcFactory<OverlayDataSvc> s_factory;
const ISvcFactory& OverlayDataSvcFactory = s_factory;

/// Standard Constructor
OverlayDataSvc::OverlayDataSvc(const std::string& name,ISvcLocator* svc) 
: DataSvc(name,svc) , m_cnvSvc(0),
               m_rootIoSvc(0), m_curFileType(""), m_eventOverlay(0), m_myOverlayPtr(&m_myOverlay), m_needToReadEvent(true)
{
    declareProperty("ConfigureForInput",  m_configureForInput  = false);
    declareProperty("ConfigureForOutput", m_configureForOutput = false);

    // The following parameters are oriented to input
    declareProperty("OverlayTool",        m_overlay            = "McIlwain_L");
    declareProperty("InputXmlFilePath",   m_inputXmlFilePath   = "$(OVERLAYROOT)/xml");
    declareProperty("clearOption",        m_clearOption        = "");
    declareProperty("RootName",           m_rootName           = OverlayEventModel::OverlayEventHeader);
    declareProperty("PersistencySvcName", m_persistencySvcName = "OverlayPersistencySvc");

    // The following parameters are oriented to output
    declareProperty("overlayRootFile",    m_outputFileName     = "overlay.root");
    declareProperty("splitMode",          m_splitMode          = 1);
    declareProperty("bufferSize",         m_bufSize            = 64000);
    // ROOT default compression
    declareProperty("compressionLevel",   m_compressionLevel   = 1);
    declareProperty("treeName",           m_treeName           = "Overlay");

    m_objectList.clear();

    m_inputFileMap.clear();
    m_inputIndexMap.clear();
    m_clidToPathMap.clear();

    return;
}

/// Standard Destructor
OverlayDataSvc::~OverlayDataSvc()  
{
    return;
}


/// Service initialisation
StatusCode OverlayDataSvc::initialize()    
{
    MsgStream log(msgSvc(), name());

    // Make sure our JO's get set
    setProperties();

    // Nothing to do: just call base class initialisation
    StatusCode      status  = DataSvc::initialize();
    ISvcLocator*    svc_loc = serviceLocator();

    // If not configured for input or output (and not both) the signal an error and crash ungracefully
    if (!(m_configureForInput || m_configureForOutput) || (m_configureForInput && m_configureForOutput))
    {
        log << MSG::ERROR << "service incorrectly configured for input and/or output!" << endreq;
        return StatusCode::FAILURE;
    }

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

    // Do the following if we have been configured for input
    if (m_configureForInput)
    {
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
    }
    // Otherwise we configure for output
    else
    {
        // Use the RootIoSvc to setup our output ROOT files
        m_rootIoSvc->prepareRootOutput("OverlayOut", m_outputFileName, m_treeName, m_compressionLevel, "GLAST Digitization Data");
        m_eventOverlay = new EventOverlay();
        m_rootIoSvc->setupBranch("OverlayOut", "EventOverlay", "EventOverlay", &m_eventOverlay, m_bufSize, m_splitMode);
    }

    // use the incident service to register begin, end events
    IIncidentSvc* incsvc = 0;
    status = service ("IncidentSvc", incsvc, true);

    if( status.isFailure() ) return status;

    incsvc->addListener(this, "BeginEvent", 100);
    incsvc->addListener(this, "EndEvent", 0);

    // Attach data loader facility
    status = svc_loc->service(m_persistencySvcName, m_cnvSvc, true);
    status = setDataLoader( m_cnvSvc );

    // Convention for multiple input overlay files is that there will be separate OverlayDataSvc's with 
    // names appended by "_xx", for example OverlayDataSvc_1 for the second input file. 
    // In order to ensure the data read in goes into a unique section of the TDS we need to modify the 
    // base root path, which we do by examining the name of the service
    int subPos = name().rfind("_");
    std::string nameEnding = subPos > 0 ? name().substr(subPos, name().length() - subPos) : "";

    if (nameEnding != "")
    {
        m_rootName = m_rootName + nameEnding;
    }

    return status;
}
/// Service reinitialisation
StatusCode OverlayDataSvc::reinitialize()    
{
    // Do nothing for this service
    return StatusCode::SUCCESS;
}
/// Service finalization
StatusCode OverlayDataSvc::finalize()    
{
    if( m_cnvSvc ) m_cnvSvc->release();
    m_cnvSvc = 0;

    // Do the following if configured for input
    if (m_configureForInput)
    {
        // Loop through any open files and close them
        for(std::map<std::string,std::string>::iterator fileMapItr = m_inputFileMap.begin();
            fileMapItr != m_inputFileMap.end(); fileMapItr++)
        {
            m_rootIoSvc->closeInput(fileMapItr->second);
        }

        delete m_fetch;
    }
    // Otherwise, do the output finalization
    else 
    {
        m_rootIoSvc->closeFile("OverlayOut");
    }

    DataSvc::finalize();

    return StatusCode::SUCCESS;
}

// Below here implements the data input section...

EventOverlay* OverlayDataSvc::getRootEventOverlay()
{
    if (m_needToReadEvent && m_configureForInput) selectNextEvent();

    return m_eventOverlay;
}

StatusCode OverlayDataSvc::selectNextEvent()
{
    MsgStream log(msgSvc(), name());

    if (m_configureForInput)
    {
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
            
            m_eventOverlay = dynamic_cast<EventOverlay*>(m_rootIoSvc->getNextEvent(m_curFileType, inputIndex));
        }

        // Set flag to indicate we have read the event
        m_needToReadEvent = false;
    }

    return StatusCode::SUCCESS;
}

StatusCode OverlayDataSvc::registerOutputPath(const std::string& path)
{
    m_objectList.push_back(path);

    return StatusCode::SUCCESS;
}

// handle "incidents"
void OverlayDataSvc::handle(const Incident &inc)
{
    if      ( inc.type()=="BeginEvent") beginEvent();
    else if (inc.type()=="EndEvent")    endEvent();
}

void OverlayDataSvc::beginEvent() // should be called at the beginning of an event
{ 
    // What we do depends on our configuration
    if (m_configureForInput)
    {
        // At beginning of event we need to clear our EventOverlay object
        m_myOverlay.Clear(m_clearOption.value().c_str());

        // Set the flag to indicate the need to input the next event
        m_needToReadEvent = true;
    }
    else
    {
        // At beginning of event free up the DigiEvent if we have one
        m_eventOverlay->Clear();

        // Assume all events are saved
        m_saveEvent = true;
    }

    return;
}

void OverlayDataSvc::endEvent()  // must be called at the end of an event to update, allow pause
{ 
    if (m_configureForOutput)
    {
        // Should event be saved?
        if (m_saveEvent)
        {
            // For now, go through the list and make sure the objects have been converted...
            for(std::vector<std::string>::iterator dataIter = m_objectList.begin();
                                                   dataIter != m_objectList.end();
                                                   dataIter++)
            {
                std::string path = *dataIter;

                DataObject* object = 0;
                StatusCode status = retrieveObject(path, object);
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
                StatusCode status = retrieveObject(path, object);

                IOpaqueAddress* address = 0;
                status = m_cnvSvc->createRep(object, address);
            }

            // Now fill the root tree for this event
            m_rootIoSvc->fillTree("OverlayOut");
        }
    }

    return;
}

void OverlayDataSvc::setNewInputBin(double x) 
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

            rootType << m_rootName << "_" << m_inputFileMap.size();

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
            long long startEvent    = (long long)(CLHEP::RandFlat::shoot() * (numEvents - 1));
            //Long64_t startEvent    = (Long64_t)(CLHEP::RandFlat::shoot() * (numEvents - 1));

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

