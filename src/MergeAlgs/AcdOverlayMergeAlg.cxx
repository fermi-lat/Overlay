/**
 * @file AcdOverlayMergeAlg.cxx
 * @brief implementation  of the algorithm AcdOverlayMergeAlg.
 *
 * @author Zach Fewtrell zachary.fewtrell@nrl.navy.mil
 * 
 *  $Header: /nfs/slac/g/glast/ground/cvs/GlastRelease-scons/Overlay/src/MergeAlgs/AcdOverlayMergeAlg.cxx,v 1.9 2011/06/27 17:45:57 usher Exp $
 */

// Gaudi specific include files
#include "GaudiKernel/Algorithm.h"
#include "GaudiKernel/AlgFactory.h"
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/IDataProviderSvc.h"
#include "GaudiKernel/SmartDataPtr.h"
#include "GaudiKernel/DataSvc.h"

// Glast specific includes
#include "Event/TopLevel/EventModel.h"
#include "Event/TopLevel/Event.h"
#include "Event/MonteCarlo/McPositionHit.h"
#include "OverlayEvent/OverlayEventModel.h"
#include "OverlayEvent/EventOverlay.h"
#include "OverlayEvent/AcdOverlay.h"

#include "GlastSvc/GlastDetSvc/IGlastDetSvc.h"

#include "AcdUtil/IAcdCalibSvc.h"
#include "AcdUtil/AcdRibbonDim.h"
#include "AcdUtil/AcdTileDim.h"
#include "AcdUtil/AcdGeomMap.h"
#include "AcdUtil/IAcdGeometrySvc.h"

#include <map>

typedef HepGeom::Point3D<double> HepPoint3D;
typedef HepGeom::Vector3D<double> HepVector3D;

// Class definition
class AcdOverlayMergeAlg : public Algorithm {

public:


    AcdOverlayMergeAlg(const std::string& name, ISvcLocator* pSvcLocator); 

    StatusCode initialize();
    StatusCode execute();
    StatusCode finalize() {return StatusCode::SUCCESS;}
 
private:

    /// used for constants & conversion routines.
    IGlastDetSvc*    m_detSvc;

    /// Pointer to the Acd geometry svc
    IAcdGeometrySvc* m_acdGeoSvc;
    
    /// Energy threshold ("zero-suppression") for ACD hits
    double           m_energyThreshold;

    /// Pointer to the overlay's data provider service
    DataSvc*         m_dataSvc;
};


// Define the factory for this algorithm
//static const AlgFactory<AcdOverlayMergeAlg>  Factory;
//const IAlgFactory& AcdOverlayMergeAlgFactory = Factory;
DECLARE_ALGORITHM_FACTORY(AcdOverlayMergeAlg);

/// construct object & declare jobOptions
AcdOverlayMergeAlg::AcdOverlayMergeAlg(const std::string& name, ISvcLocator* pSvcLocator) :
  Algorithm(name, pSvcLocator)
{

    // Declare the properties that may be set in the job options file
//    declareProperty("FirstRangeReadout",   m_firstRng= "autoRng");
    // default setting means that the energy cut is not applied
    declareProperty("AcdHitEnergyThreshold", m_energyThreshold = -1.0);
}

/// initialize the algorithm. retrieve helper tools & services
StatusCode AcdOverlayMergeAlg::initialize() 
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

    IService* iService = 0;
    sc = serviceLocator()->service("AcdGeometrySvc", iService, true);
    if (sc.isSuccess() ) 
    {
        sc = iService->queryInterface(IID_IAcdGeometrySvc, (void**)&m_acdGeoSvc);
    }

    if (sc.isFailure()) 
    {
        log << MSG::ERROR << "Failed to get the AcdGeometerySvc" << endreq;
        return sc;
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

/// \brief take Hits from McIntegratingHits, create & register AcdDigis
StatusCode AcdOverlayMergeAlg::execute() 
{
    // Get a message object for output
    MsgStream msglog(msgSvc(), name());

    // First, recover any overlay digis, to see if we have anything to do
    SmartDataPtr<Event::AcdOverlayCol> overlayCol(m_dataSvc, m_dataSvc->rootName() + OverlayEventModel::Overlay::AcdOverlayCol);
    if (!overlayCol) return StatusCode::SUCCESS;

    // The calibration will need the Event Header information for both the sim and the overlay
    SmartDataPtr<Event::McPositionHitCol> mcPosHitCol(eventSvc(), EventModel::MC::McPositionHitCol);
    if (!mcPosHitCol) return StatusCode::SUCCESS;

    // The calibration will need the Event Header information for both the sim and the overlay
    SmartDataPtr<Event::EventHeader> digiHeader(eventSvc(), "/Event");
  
    if (!digiHeader) {
      msglog << MSG::ERROR << "Unable to retrieve event timestamp for digis" << endreq;
      return StatusCode::FAILURE;
    }

    // The calibration will need the Event Header information for both the sim and the overlay
    SmartDataPtr<Event::EventOverlay> overHeader(m_dataSvc, m_dataSvc->rootName() + OverlayEventModel::Overlay::EventOverlay);
  
    if (!overHeader) {
      msglog << MSG::ERROR << "Unable to retrieve event timestamp for overlay" << endreq;
      return StatusCode::FAILURE;
    }

    // Loop through the input digis and using the above map merge with existing MC digis
    for(Event::AcdOverlayCol::iterator overIter  = overlayCol->begin(); overIter != overlayCol->end(); overIter++)
    {
        Event::AcdOverlay* acdOverlay = *overIter;

        // Retrieve the volume identifiers
        const idents::VolumeIdentifier& volumeId = acdOverlay->getVolumeId();
        const idents::AcdId&            acdId    = acdOverlay->getAcdId();

        // Energy deposited
        double energyDep = acdOverlay->getEnergyDep();

        //if threshold is above zero, cut on that value...
        if (m_energyThreshold>0.0 && energyDep < m_energyThreshold) continue;

        // Position
        HepPoint3D position = acdOverlay->getPosition();

        HepPoint3D center;

        // If we have a struck tile go down this path
        if (acdId.tile())
        {
            // Look up the geometry for the tile
            const AcdTileDim* tileDim = m_acdGeoSvc->geomMap().getTile(acdId,*m_acdGeoSvc);

            center = tileDim->tileCenter(0);

            // Work out the entry point
            HepPoint3D entryPoint = tileDim->corner(0)[0];

            for(int idx = 1; idx < 4; idx++) entryPoint += tileDim->corner(0)[idx];

            entryPoint /= 4;

            HepPoint3D locEntryPoint;

            tileDim->toLocal(entryPoint, locEntryPoint, 0);

            HepPoint3D locExitPoint(locEntryPoint.x(), locEntryPoint.y(), locEntryPoint.z() + 0.5);

            HepPoint3D exitPoint = tileDim->transform(0).inverse() * locExitPoint;

            // Irregular shape?
            if (tileDim->nVol() > 1)
            {
                // Work out the exit point
                HepPoint3D exitPoint = tileDim->corner(1)[0];

                for(int idx = 1; idx < 4; idx++) exitPoint += tileDim->corner(1)[idx];

                exitPoint /= 4;

                tileDim->toLocal(exitPoint, locExitPoint, 1);
            }

            // Create a new McPositionHit for this 
            Event::McPositionHit* mcHit = new Event::McPositionHit();

            // Initalize
            mcHit->init(energyDep, 
                        volumeId, 
                        locEntryPoint, 
                        locExitPoint, 
                        entryPoint, 
                        exitPoint, 
                        Event::McPositionHit::overlayHit);

            // Since this is from overlay, add the acdOverlay status mask to the packed flags
            mcHit->addPackedMask(acdOverlay->getStatus());

            mcPosHitCol->push_back(mcHit);
        }

    }

    return StatusCode::SUCCESS;
}
