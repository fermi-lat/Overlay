/**  @file BackgroundSelectTool.cxx
    @brief implementation of class BackgroundSelectTool
    
  $Header: /nfs/slac/g/glast/ground/cvs/Interleave/src/BackgroundSelectTool.cxx,v 1.19 2008/06/17 22:13:59 kocian Exp $  
*/

#include "IBackgroundSelectTool.h"
#include "IBackgroundBinTool.h"
#include "IFetchEvents.h"

#include "GaudiKernel/ToolFactory.h"
#include "GaudiKernel/AlgTool.h"
#include "GaudiKernel/SmartDataPtr.h"
#include "GaudiKernel/GaudiException.h" 
#include "GaudiKernel/IDataProviderSvc.h"

#include "digiRootData/DigiEvent.h"

#include "facilities/Util.h"

#include "CLHEP/Random/RandFlat.h"

// access to the RootIo
#include "RootIo/IRootIoSvc.h"

#include "XmlFetchEvents.h"

#include <stdexcept>
#include <cassert>
#include <sstream>
#include <cmath>
#include <vector>
#include <list>
#include <ctime>

/** @class BackgroundSelection
    @brief manage the selection of background events to merge with signal events
    @author Dan Flath

It uses the name of a tuple variable as a key to look up trigger and downlink rates of a background
source depending on the value of that variable. See the corresponding member functions. Given such
a value, it expects to find a source of such events generated with that value which can be inserted 
into the Gleam output tuple.

*/
class BackgroundSelectTool : public AlgTool, virtual public IBackgroundSelectTool
{
public:

    // Standard Gaudi Tool constructor
    BackgroundSelectTool(const std::string& type, const std::string& name, const IInterface* parent);

    // After building it, destroy it
    ~BackgroundSelectTool();

    /// @brief Intialization of the tool
    StatusCode initialize();

    /// @brief Finalize method for the tool
    StatusCode finalize();

    /** @brief select an event and copy the contents to the output tree
    */
    DigiEvent* selectEvent();

private:

    /**@brief Retrieve the correct tree for current value of the variable(s) so copyEvent comes from correct sample bin
    */
    void setNewInputBin(double val);

    /// access the RootIoSvc to get the CompositeEventList ptr
    IRootIoSvc *         m_rootIoSvc;

    unsigned int         m_eventOffset;

    IFetchEvents*        m_fetch;       ///< abstract guy that processes the xml file

    // Pointer to the object which determines which bin we are in
    IBackgroundBinTool*  m_binTool;

    // Pointer to input data
    DigiEvent*           m_digiEvent;

    StringProperty       m_inputXmlFile;

    /// Option string which will be passed to McEvent::Clear
    StringProperty       m_clearOption;
};

static ToolFactory<BackgroundSelectTool> s_factory;
const IToolFactory& BackgroundSelectToolFactory = s_factory;

//------------------------------------------------------------------------
BackgroundSelectTool::BackgroundSelectTool(const std::string& type, 
                                 const std::string& name, 
                                 const IInterface* parent) :
                                 AlgTool(type, name, parent)
                                 , m_rootIoSvc(0)
                                 , m_eventOffset(0)
                                 , m_fetch(0)
                                 , m_digiEvent(0)
{
    //Declare the additional interface
    declareInterface<IBackgroundSelectTool>(this);

    // declare properties with setProperties calls
    declareProperty("InputXmlFile", m_inputXmlFile="$(OVERLAYROOT)/xml/McIlwain_L.xml");
    declareProperty("clearOption",  m_clearOption="");

    std::string varName = this->name();
}
//------------------------------------------------------------------------
BackgroundSelectTool::~BackgroundSelectTool()
{
    delete m_fetch;
}

StatusCode BackgroundSelectTool::initialize()
{
    StatusCode sc   = StatusCode::SUCCESS;
    MsgStream log(msgSvc(), name());

    // Set the properties
    setProperties();

    sc = service("RootIoSvc", m_rootIoSvc, true);       
    if( sc.isFailure() ) 
    {
        log << MSG::ERROR << "failed to get the RootIoSvc" << endreq;
        return sc;
    }

    if( !m_inputXmlFile.value().empty())
    {
        std::string xmlFile = m_inputXmlFile.value();

        facilities::Util::expandEnvVar(&xmlFile);
        log << MSG::INFO << "Using xml file " << xmlFile 
            << " for BackgroundSelectTool " + name() + "." << endreq;

        // Extract the "parameter" name from the Tool name string
        std::string toolName = this->name();
        int         dotPos   = toolName.find(".");
        std::string param    = dotPos > 0 ? toolName.substr(dotPos+1,toolName.size()) : toolName;

        // Ok, set up the xml reading object
        m_fetch = new XmlFetchEvents(xmlFile, param);

        // Build the tool name from the param name
        std::string binToolName = param + "_Tool";

        // Now look it up
        if (toolSvc()->retrieveTool(binToolName, "BinTool", m_binTool).isFailure())
        {
            log << MSG::INFO << "Couldn't find the BinTool: " << binToolName << endreq;
            sc = StatusCode::FAILURE;
        }
    }

    return sc;
}

StatusCode BackgroundSelectTool::finalize ()
{
    StatusCode  status = StatusCode::SUCCESS;

    delete m_fetch;
    
    return status;
}

//------------------------------------------------------------------------
DigiEvent* BackgroundSelectTool::selectEvent()
{
    MsgStream log(msgSvc(), name());

    // Embed in try catch loop to catch various root problems
    try 
    {
        // Get the desired bin
        double x = m_binTool->value();

        // First check that the returned value is within range
        if( !m_fetch->isValid(x) )
        {
            std::stringstream msg;
            msg << "BackgroundSelectTool::selectEvent: called with " << name() 
                <<" = "<< x << " is not in range " 
                <<  m_fetch->minValFullRange() << ", to " 
                <<  m_fetch->maxValFullRange() << std::endl;

            throw std::runtime_error(msg.str());
        }

        // make sure we have the right tree selected for new value    
        // if still valid, do not change
        if( !m_fetch->isCurrent(x) )
        {
            // New bin, set new tree
            setNewInputBin(x);   
        }

        // grab the next event, cycling if needed
        if (m_digiEvent) m_digiEvent->Clear(m_clearOption.value().c_str());

        m_digiEvent = 0;

        // Try reading the event this way... 
        // using treename as the key
        m_digiEvent = dynamic_cast<DigiEvent*>(m_rootIoSvc->getNextEvent("overlay"));

        // If the call returns a null pointer then most likely we have hit the end of file
        // Try to wrap back to first event and try again
        if( m_digiEvent == 0)
        { 
            m_eventOffset = 0; 

            m_rootIoSvc->setIndex(m_eventOffset);
        
            m_digiEvent = dynamic_cast<DigiEvent*>(m_rootIoSvc->getNextEvent("overlay"));
        }
    } 
    catch(...) 
    {
        log << MSG::WARNING << "exception thrown in BackgroundSelectTool" << endreq;
        throw;
    }

    return m_digiEvent;
}

//------------------------------------------------------------------------
void BackgroundSelectTool::setNewInputBin(double x) 
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

        // ****TEST*****
        startEvent = numEventsLong - 2;

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
