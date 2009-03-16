/*
 * @file DigiToOverlayAlg.cxx
 *
 * @brief Drives translation of digi format data to overlay format
 *
 * @author Tracy Usher
 *
 * $Header: /nfs/slac/g/glast/ground/cvs/Overlay/src/Translation/DigiToOverlayAlg.cxx,v 1.1 2008/12/01 22:50:12 usher Exp $
 */

#include "GaudiKernel/Algorithm.h"
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/AlgFactory.h"
#include "GaudiKernel/SmartDataPtr.h"

#include "IDigiToOverlayTool.h"

#include <vector>

class DigiToOverlayAlg : public Algorithm 
{
public:

    DigiToOverlayAlg(const std::string&, ISvcLocator*);
    StatusCode initialize();
    StatusCode execute();
    StatusCode finalize();

private:
    /// Type of tool to run
    std::string         m_type;

    /// Tools to use for translation
    StringArrayProperty m_translateToolNames;

    /// Translation tool list
    std::vector<IDigiToOverlayTool*> m_translateTools;
};

// Used by Gaudi for identifying this algorithm
static const AlgFactory<DigiToOverlayAlg>    Factory;
const IAlgFactory& DigiToOverlayAlgFactory = Factory;

DigiToOverlayAlg::DigiToOverlayAlg(const std::string& name,
                                         ISvcLocator* pSvcLocator)
    : Algorithm(name, pSvcLocator) 
{
    // variable to select the tool type
    declareProperty("Type", m_type="General");

    // Build a list of default tool names
    std::vector<std::string> defaultToolNames;
    defaultToolNames.push_back("EventToOverlayTool");
    defaultToolNames.push_back("TkrDigiToOverlayTool");
    defaultToolNames.push_back("CalXtalToOverlayTool");
    defaultToolNames.push_back("AcdHitToOverlayTool");
    defaultToolNames.push_back("GemToOverlayTool");
    defaultToolNames.push_back("PtToOverlayTool");

    // Property list def
    declareProperty("TranslateToolNames", m_translateToolNames = defaultToolNames);
}


StatusCode DigiToOverlayAlg::initialize() 
{
    // Purpose and Method: initializes DigiToOverlayAlg
    // Inputs: none
    // Outputs: a status code
    // Dependencies: value of m_type determining the type of tool to run
    // Restrictions and Caveats: none

    StatusCode sc = StatusCode::SUCCESS;
    MsgStream log(msgSvc(), name());
    log << MSG::INFO << "initialize" << endreq;

    if ( setProperties().isFailure() ) {
        log << MSG::ERROR << "setProperties() failed" << endreq;
        return StatusCode::FAILURE;
    }

    // Find the tools to do the translation with
    IDigiToOverlayTool* toolPtr = 0;

    // Loop over the tools in our list
    const std::vector<std::string>& toolNameList = m_translateToolNames; 
    for(std::vector<std::string>::const_iterator toolItr  = toolNameList.begin();
                                                 toolItr != toolNameList.end();
                                                 toolItr++)
    {
        if (toolSvc()->retrieveTool(*toolItr, toolPtr).isFailure())
        {
            log << MSG::WARNING << "Failed to retrieve " << *toolItr << " tool" << endreq ;
            return StatusCode::FAILURE ;
        }

        m_translateTools.push_back(toolPtr);
    }

    return sc;
}


StatusCode DigiToOverlayAlg::execute() 
{
    // Purpose and Method: execution method (called once for every event)
    //                     Doesn't do anything but calls the chosen tool.
    // Inputs: none
    // Outputs: a status code
    // Dependencies: none
    // Restrictions and Caveats: none

    StatusCode sc = StatusCode::SUCCESS; 
    MsgStream log(msgSvc(), name());
    log << MSG::DEBUG << "execute" << endreq;

    // Translate the TkrDigis to TkrOverlay objects
    for(std::vector<IDigiToOverlayTool*>::iterator toolItr  = m_translateTools.begin(); 
                                                   toolItr != m_translateTools.end();
                                                   toolItr++)
    {
        (*toolItr)->translate();
    }

    return sc;
}

StatusCode DigiToOverlayAlg::finalize() 
{
    MsgStream log(msgSvc(), name());
    log << MSG::INFO << "finalize" << endreq;
    return StatusCode::SUCCESS;
}
