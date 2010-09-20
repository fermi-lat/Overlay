/*
 * @file GemOverlayMergeAlg.cxx
 *
 * @brief Calls an user-chosen tool to merge GemOverlay from input overlay files
 *
 * @author Tracy Usher
 *
 * $Header: /nfs/slac/g/glast/ground/cvs/Overlay/src/MergeAlgs/GemOverlayMergeAlg.cxx,v 1.2 2009/02/12 16:46:22 usher Exp $
 */


#include "GaudiKernel/Algorithm.h"
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/AlgFactory.h"
#include "GaudiKernel/SmartDataPtr.h"

#include "Event/TopLevel/EventModel.h"
#include "Event/Trigger/TriggerInfo.h"
#include "OverlayEvent/OverlayEventModel.h"
#include "OverlayEvent/GemOverlay.h"

#include "enums/TriggerBits.h"

#include <map>

class GemOverlayMergeAlg : public Algorithm 
{
 public:

    GemOverlayMergeAlg(const std::string&, ISvcLocator*);
    StatusCode initialize();
    StatusCode execute();
    StatusCode finalize();

 private:
    /// Type of tool to run
    std::string m_type;
};

// Used by Gaudi for identifying this algorithm
//static const AlgFactory<GemOverlayMergeAlg>    Factory;
//const IAlgFactory& GemOverlayMergeAlgFactory = Factory;
DECLARE_ALGORITHM_FACTORY(GemOverlayMergeAlg);

GemOverlayMergeAlg::GemOverlayMergeAlg(const std::string& name,
                                         ISvcLocator* pSvcLocator)
    : Algorithm(name, pSvcLocator) 
{
    // variable to select the tool type
    declareProperty("Type", m_type="General");
}


StatusCode GemOverlayMergeAlg::initialize() 
{
    // Purpose and Method: initializes GemOverlayMergeAlg
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


StatusCode GemOverlayMergeAlg::execute() 
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

    // First, recover any overlay digis, to see if we have anything to do
    SmartDataPtr<Event::GemOverlay> gemOverlay(eventSvc(), OverlayEventModel::Overlay::GemOverlay);
    if(!gemOverlay) return sc;

    // Now recover the TriggerInfo for this event
    SmartDataPtr<Event::TriggerInfo> triggerInfo(eventSvc(), "/Event/TriggerInfo");
    if (!triggerInfo)
    {
        log << MSG::INFO << "Cannot find TriggerInfo in TDS, not processing GemOverlay info" << endreq;
        return sc;
    }

    // Recover the trigger bits
    unsigned int triggerBits = triggerInfo->getTriggerBits();

    // Update the track vector information
    if (gemOverlay->getTkrVector())
    {
        unsigned short tkrVector = triggerInfo->getTkrVector() | gemOverlay->getTkrVector();

        // Reset the track vector
        triggerInfo->setTkrVector(tkrVector);

        // Update trigger bits for possible change in tkrVector
        triggerBits |= enums::b_Track;
    }

    // Update the ACD vector information
    if (gemOverlay->getCnoVector())
    {
        unsigned short cnoVector = triggerInfo->getCnoVector() | gemOverlay->getTkrVector();

        // Reset the cno vector
        triggerInfo->setCnoVector(cnoVector);

        // Update the trigger bits
        triggerBits |= enums::b_ACDH;
    }

    // Update the CAL information
    if (gemOverlay->getCalLEvector() || gemOverlay->getCalHEvector())
    {
        // Update the Cal vectors
        unsigned short calLeVector = triggerInfo->getCalLeVector() | gemOverlay->getCalLEvector();
        unsigned short calHeVector = triggerInfo->getCalHeVector() | gemOverlay->getCalHEvector();

        // reset in TriggerInfo
        triggerInfo->setCalLeVector(calLeVector);
        triggerInfo->setCalHeVector(calHeVector);

        // Update the trigger bits
        triggerBits |= (calLeVector ? enums::b_LO_CAL:0) | (calHeVector ? enums::b_HI_CAL:0);
    }

    // Must also do the roi... 
    if (gemOverlay->getRoiVector())
    {
        unsigned short roiVector = triggerInfo->getRoiVector() | gemOverlay->getRoiVector();

        // reset the roi vector
        triggerInfo->setRoiVector(roiVector);

        // Update the trigger bits
        triggerBits |= enums::b_ROI;
    }

    // Update the struct tile list... start by getting a local copy to modify
    Event::TriggerInfo::TileList tileList = triggerInfo->getTileList();

    // Now recover the overlay's hit tile list
    const Event::GemOverlayTileList& overTileList = gemOverlay->getTileList();

    // Update the hit vectors
    tileList["xzm"] |= overTileList.getXzm();
    tileList["xzp"] |= overTileList.getXzp();
    tileList["yzm"] |= overTileList.getYzm();
    tileList["yzp"] |= overTileList.getYzp();
    tileList["xy"]  |= overTileList.getXy();
    tileList["rbn"] |= overTileList.getRbn();
    tileList["na"]  |= overTileList.getNa();

    // Reset the tile list in the TriggerInfo object
    triggerInfo->setTileList(tileList);

    // Reset the trigger bits
    triggerInfo->setTriggerBits(triggerBits);

    // Update the delta event and window open times
    triggerInfo->setDeltaEventTime(gemOverlay->getDeltaEventTime());
    triggerInfo->setDeltaWindowOpenTime(gemOverlay->getDeltaWindowOpenTime());

    return sc;
}

StatusCode GemOverlayMergeAlg::finalize() 
{
    MsgStream log(msgSvc(), name());
    log << MSG::INFO << "finalize" << endreq;
    return StatusCode::SUCCESS;
}
