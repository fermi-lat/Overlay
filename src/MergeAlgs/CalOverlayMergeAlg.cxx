/**
 * @file CalOverlayMergeAlg.cxx
 * @brief implementation  of the algorithm CalOverlayMergeAlg.
 *
 * @author Zach Fewtrell zachary.fewtrell@nrl.navy.mil
 * 
 *  $Header: /nfs/slac/g/glast/ground/cvs/Overlay/src/MergeAlgs/CalOverlayMergeAlg.cxx,v 1.6 2011/01/31 23:07:32 usher Exp $
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
#include "CLHEP/Geometry/Transform3D.h"
#include "CLHEP/Geometry/Point3D.h"
#include "CLHEP/Geometry/Vector3D.h"

#include <map>

typedef HepGeom::Point3D<double> HepPoint3D;
typedef HepGeom::Vector3D<double> HepVector3D;

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
    IGlastDetSvc*            m_detSvc;

    /// Pointer to the propagator
    IPropagator*             m_propagator;

    /// store first range option ("autoRng" ---> best range first, "lex8", "lex1", "hex8", "hex1" ---> lex8-hex1 first)
    StringProperty           m_firstRng;

    /// Local useful stuff
    std::vector<std::string> m_diodeNames;
    std::vector<double>      m_diodeHeights;
    double                   m_diodeWidth;
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

    // Get a copy of the propagator
    sc = toolSvc()->retrieveTool("G4PropagationTool", m_propagator);
    if (sc.isSuccess()) 
    {
        log << MSG::INFO << "Retrieved G4PropagationTool" << endreq;
    } else {
        log << MSG::ERROR << "Couldn't retrieve G4PropagationTool" << endreq;
    }

    // Initialize the diode names
    m_diodeNames.push_back("diodeSNEG");
    m_diodeNames.push_back("diodeSPOS");
    m_diodeNames.push_back("diodeLNEG");
    m_diodeNames.push_back("diodeLPOS");

    // Get the diode heights
    double diodeLY = 0.;
    double diodeSY = 0.;
    double diodeX  = 0.;
    double diodeZ  = 0.;

    sc = m_detSvc->getNumericConstByName("diodeLY", &diodeLY);
    sc = m_detSvc->getNumericConstByName("diodeSY", &diodeSY);
    sc = m_detSvc->getNumericConstByName("diodeX",  &diodeX);
    sc = m_detSvc->getNumericConstByName("diodeZ",  &diodeZ);

    m_diodeHeights.push_back(diodeSY);
    m_diodeHeights.push_back(diodeSY);
    m_diodeHeights.push_back(diodeLY);
    m_diodeHeights.push_back(diodeLY);
    
    m_diodeWidth = diodeZ;

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

    // Does the collection exist? 
    if (!calMcHitCol)
    {
        // create the TDS location for the McParticle Collection
        calMcHitCol = new Event::McIntegratingHitVector;
        StatusCode sc = eventSvc()->registerObject(EventModel::MC::McIntegratingHitCol, calMcHitCol);
        if (sc.isFailure()) 
        {
            log << "Overlay failed to register McIntegratingHit in the TDS" << endreq;
            return sc;
        }
    }

    // Create a map of the simulation McIntegratingHits, indexing by identifier
    std::map<idents::VolumeIdentifier, Event::McIntegratingHit*> idToMcHitMap;

    for(Event::McIntegratingHitCol::iterator calIter = calMcHitCol->begin(); calIter != calMcHitCol->end(); calIter++)
    {
        Event::McIntegratingHit*calMcHit = *calIter;

        // Add this McIntegratingHit to our map
        idToMcHitMap[calMcHit->volumeID()] = calMcHit;
    }

    // All "particles" we add will be of type "overlay"
    Event::McIntegratingHit::Particle particle = Event::McIntegratingHit::overlay;

    // Propagator needs a direction, choose "up"
    Vector direction(0.,0.,-1.);

    // Loop through the input CalOverlays and using the above map merge with existing McIntegratingHits
    for(Event::CalOverlayCol::iterator overIter  = overlayCol->begin(); overIter != overlayCol->end(); overIter++)
    {
        Event::CalOverlay* calOverlay = *overIter;

        // Use the propagator to get a valid VolumeIdentifier at the resolution of the McIntegratingHits
        m_propagator->setStepStart(calOverlay->getPosition(), direction);
        idents::VolumeIdentifier overId  = m_propagator->getVolumeId(0);

        // Is this a crystal or a diode?
        int volType = (int)overId[CalUtil::fCellCmp];

        if (volType != 0)
        {
            // Attempt to "tweak" the volume identifer to nudge over to a crystal
            idents::VolumeIdentifier newIdent;

            // Loop through and copy the current fields we want
            for (int identIdx = 0; identIdx < CalUtil::fCellCmp; identIdx++) 
            {
                newIdent.append(overId[identIdx]);
            }

            // Now append the fields we need for a crystal
            newIdent.append(0);                          // for xtal component
            if ((volType ==2) || (volType ==4))          // + end
            {  
                newIdent.append(11);                     // max segment number
            } 
            else newIdent.append(0);                     // - end

            // Now change the old id to the new one
            overId.init(newIdent.getValue(), newIdent.size());
        }

        // Get a CalXtalId from this
        idents::CalXtalId calXtalId(overId);

        // Work out the transform for this volume
        StatusCode sc;
        HepGeom::Transform3D xtalTransform;
        if((sc = m_detSvc->getTransform3DByID(overId,&xtalTransform)).isFailure())
        {
            log << MSG::INFO << "Couldn't retrieve the transform for this xtal segment id" << endreq;
            return StatusCode::SUCCESS;
        }

        HepPoint3D globalHit = calOverlay->getPosition();
        HepPoint3D localHit  = xtalTransform.inverse() * globalHit;

        // Check that this is a valid CalXtalId
        idents::CalXtalId checkId(overId);

        // Does the identifier for this CalOverlay match an McIntegratingHit in the sim map?
        std::map<idents::VolumeIdentifier,Event::McIntegratingHit*>::iterator calIter = idToMcHitMap.find(overId);

        // Pointer to the mcHit
        Event::McIntegratingHit* mcHit = 0;

        // If no match then we need to create a new McIntegratingHit and add to the collection
        if (calIter == idToMcHitMap.end())
        {
            mcHit = new Event::McIntegratingHit();

            mcHit->setVolumeID(overId);
            mcHit->setPackedFlags(Event::McIntegratingHit::overlayHit);
            mcHit->addEnergyItem(calOverlay->getEnergy(), particle, localHit);

            calMcHitCol->push_back(mcHit);
        }
        // Otherwise, a match so we just merge it into the existing McIntegratingHit
        else
        {
            mcHit = calIter->second;

            mcHit->addPackedMask(Event::McIntegratingHit::overlayHit);
            mcHit->addEnergyItem(calOverlay->getEnergy(), particle, localHit);
        }

        // *************************************************************************
        // The section below added to include the light seen by the diodes. It is 
        // meant to emulate the same code in G4Generator's IntDetectorManager class
        // *************************************************************************

        // The first task we work at is to get a transformation from global to local
        // coordinates for the volume containing the individual crystal segments
        // Create a volume identifier for the containing low segment of the crystal
        idents::VolumeIdentifier xtalIdLow;

        // Create the log part of the identifier
        for(int idx = 0; idx <= CalUtil::fCellCmp; idx++)
        {
            xtalIdLow.append(overId[idx]);
        }

        // This will be the high end
        idents::VolumeIdentifier xtalIdHi = xtalIdLow;

        xtalIdLow.append(0);
        xtalIdHi.append(11);

        // Use these to determine the offset of the center of the crystal
        HepGeom::Transform3D xtalLowTransform;
        if((sc = m_detSvc->getTransform3DByID(xtalIdLow,&xtalLowTransform)).isFailure())
        {
            log << MSG::INFO << "Couldn't retrieve the transform for this xtal id." << endreq;
            return StatusCode::SUCCESS;
        }

        HepGeom::Transform3D xtalHiTransform;
        if((sc = m_detSvc->getTransform3DByID(xtalIdHi,&xtalHiTransform)).isFailure())
        {
            log << MSG::INFO << "Couldn't retrieve the transform for this xtal id." << endreq;
            return StatusCode::SUCCESS;
        }

        // We need to make a new translation vector, use the coordinates returned above
        // It is done this way to automatically insure that we get the right value for the 
        // translation without having to worry if this is an x or y measuring crystal
        // If both are same, average returns same value, if both are different we get the 
        // average between the two
        CLHEP::Hep3Vector xtalTrans(0.5*(xtalHiTransform.dx()+xtalLowTransform.dx()), 
                             0.5*(xtalHiTransform.dy()+xtalLowTransform.dy()),
                             xtalHiTransform.dz());

        // The transformation to the center of the enclosing crystal
        xtalTransform = HepGeom::Transform3D(xtalHiTransform.getRotation(), xtalTrans);

        // And reset the local position to this encosing volume
        localHit = xtalTransform.inverse() * globalHit;

        // We need to determine the fraction of light seen by each of the diodes
        // We do this using the same approach done in the simulation 
        // (see IntDetectorManager in G4Generator)
        // Start by looping over the diodes (order is small Neg, small Pos, large Neg, large Pos)
        for(int diodeIdx = 1; diodeIdx < 5; diodeIdx++)
        {
            // Create a volume identifier for this diode. 
            // Apparently we cannot modify the fields so we must create a new one
            // each time through this loop
            idents::VolumeIdentifier diodeId;

            // Create the log part of the identifier
            for(int idx = 0; idx < CalUtil::fCellCmp; idx++)
            {
                diodeId.append(overId[idx]);
            }

            // now make it a diode part
            diodeId.append(diodeIdx);

            // Look up the transform, this will give the position of the center
            // of the diode and its rotation
            HepGeom::Transform3D trnsDiode;
            if((sc = m_detSvc->getTransform3DByID(diodeId,&trnsDiode)).isFailure())
            {
                log << MSG::INFO << "Could not retrieve diode transform" << endreq;
                continue;
            }

            // We need to get the normal vector pointing from the diode into the xtal
            // We can get that by checking if the crystal measures x or y, and
            // checking to see if the diode is on the plus or negative side
            // Assume a normal in plus y
            CLHEP::Hep3Vector diodeNrml(0., 1., 0.);

            // Get both the distance from the center of the current xtal segment to the diode
            // And reset the diodeNrml if we are oriented along x
            double longDistToDiode = xtalTransform.dy() - trnsDiode.dy();

            if (calXtalId.isX()) 
            {
                longDistToDiode = xtalTransform.dx() - trnsDiode.dx();
                diodeNrml       = CLHEP::Hep3Vector(1., 0., 0.);
            }

            if (longDistToDiode < 0.) diodeNrml *= -1.;

            // Get the dimensions of the face of the diode we are dealing with
            double diodeFaceAlpha = 2. * m_diodeHeights[diodeIdx-1];
            double diodeFaceBeta  = 2. * m_diodeWidth;

            // Get the vector from the center of the diode to the current energy deposition point
            CLHEP::Hep3Vector lineToCenter = globalHit - trnsDiode.getTranslation();
            double     distToCenter = lineToCenter.mag();

            // Now want the angle between this line and the diode normal
            double cosTheta = lineToCenter.unit().dot(diodeNrml);

            if (cosTheta < 0.)
            {
                cosTheta *= -1.;
            }

            // Projected area of rectangle is diodeFaceArea * cosTheta, which can also be 
            // written as diodeFaceAlpha * diodeFaceBeta * cosTheta, so we scale diodeFaceBeta
            // by cosTheta for what follows
            double diodeFaceBetaPr = diodeFaceBeta * cosTheta;

            // Determine fractional solid angle subtended. For this problem, this is the solid
            // angle subtended by a pyramid.
            double radical   = (4.*distToCenter*distToCenter + diodeFaceAlpha*diodeFaceAlpha)
                             * (4.*distToCenter*distToCenter + diodeFaceBetaPr*diodeFaceBetaPr);
            double aSinArg   = diodeFaceAlpha * diodeFaceBetaPr / sqrt(radical);
            double fracAngle = asin(aSinArg) / M_PI;
                    
            CLHEP::Hep3Vector stepPos    = localHit;
            double     directFrac = 0.;
            double     totalDep   = 0.;

            // Does angle to surface normal put us in the range of surface reflection? 
            if (distToCenter < 30.) 
            {
				// The following is based on patterns extracted from flight data
			    double dist2Side = std::max(0., 13.5 -fabs(stepPos.y()));
				double dist2End  = std::max(0., 165.-fabs(stepPos.x()));

				// No angle factor for direct light - probably due to end roughening...
					directFrac += fracAngle;

				// Total light attenuation governed by location - strong dependence side to side, 
				// as well as dependencce with distance from end
				if(dist2End < 30) 
                {
					double attenFactor = ((30. - dist2End) * (std::max(0., 13.5-dist2Side)))/ (30.*13.5);
					totalDep += 1. - .33*attenFactor * attenFactor;
				}
				else 
                {
					totalDep   += 1.;
					directFrac += fracAngle;
				}
            }
            // Otherwise, all light "seen" by diode and all energy deposited in crystal
            else
            {
                directFrac += fracAngle;
                totalDep   += 1.;
            }

            // Now get the direct and total energy fractions
            double directDepE = calOverlay->getEnergy() * directFrac;
            double totalDepE  = calOverlay->getEnergy() * totalDep;

            // Retrieve reference to object to fill for this diode
            Event::McIntegratingHit::XtalEnergyDep& xtalDep = mcHit->getXtalEnergyDep(m_diodeNames[diodeIdx-1]);

            xtalDep.addEnergyItem(totalDepE, directDepE, localHit);

            int stop = 0;
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
