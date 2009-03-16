/*
 * @file DoMergeAlg.cxx
 *
 * @brief Decides whether or not to read an overlay event
 *
 * @author Tracy Usher
 *
 * $Header: /nfs/slac/g/glast/ground/cvs/Overlay/src/MergeAlgs/DoMergeAlg.cxx,v 1.3 2008/12/18 23:38:52 usher Exp $
 */


#include "GaudiKernel/Algorithm.h"
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/AlgFactory.h"
#include "GaudiKernel/SmartDataPtr.h"

#include "Event/TopLevel/EventModel.h"
#include "Event/MonteCarlo/McPositionHit.h"
#include "Event/MonteCarlo/McIntegratingHit.h"

#include <map>

class DoMergeAlg : public Algorithm 
{
 public:

    DoMergeAlg(const std::string&, ISvcLocator*);
    StatusCode initialize();
    StatusCode execute();
    StatusCode finalize();

 private:

};

// Used by Gaudi for identifying this algorithm
static const AlgFactory<DoMergeAlg>    Factory;
const IAlgFactory& DoMergeAlgFactory = Factory;

DoMergeAlg::DoMergeAlg(const std::string& name, ISvcLocator* pSvcLocator)
    : Algorithm(name, pSvcLocator) 
{
}


StatusCode DoMergeAlg::initialize() 
{
    // Purpose and Method: initializes DoMergeAlg
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

    return sc;
}


StatusCode DoMergeAlg::execute() 
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

    // How many hits in ACD, TKR or CAL?
    int numPosHits = 0;
    int numIntHits = 0;

    // Recover the McPositionHits for this event
    SmartDataPtr<Event::McPositionHitCol> posHitCol(eventSvc(), EventModel::MC::McPositionHitCol);

    if (posHitCol) numPosHits = posHitCol->size();

    // Recover the McIntegratingHits for this event
    SmartDataPtr<Event::McIntegratingHitCol> intHitCol(eventSvc(), EventModel::MC::McIntegratingHitCol);

    if (intHitCol) numIntHits = intHitCol->size();

    // if there are no McPositionHits AND no McIntegratingHits then the simulated particle did not interact
    if (numPosHits == 0 && numIntHits == 0)
    {
        setFilterPassed(false); 
    }

    return sc;
}

StatusCode DoMergeAlg::finalize() 
{
    MsgStream log(msgSvc(), name());
    log << MSG::INFO << "finalize" << endreq;
    return StatusCode::SUCCESS;
}
