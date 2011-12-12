/*
 * @file DiagDataOverlayMergeAlg.cxx
 *
 * @brief Calls an user-chosen tool to merge GemOverlay from input overlay files
 *
 * @author Tracy Usher
 *
 * $Header: /nfs/slac/g/glast/ground/cvs/GlastRelease-scons/Overlay/src/MergeAlgs/DiagDataOverlayMergeAlg.cxx,v 1.4 2011/06/27 17:45:57 usher Exp $
 */


#include "GaudiKernel/Algorithm.h"
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/AlgFactory.h"
#include "GaudiKernel/SmartDataPtr.h"
#include "GaudiKernel/DataSvc.h"

#include "Event/TopLevel/EventModel.h"
#include "OverlayEvent/OverlayEventModel.h"
#include "OverlayEvent/DiagDataOverlay.h"

#include "LdfEvent/DiagnosticData.h"

#include "Event/Recon/TkrRecon/TkrDiagnosticFlag.h"

#include <map>

class DiagDataOverlayMergeAlg : public Algorithm 
{
 public:

    DiagDataOverlayMergeAlg(const std::string&, ISvcLocator*);
    StatusCode initialize();
    StatusCode execute();
    StatusCode finalize();

 private:
    /// Pointer to the overlay's data provider service
    DataSvc*         m_dataSvc;

    /// Type of tool to run
    std::string m_type;
};

// Used by Gaudi for identifying this algorithm
//static const AlgFactory<DiagDataOverlayMergeAlg>    Factory;
//const IAlgFactory& DiagDataOverlayMergeAlgFactory = Factory;
DECLARE_ALGORITHM_FACTORY(DiagDataOverlayMergeAlg);

DiagDataOverlayMergeAlg::DiagDataOverlayMergeAlg(const std::string& name,
                                         ISvcLocator* pSvcLocator)
    : Algorithm(name, pSvcLocator) 
{
    // variable to select the tool type
    declareProperty("Type", m_type="General");
}


StatusCode DiagDataOverlayMergeAlg::initialize() 
{
    // Purpose and Method: initializes DiagDataOverlayMergeAlg
    // Inputs: none
    // Outputs: a status code
    // Dependencies: value of m_type determining the type of tool to run
    // Restrictions and Caveats: none

    StatusCode sc = StatusCode::SUCCESS;
    MsgStream log(msgSvc(), name());
    log << MSG::INFO << "initialize" << endreq;

    if ( setProperties().isFailure() ) 
    {
        log << MSG::ERROR << "setProperties() failed" << endreq;
        return StatusCode::FAILURE;
    }

    // Convention for multiple input overlay files is that there will be separate OverlayDataSvc's with 
    // names appended by "_xx", for example OverlayDataSvc_1 for the second input file. 
    // In order to ensure the data read in goes into a unique section of the TDS we need to modify the 
    // base root path, which we do by examining the name of the service
    std::string dataSvcName = "OverlayDataSvc";
    int         subPos      = name().rfind("_");
    std::string nameEnding  = subPos > 0 ? name().substr(subPos, name().length() - subPos) : "";

    if (nameEnding != "") dataSvcName += nameEnding;

    IService* dataSvc = 0;
    sc = service(dataSvcName, dataSvc);
    if (sc.isFailure() ) {
        log << MSG::ERROR << "  can't get OverlayDataSvc " << endreq;
        return sc;
    }

    // Caste back to the "correct" pointer
    m_dataSvc = dynamic_cast<DataSvc*>(dataSvc);

    return sc;
}

typedef std::pair<unsigned, unsigned>   DiagKeyPair;
typedef std::map<DiagKeyPair, unsigned> OverDiagMap;

StatusCode DiagDataOverlayMergeAlg::execute() 
{
    // Purpose and Method: Is used to overlay DiagnosticData data
    //                     on top of simulated DiagnosticData
    // Inputs: none
    // Outputs: a status code
    // Dependencies: none
    // Restrictions and Caveats: none

    StatusCode sc = StatusCode::SUCCESS; 

    SmartDataPtr<Event::TkrDiagnosticFlag> diagFlag(eventSvc(), EventModel::TkrRecon::TkrDiagnosticFlag);
    // if no diagFlag, do the default (full diagnostics), otherwise check the flag
    if(diagFlag) {
        bool doDiag = diagFlag->getDiagnosticFlag();
        if (!doDiag) return sc;
    }

    MsgStream log(msgSvc(), name());
    log << MSG::DEBUG << "execute" << endreq;

    // First, recover any overlay diagnostic data, to see if we have anything to do
    SmartDataPtr<Event::DiagDataOverlay> diagDataOverlay(m_dataSvc, m_dataSvc->rootName() + OverlayEventModel::Overlay::DiagDataOverlay);
    if(!diagDataOverlay) return sc;

    // Successful recovery of an overlay TDS object! 
    // Build the overlay maps from the input overlay diagnostic data
    // First up is the tracker info
    OverDiagMap tkrOverDiagMap;

    for(int idx = 0; idx < diagDataOverlay->getNumTkrDiagnostic(); idx++)
    {
        const Event::TkrDiagDataOverlay& tkrDiagData = diagDataOverlay->getTkrDiagnosticByIndex(idx);

        DiagKeyPair key(tkrDiagData.tower(),tkrDiagData.gtcc());

        tkrOverDiagMap.insert(std::pair<DiagKeyPair,unsigned>(key,tkrDiagData.dataWord()));
    }

    // Now do the same for the Cal
    OverDiagMap calOverDiagMap;

    for(int idx = 0; idx < diagDataOverlay->getNumCalDiagnostic(); idx++)
    {
        const Event::CalDiagDataOverlay& calDiagData = diagDataOverlay->getCalDiagnosticByIndex(idx);

        DiagKeyPair key(calDiagData.tower(),calDiagData.layer());

        calOverDiagMap.insert(std::pair<DiagKeyPair,unsigned>(key,calDiagData.dataWord()));
    }

    // Now recover the Ldf DiagnosticData from the TDS. This will be the simulated data 
    // to which we will overlay information from the input overlay event
    SmartDataPtr<LdfEvent::DiagnosticData> diagnosticData(eventSvc(), "/Event/Diagnostic");
    if (!diagnosticData)
    {
        // If no diagnostic data in the TDS then we create one
        diagnosticData = new LdfEvent::DiagnosticData();

        sc = eventSvc()->registerObject("/Event/Diagnostic", diagnosticData);

        if(sc.isFailure() ) 
        {
            log << MSG::ERROR << "could not register LdfEvent::DiagnosticData in the TDS" << endreq;
            return sc;
        }
    }

    // The first step in actually overlaying is to cross reference any common information. The idea
    // is to build a key from the Ldf DiagnosticData object which is used to reference the information
    // from the input overlay event. We then OR the information and overwrite the DiagnosticData info
    // in the TDS
    // Start with the tracker
    for(int idx = 0; idx < diagnosticData->getNumTkrDiagnostic(); idx++)
    {
        // Retrieve the information from the TDS DiagnosticData object
        const LdfEvent::TkrDiagnosticData& tkrDiagData = diagnosticData->getTkrDiagnosticByIndex(idx);

        // Make a key for referencing into the overlay map
        DiagKeyPair key(tkrDiagData.tower(),tkrDiagData.gtcc());

        // Search for a match
        OverDiagMap::iterator tkrOverIter = tkrOverDiagMap.find(key);

        // Do we have a match?
        if (tkrOverIter != tkrOverDiagMap.end())
        {
            unsigned updateBits = tkrDiagData.dataWord() | tkrOverIter->second;

            // Update the bits for the tracker
            diagnosticData->setTkrDataWordByIndex(idx,updateBits);

            // Now clear the overlay information from the map
            tkrOverDiagMap.erase(tkrOverIter);
        }
    }

    // And then the Cal
    for(int idx = 0; idx < diagnosticData->getNumCalDiagnostic(); idx++)
    {
        const LdfEvent::CalDiagnosticData& calDiagData = diagnosticData->getCalDiagnosticByIndex(idx);

        DiagKeyPair key(calDiagData.tower(),calDiagData.layer());

        // Search for a match
        OverDiagMap::iterator calOverIter = calOverDiagMap.find(key);

        // Do we have a match?
        if (calOverIter != calOverDiagMap.end())
        {
            unsigned updateBits = calDiagData.dataWord() | calOverIter->second;

            // Update the bits for the Cal
            diagnosticData->setCalDataWordByIndex(idx,updateBits);

            // Now clear the overlay information from the map
            calOverDiagMap.erase(calOverIter);
        }
    }

    // We're not done yet! 
    // If there is anything left in the input overlay maps then we need to add these to the 
    // LdfEvent DiagnosticData object. 
    // Start with the tracker
    for(OverDiagMap::iterator tkrIter = tkrOverDiagMap.begin(); tkrIter != tkrOverDiagMap.end(); tkrIter++)
    {
        DiagKeyPair key   = tkrIter->first;
        unsigned    datum = tkrIter->second;

        diagnosticData->addTkrDiagnostic(LdfEvent::TkrDiagnosticData(datum,key.first,key.second));
    }

    // Now do the CAL
    for(OverDiagMap::iterator calIter = calOverDiagMap.begin(); calIter != calOverDiagMap.end(); calIter++)
    {
        DiagKeyPair key   = calIter->first;
        unsigned    datum = calIter->second;

        diagnosticData->addCalDiagnostic(LdfEvent::CalDiagnosticData(datum,key.first,key.second));
    }

    return sc;
}

StatusCode DiagDataOverlayMergeAlg::finalize() 
{
    MsgStream log(msgSvc(), name());
    log << MSG::INFO << "finalize" << endreq;
    return StatusCode::SUCCESS;
}
