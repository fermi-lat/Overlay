/**
 * @file CalOverlayMergeAlg.cxx
 * @brief implementation  of the algorithm CalOverlayMergeAlg.
 *
 * @author Zach Fewtrell zachary.fewtrell@nrl.navy.mil
 * 
 *  $Header: /nfs/slac/g/glast/ground/cvs/CalDigi/src/CalOverlayMergeAlg.cxx,v 1.1 2008/10/15 15:43:19 usher Exp $
 */

// Gaudi specific include files
#include "GaudiKernel/Algorithm.h"
#include "GaudiKernel/AlgFactory.h"
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/IDataProviderSvc.h"
#include "GaudiKernel/SmartDataPtr.h"

// Glast specific includes
#include "Event/TopLevel/EventModel.h"
#include "Event/TopLevel/Event.h"
#include "Event/MonteCarlo/McIntegratingHit.h"
#include "OverlayEvent/OverlayEventModel.h"
#include "OverlayEvent/EventOverlay.h"
#include "OverlayEvent/CalOverlay.h"
#include "CalUtil/CalDefs.h"
#include "GlastSvc/GlastDetSvc/IGlastDetSvc.h"
#include "GlastSvc/Reco/IPropagator.h"
#include "CalUtil/CalGeom.h"  
#include "idents/TowerId.h"

#include "CalXtalResponse/ICalCalibSvc.h"

#include <map>

// Class definition
class CalOverlayMergeAlg : public Algorithm {

public:

    CalOverlayMergeAlg(const std::string& name, ISvcLocator* pSvcLocator); 

    StatusCode initialize();
    StatusCode execute();
    StatusCode finalize() {return StatusCode::SUCCESS;}
 
private:

    /// For making a VolumeIdentifier
    idents::VolumeIdentifier makeVolId(idents::CalXtalId calXtalId);

    /// used for constants & conversion routines.
    IGlastDetSvc*  m_detSvc;
  
    /// pointer to CalCalibSvc objects.
    ICalCalibSvc*  m_overCalibSvc;  
    ICalCalibSvc*  m_digiCalibSvc;  

    /// Pointer to the propagator
    IPropagator*   m_propagator;

    /// store first range option ("autoRng" ---> best range first, "lex8", "lex1", "hex8", "hex1" ---> lex8-hex1 first)
    StringProperty m_firstRng;
};


// Define the factory for this algorithm
static const AlgFactory<CalOverlayMergeAlg>  Factory;
const IAlgFactory& CalOverlayMergeAlgFactory = Factory;

/// construct object & declare jobOptions
CalOverlayMergeAlg::CalOverlayMergeAlg(const string& name, ISvcLocator* pSvcLocator) :
  Algorithm(name, pSvcLocator)
{

    // Declare the properties that may be set in the job options file
    declareProperty("FirstRangeReadout",   m_firstRng= "autoRng"); 
}

/// initialize the algorithm. retrieve helper tools & services
StatusCode CalOverlayMergeAlg::initialize() 
{
    StatusCode sc = StatusCode::SUCCESS;
  
    MsgStream log(msgSvc(), name());
    log << MSG::INFO << "initialize" << endreq;

    sc = setProperties();
    if (sc.isFailure()) {
        log << MSG::ERROR << "Could not set jobOptions properties" << endreq;
        return sc;
    }
    
    // try to find the GlastDetSvc service
    sc = service("GlastDetSvc", m_detSvc);
    if (sc.isFailure() ) {
        log << MSG::ERROR << "  can't get GlastDetSvc " << endreq;
        return sc;
    }
  
    // obtain the CalCalibSvc for the input overlay digis (data!)
    sc = service("CalOverlayCalibSvc", m_overCalibSvc);
    if (sc.isFailure()) 
    {
        log << MSG::ERROR << "can't get CalOverlayCalibSvc "  << endreq;
        return sc;
    }
  
    // now obtain the CalCalibSvc for the simulation digis
    sc = service("CalCalibSvc", m_digiCalibSvc);
    if (sc.isFailure()) 
    {
        log << MSG::ERROR << "can't get CalCalibSvc "  << endreq;
        return sc;
    }

    // Get a copy of the propagator
    sc = toolSvc()->retrieveTool("G4PropagationTool", m_propagator);
    if (sc.isSuccess()) 
    {
        log << MSG::INFO << "Retrieved G4PropagationTool" << endreq;
    } else {
        log << MSG::ERROR << "Couldn't retrieve G4PropagationTool" << endreq;
    }

    return sc;
}

/// \brief take Hits from McIntegratingHits, create & register CalDigis
StatusCode CalOverlayMergeAlg::execute() 
{
    // Get a message object for output
    MsgStream log(msgSvc(), name());

    // First, recover any overlay digis, to see if we have anything to do
    SmartDataPtr<Event::CalOverlayCol> overlayCol(eventSvc(), OverlayEventModel::Overlay::CalOverlayCol);
    if(!overlayCol) return StatusCode::SUCCESS;

    // Now recover the McIntegratingHits for this event
    SmartDataPtr<Event::McIntegratingHitCol> calMcHitCol(eventSvc(), EventModel::MC::McIntegratingHitCol);

    // Create a map of the simulation McIntegratingHits, indexing by identifier
    std::map<idents::VolumeIdentifier, Event::McIntegratingHit*> idToMcHitMap;

    for(Event::McIntegratingHitCol::iterator calIter = calMcHitCol->begin(); calIter != calMcHitCol->end(); calIter++)
    {
        Event::McIntegratingHit*calMcHit = *calIter;
//        const idents::CalXtalId calXtalId(calMcHit->volumeID());

//        const idents::VolumeIdentifier tempId = calMcHit->volumeID();

        // Add this McIntegratingHit to our map
//        idToMcHitMap[calXtalId] = calMcHit;
        idToMcHitMap[calMcHit->volumeID()] = calMcHit;
    }

    // All "particles" we add will be of type "overlay"
    Event::McIntegratingHit::Particle particle = Event::McIntegratingHit::overlay;

    // Propagator needs a direction, choose "up"
    Vector direction(0.,0.,-1.);

    // Loop through the input CalOverlays and using the above map merge with existing McIntegratingHits
    for(Event::CalOverlayCol::iterator overIter  = overlayCol->begin(); overIter != overlayCol->end(); overIter++)
    {
        Event::CalOverlay*      calOverlay = *overIter;

        // Use the propagator to get a valid VolumeIdentifier at the resolution of the McIntegratingHits
        m_propagator->setStepStart(calOverlay->getPosition(), direction);
        idents::VolumeIdentifier overId  = m_propagator->getVolumeId(0);

        // Check that this is a valid CalXtalId
        idents::CalXtalId checkId(overId);

        // Does the identifier for this CalOverlay match an McIntegratingHit in the sim map?
        std::map<idents::VolumeIdentifier,Event::McIntegratingHit*>::iterator calIter = idToMcHitMap.find(overId);

        // If no match then we need to create a new McIntegratingHit and add to the collection
        if (calIter == idToMcHitMap.end())
        {
            Event::McIntegratingHit* newMcHit = new Event::McIntegratingHit();

//            idents::VolumeIdentifier id = makeVolId(overId);

            newMcHit->setVolumeID(overId);
            newMcHit->addEnergyItem(calOverlay->getEnergy(), particle, calOverlay->getPosition());

            calMcHitCol->push_back(newMcHit);
        }
        // Otherwise, a match so we just merge it into the existing McIntegratingHit
        else
        {
            Event::McIntegratingHit* mcHit = calIter->second;

            mcHit->addEnergyItem(calOverlay->getEnergy(), particle, calOverlay->getPosition());
        }
    }

    return StatusCode::SUCCESS;
}

idents::VolumeIdentifier CalOverlayMergeAlg::makeVolId(idents::CalXtalId calXtalId)
{
    // Snippet of code fror Leon Rochester for converting a CalXtalId into
    // a VolumeIdentifier...
    short tower, layer, column;

    calXtalId.getUnpackedId(tower, layer, column);

    idents::VolumeIdentifier volId;

    idents::TowerId towerId(tower);

    volId.append(0);
    volId.append(towerId.iy());
    volId.append(towerId.ix());
    volId.append(0);
    volId.append(layer);
    int measured = (calXtalId.isX() ? 0 : 1);
    volId.append(measured);
    volId.append(column);
    volId.append(0);
    volId.append(0);

    return volId;
}
