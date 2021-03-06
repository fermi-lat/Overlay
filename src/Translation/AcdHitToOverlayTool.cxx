/**  @file AcdHitToOverlayTool.cxx
    @brief implementation of class AcdHitToOverlayTool
    
  $Header: /nfs/slac/g/glast/ground/cvs/Overlay/src/Translation/AcdHitToOverlayTool.cxx,v 1.9 2014/02/14 03:38:06 echarles Exp $  
*/

#include "IDigiToOverlayTool.h"

#include "GaudiKernel/ToolFactory.h"
#include "GaudiKernel/AlgTool.h"
#include "GaudiKernel/SmartDataPtr.h"
#include "GaudiKernel/GaudiException.h" 
#include "GaudiKernel/IDataProviderSvc.h"
#include "GaudiKernel/DataSvc.h"

#include "Event/TopLevel/EventModel.h"
#include "Event/Digi/AcdDigi.h"
#include "Event/Recon/AcdRecon/AcdRecon.h"

#include "LdfEvent/Gem.h"

#include "OverlayEvent/OverlayEventModel.h"
#include "OverlayEvent/AcdOverlay.h"

#include "AcdUtil/IAcdCalibSvc.h"
#include "AcdUtil/AcdRibbonDim.h"
#include "AcdUtil/AcdTileDim.h"
#include "AcdUtil/AcdGeomMap.h"
#include "AcdUtil/IAcdGeometrySvc.h"

#include "AcdUtil/AcdCalibFuncs.h"

#include "CalibData/Acd/AcdPed.h"
#include "CalibData/Acd/AcdGain.h"
#include "CalibData/Acd/AcdHighRange.h"
#include "CalibData/Acd/AcdCoherentNoise.h"

/** @class BackgroundSelection
    @brief manage the selection of background events to merge with signal events
    @author Dan Flath

It uses the name of a tuple variable as a key to look up trigger and downlink rates of a background
source depending on the value of that variable. See the corresponding member functions. Given such
a value, it expects to find a source of such events generated with that value which can be inserted 
into the Gleam output tuple.

*/
typedef HepGeom::Point3D<double> HepPoint3D;

class AcdHitToOverlayTool : public AlgTool, virtual public IDigiToOverlayTool
{
public:

    // Standard Gaudi Tool constructor
    AcdHitToOverlayTool(const std::string& type, const std::string& name, const IInterface* parent);

    // After building it, destroy it
    ~AcdHitToOverlayTool();

    /// @brief Intialization of the tool
    StatusCode initialize();

    /// @brief Finalize method for the tool
    StatusCode finalize();

    ///! The current value of the quantity that we are selecting on
    StatusCode translate();

private:
  
    /// Internal methods stolen from AcdUtil's AcdPha2MipTool
    Event::AcdHit* makeAcdHit ( const Event::AcdDigi* digi);
    bool getCalibratedValues(const Event::AcdDigi* digi, double& mipsPmtA, double& mipsPmtB, bool& acceptDigi) const;
    bool getValues_lowRange(const idents::AcdId& id, Event::AcdDigi::PmtId pmt, unsigned short pha, 
			  double& pedSub, double& mips) const;
    bool getValues_highRange(const idents::AcdId& id, Event::AcdDigi::PmtId pmt, unsigned short pha, 
			   double& pedSub, double& mips) const;  
    bool accept(const idents::AcdId& id, float pedSubtracted, float mips) const;

    /// cut tiles with pedestal subtracted PHA below this value
    float m_pha_tile_cut;
    /// cut tiles with MIP equivalent signal below this value
    float m_mips_tile_cut;
    /// cut ribbons with pedestal subtracted PHA below this value
    float m_pha_ribbon_cut;
    /// cut ribbons with MIP equivalent signal below this value
    float m_mips_ribbon_cut;
    /// Value to use for "Ninja" hits, with no signal, but veto bit asserted
    float m_vetoThreshold;

    /// Convert energy to mips
    float m_mipsPerMeV;
    /// Convert energy to mips in ribbons
    float m_mipsPerMeV_Ribbon;

    /// Turn on or off the coherent noise calibration
    bool m_applyCoherentNoiseCalib;

    /// This is needed for the coherent noise calibration
    int m_gemDeltaEventTime;

    /// Pointer to the event data service (aka "eventSvc")
    IDataProviderSvc*      m_edSvc;

    /// Pointer to the Overlay data service
    DataSvc*          m_dataSvc;

    /// Pointer to the Acd geometry svc
    IAcdGeometrySvc*       m_acdGeoSvc;

    /// ACD calibration service
    AcdUtil::IAcdCalibSvc* m_calibSvc;
};

//static ToolFactory<AcdHitToOverlayTool> s_factory;
//const IToolFactory& AcdHitToOverlayToolFactory = s_factory;
DECLARE_TOOL_FACTORY(AcdHitToOverlayTool);

//------------------------------------------------------------------------
AcdHitToOverlayTool::AcdHitToOverlayTool(const std::string& type, 
                                 const std::string& name, 
                                 const IInterface* parent) :
                                 AlgTool(type, name, parent)
{
    //Declare the additional interface
    declareInterface<IDigiToOverlayTool>(this);

    //Declare properties for this tool
    declareProperty("PHATileCut",    m_pha_tile_cut    = 25.0);
    declareProperty("MIPSTileCut",   m_mips_tile_cut   = 0.0);
    declareProperty("PHARibbonCut",  m_pha_ribbon_cut  = 25.0);
    declareProperty("MIPSRibbonCut", m_mips_ribbon_cut = 0.0);
    declareProperty("VetoThrehsold", m_vetoThreshold   = 0.4);

    declareProperty("MipsPerMeV",    m_mipsPerMeV      = 0.52631);
    declareProperty("MipsPerMeV_Ribbon",m_mipsPerMeV_Ribbon = 2.0);

    declareProperty("ApplyCoherentNoiseCalib",m_applyCoherentNoiseCalib = false);
    return;
}
//------------------------------------------------------------------------
AcdHitToOverlayTool::~AcdHitToOverlayTool()
{
    return;
}

StatusCode AcdHitToOverlayTool::initialize()
{
    StatusCode sc   = StatusCode::SUCCESS;
    MsgStream log(msgSvc(), name());

    // Set the properties
    setProperties();

    IService* iService = 0;
    sc = serviceLocator()->service("EventDataSvc", iService, true);
    if ( sc.isFailure() ) {
        log << MSG::ERROR << "could not find EventDataSvc !" << endreq;
        return sc;
    }
    m_edSvc = dynamic_cast<IDataProviderSvc*>(iService);

    sc = serviceLocator()->service("OverlayOutputSvc", iService, true);
    if ( sc.isFailure() ) {
        log << MSG::ERROR << "could not find EventDataSvc !" << endreq;
        return sc;
    }
    m_dataSvc = dynamic_cast<DataSvc*>(iService);

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

    sc = serviceLocator()->service("AcdCalibSvc", m_calibSvc, true);
    if ( !sc.isSuccess() ) 
    {
        log << MSG::ERROR << "Could not get CalibDataSvc: AcdCalibSvc" << endreq;
        return sc;
    } 
    else 
    {
        log << MSG::INFO << "Got CalibDataSvc: AcdCalibSvc" << endreq;
    }

    return sc;
}

StatusCode AcdHitToOverlayTool::finalize ()
{
    StatusCode  status = StatusCode::SUCCESS;
    
    return status;
}

//------------------------------------------------------------------------
StatusCode AcdHitToOverlayTool::translate()
{
    MsgStream log(msgSvc(), name());
    StatusCode status = StatusCode::SUCCESS;

    // Deal with the GEM Delta Event Time, if requested
    if ( m_applyCoherentNoiseCalib ) 
    {
        SmartDataPtr<LdfEvent::Gem> gemTds(m_edSvc, "/Event/Gem");    
	if (gemTds) 
        {
	    m_gemDeltaEventTime = gemTds->deltaEventTime();
	}  else {
	    log << MSG::ERROR << "Failed to get GemDeltaEventTime from Event" << endreq;
	    return StatusCode::FAILURE;	    
	}
    }

    // Now recover the hit collection
    SmartDataPtr<Event::AcdDigiCol> acdDigiCol(m_edSvc, EventModel::Digi::AcdDigiCol);

    // Create a collection of AcdOverlays and register in the TDS
    SmartDataPtr<Event::AcdOverlayCol> overlayCol(m_dataSvc, m_dataSvc->rootName() + OverlayEventModel::Overlay::AcdOverlayCol);
    if(!overlayCol)
    {
        overlayCol = new Event::AcdOverlayCol();

        status = m_dataSvc->registerObject(m_dataSvc->rootName() + OverlayEventModel::Overlay::AcdOverlayCol, overlayCol);
        if( status.isFailure() ) 
        {
            log << MSG::ERROR << "could not register OverlayEventModel::Overlay::AcdOverlayCol" << endreq;
            return status;
        }
    }

    float mipsPerMeV10 = m_mipsPerMeV;
    float mipsPerMeV12 = m_mipsPerMeV/1.2;
    
    float mipPerMeV(0.);

    // Loop over available AcdHits
    for(Event::AcdDigiCol::const_iterator acdIter = acdDigiCol->begin(); acdIter != acdDigiCol->end(); acdIter++)
    {
        // Dereference our AcdHit object
        const Event::AcdDigi* acdDigi = *acdIter;

        // Make an AcdHit to go with this
        Event::AcdHit* acdHit = makeAcdHit(acdDigi);

        if (!acdHit) continue;

        // Extract volume identifiers
        idents::VolumeIdentifier volId = acdDigi->getVolId();
        idents::AcdId            acdId = acdDigi->getId();

        // Extract deposited energy.  The conversion from mips depends on if we have a tile or ribbon
	if ( acdId.tile() ) {
	  mipPerMeV = acdId.top() && acdId.row() == 2 ? mipsPerMeV12 : mipsPerMeV10;
	} else if ( acdId.ribbon() ) {
	  mipPerMeV = m_mipsPerMeV_Ribbon;
	}	

        float  eDep   = acdHit->mips() / mipPerMeV;

        // Our entry/exit point
        HepPoint3D center(0.,0.,0.);

        // If we have a struck tile go down this path
        if (acdId.tile())
        {
            // Look up the geometry for the tile
            const AcdTileDim* tileDim = m_acdGeoSvc->geomMap().getTile(acdId,*m_acdGeoSvc);

            status = tileDim->statusCode();
            center = tileDim->tileCenter(0);
        }
        else if (acdId.ribbon())
        {
            // Look up the geometry for the tile
            const AcdRibbonDim* ribbonDim = m_acdGeoSvc->geomMap().getRibbon(acdId,*m_acdGeoSvc);
            status = ribbonDim->statusCode();

//            const HepPoint3D& ribbonCenter = ribbonDim->ribbonCenter(0);

//            center = ribbonCenter;
        }

        // Create the AcdOverlay object
        Event::AcdOverlay* acdOverlay = new Event::AcdOverlay(volId, acdId, eDep, center);

        // Get the status bits
        unsigned int statusBits = acdHit->getFlags(Event::AcdHit::A) | (acdHit->getFlags(Event::AcdHit::B) << 16);

        acdOverlay->setStatus(statusBits);

        // Dump the AcdHit we made
        delete acdHit;

        // Add the finished product to our collection
        overlayCol->push_back(acdOverlay);
    }

    return status;
}



Event::AcdHit* AcdHitToOverlayTool::makeAcdHit ( const Event::AcdDigi* digi)
{
    // Start with null pointer for return value
    Event::AcdHit* hit = 0;

    double mipsPmtA(0.);
    double mipsPmtB(0.);
    bool   acceptDigi(false);
    
    bool ok = getCalibratedValues(digi,mipsPmtA,mipsPmtB,acceptDigi);

    if ( !ok ) return hit;

    // Check Veto thresholds but remember that we are working with data digis here
    if ( digi->getHitMapBit( Event::AcdDigi::A ) ) 
    {
        mipsPmtA = mipsPmtA > m_vetoThreshold ? mipsPmtA : m_vetoThreshold;
        acceptDigi = true;
    }
    if ( digi->getHitMapBit( Event::AcdDigi::B ) ) 
    {
        mipsPmtB = mipsPmtB > m_vetoThreshold ? mipsPmtB : m_vetoThreshold;
        acceptDigi = true;
    }

    // Check for Ninja Hits
    if ( digi->isNinja() || digi->getGemFlag() ) 
    {
        mipsPmtA = mipsPmtA > m_vetoThreshold ? mipsPmtA : m_vetoThreshold;
        mipsPmtB = mipsPmtB > m_vetoThreshold ? mipsPmtB : m_vetoThreshold;
        acceptDigi = true;
    }

    if ( acceptDigi ) 
    {
        hit = new Event::AcdHit(*digi,mipsPmtA,mipsPmtB);
	// Correct for the fact that the Accept Mask bits are always set.
	hit->correctAcceptMapBits( (mipsPmtA > 1e-6), (mipsPmtB > 1e-6) );
    }

    return hit;
}

bool AcdHitToOverlayTool::getCalibratedValues(const Event::AcdDigi* digi, 
                                              double&               mipsPmtA, 
                                              double&               mipsPmtB, 
                                              bool&                 acceptDigi) const 
{
    // get calibration consts
    acceptDigi = false;
    double pedSubA(0.);
    double pedSubB(0.);

    // do PMT A
    bool hasHitA = digi->getAcceptMapBit(Event::AcdDigi::A) || digi->getVeto(Event::AcdDigi::A);    
    if ( hasHitA ) 
    {
        Event::AcdDigi::Range rangeA = digi->getRange(Event::AcdDigi::A);  
        bool ok = rangeA == Event::AcdDigi::LOW ? 
        getValues_lowRange(digi->getId(),Event::AcdDigi::A,digi->getPulseHeight(Event::AcdDigi::A),pedSubA,mipsPmtA) :
        getValues_highRange(digi->getId(),Event::AcdDigi::A,digi->getPulseHeight(Event::AcdDigi::A),pedSubA,mipsPmtA);
    
        if ( !ok ) return false;    
        acceptDigi |= rangeA == Event::AcdDigi::HIGH ? true : accept(digi->getId(),pedSubA,mipsPmtA);
    }

    // do PMT B
    bool hasHitB = digi->getAcceptMapBit(Event::AcdDigi::B) || digi->getVeto(Event::AcdDigi::B);    
    if ( hasHitB ) 
    {
        Event::AcdDigi::Range rangeB = digi->getRange(Event::AcdDigi::B);  
        bool ok = rangeB == Event::AcdDigi::LOW ? 
        getValues_lowRange(digi->getId(),Event::AcdDigi::B,digi->getPulseHeight(Event::AcdDigi::B),pedSubB,mipsPmtB) :
        getValues_highRange(digi->getId(),Event::AcdDigi::B,digi->getPulseHeight(Event::AcdDigi::B),pedSubB,mipsPmtB);
    
        if ( !ok ) return false;
    
        acceptDigi |= rangeB == Event::AcdDigi::HIGH ? true : accept(digi->getId(),pedSubB,mipsPmtB);
    }

    return true;
}

bool AcdHitToOverlayTool::getValues_lowRange(const idents::AcdId&  id, 
                                             Event::AcdDigi::PmtId pmt, 
                                             unsigned short        pha, 
                                             double&               pedSub, 
                                             double&               mips) const 
{
    if ( m_calibSvc == 0 ) return false;  
  
    CalibData::AcdPed* ped(0);
  
    StatusCode sc = m_calibSvc->getPedestal(id,pmt,ped);
    if ( sc.isFailure() ) 
    {
        return false;
    }
    double pedestal = ped->getMean();

    CalibData::AcdGain* gain(0);
  
    sc = m_calibSvc->getMipPeak(id,pmt,gain);
    if ( sc.isFailure() ) 
    {
        return false;
    }
    double mipPeak = gain->getPeak();

    // Added Feb. 2014 by EAC to deal with coherent noise
    if ( m_applyCoherentNoiseCalib ) 
    {
        CalibData::AcdCoherentNoise* cNoise(0);
	sc = m_calibSvc->getCoherentNoise(id,pmt,cNoise);
	if ( sc.isFailure() ) 
	{
	    return false;
	}
	double deltaPed(0.);
	sc = AcdCalib::coherentNoise(m_gemDeltaEventTime,
				     cNoise->getAmplitude(),cNoise->getDecay(),cNoise->getFrequency(),cNoise->getPhase(),
				     deltaPed);
	if ( sc.isFailure() ) 
        {
	    return false;
	}
	pedestal += deltaPed;
    }

    pedSub = (double)pha - pedestal;
    
    // Added Feb. 2014 by EAC to deal with periodic triggers below the ZS threshold.
    if ( id.tile() ) {
      if ( pedSub < m_pha_tile_cut ) { 
	pedSub = 0.;
	mips = 0.;
	return true;
      }
    } else if ( id.ribbon() ) {
      if ( pedSub < m_pha_ribbon_cut ) {
	pedSub = 0.;
	mips = 0.;
	return true;
      }	
    } else {
      pedSub = 0.;
      mips = 0.;
      return true;
    }
    sc = AcdCalib::mipEquivalent_lowRange(pha,pedestal,mipPeak,mips);
  
    return sc.isFailure() ? false : true;

}

bool AcdHitToOverlayTool::getValues_highRange(const idents::AcdId&  id, 
                                              Event::AcdDigi::PmtId pmt, 
                                              unsigned short        pha, 
                                              double&               pedSub, 
                                              double&               mips) const 
{
    if ( m_calibSvc == 0 ) return false;  
  
    CalibData::AcdHighRange* highRange(0);
  
    StatusCode sc = m_calibSvc->getHighRange(id,pmt,highRange);
    if ( sc.isFailure() ) return false;

    double pedestal = highRange->getPedestal();
    double slope = highRange->getSlope();
    double saturation = highRange->getSaturation();
    pedSub = (double)pha - pedestal;
  
    sc = AcdCalib::mipEquivalent_highRange(pha,pedestal,slope,saturation,mips);
    
    return sc.isFailure() ? false : true;
}

bool AcdHitToOverlayTool::accept(const idents::AcdId& id, float pedSubtracted, float mips) const 
{
    if ( id.tile() ) 
    {
        if ( pedSubtracted < m_pha_tile_cut ) return false;
        if ( mips < m_mips_tile_cut ) return false;    
    } 
    else if ( id.ribbon() ) 
    {
        if ( pedSubtracted < m_pha_ribbon_cut ) return false;
        if ( mips < m_mips_ribbon_cut ) return false;        
    } 
    else 
    {
        return false;
    }
  
    return true;
}
