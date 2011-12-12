/*
 * @file TkrOverlayMergeAlg.cxx
 *
 * @brief Calls an user-chosen tool to merge TkrDigis from input overlay files
 *
 * @author Tracy Usher
 *
 * $Header: /nfs/slac/g/glast/ground/cvs/GlastRelease-scons/Overlay/src/MergeAlgs/TkrOverlayMergeAlg.cxx,v 1.4 2011/06/27 17:45:57 usher Exp $
 */


#include "GaudiKernel/Algorithm.h"
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/AlgFactory.h"
#include "GaudiKernel/SmartDataPtr.h"
#include "GaudiKernel/DataSvc.h"

#include "Event/TopLevel/EventModel.h"
#include "Event/Digi/TkrDigi.h"
#include "LdfEvent/Gem.h"
#include "OverlayEvent/OverlayEventModel.h"
#include "OverlayEvent/TkrOverlay.h"
#include "OverlayEvent/GemOverlay.h"

#include "TkrUtil/ITkrGeometrySvc.h"
#include "TkrUtil/ITkrMakeClustersTool.h"
#include "TkrUtil/ITkrGhostTool.h"

#include <map>

class TkrOverlayMergeAlg : public Algorithm 
{
 public:

    TkrOverlayMergeAlg(const std::string&, ISvcLocator*);
    StatusCode initialize();
    StatusCode execute();
    StatusCode finalize();

 private:
    /// Local method to copy information 
    Event::TkrDigi* copyTkrOverlay(Event::TkrOverlay* TkrOverlay);

    /// Pointer to the overlay's data provider service
    DataSvc*         m_dataSvc;

    /// Type of tool to run
    std::string           m_type;
    /// Pointer to the tracker geometry service
    ITkrGeometrySvc*      m_tkrGeom;
    /// Pointer to the tracker splits service
    ITkrSplitsSvc*        m_tspSvc;
    /// ptr to ghost tool
    ITkrGhostTool*        m_ghostTool;
    /// ptr to cluster tool
    ITkrMakeClustersTool* m_clustersTool;

};

// Used by Gaudi for identifying this algorithm
//static const AlgFactory<TkrOverlayMergeAlg>    Factory;
//const IAlgFactory& TkrOverlayMergeAlgFactory = Factory;
DECLARE_ALGORITHM_FACTORY(TkrOverlayMergeAlg);

TkrOverlayMergeAlg::TkrOverlayMergeAlg(const std::string& name,
                                         ISvcLocator* pSvcLocator)
    : Algorithm(name, pSvcLocator) 
{
    // variable to select the tool type
    declareProperty("Type", m_type="General");
}


StatusCode TkrOverlayMergeAlg::initialize() 
{
    // Purpose and Method: initializes TkrOverlayMergeAlg
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

    // Get the Tkr Geometry service 
    sc = service("TkrGeometrySvc", m_tkrGeom, true);
    if ( sc.isFailure() ) 
    {
        log << MSG::ERROR << "Couldn't set up TkrGeometrySvc!" << endreq;
        return sc;
    }

    // Get the splits service 
    m_tspSvc = m_tkrGeom->getTkrSplitsSvc();
    if ( !m_tspSvc ) 
    {
        log << MSG::ERROR
            << "Couldn't set up TkrSplitsSvc!" << std::endl;
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

    m_ghostTool = 0;
    sc = toolSvc()->retrieveTool("TkrGhostTool",m_ghostTool) ;
    if (sc.isFailure()) 
    {
        log<<MSG::WARNING << "Failed to retrieve ghost tool" << endreq ;
        return StatusCode::FAILURE ;
    }

    m_clustersTool = 0;
    sc = toolSvc()->retrieveTool("TkrMakeClustersTool",m_clustersTool) ;
    if (sc.isFailure()) 
    {
        log<<MSG::WARNING << "Failed to retrieve ghost tool" << endreq ;
        return StatusCode::FAILURE ;
    }

    return sc;
}


StatusCode TkrOverlayMergeAlg::execute() 
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
    SmartDataPtr<Event::TkrOverlayCol> overlayCol(m_dataSvc, m_dataSvc->rootName() + OverlayEventModel::Overlay::TkrOverlayCol);
    if(!overlayCol) return StatusCode::SUCCESS;

    // Now recover the digis for this event
    SmartDataPtr<Event::TkrDigiCol> tkrDigiCol(eventSvc(), EventModel::Digi::TkrDigiCol);
/*
    // do the TkrVector shenanigans
    // make the tkrVector
    unsigned short towerBits = 0;
    m_ghostTool->calculateTkrVector(tkrDigiCol, towerBits);

    // Recover the GemOverlay from the TDS
    SmartDataPtr<Event::GemOverlay> gemOverlay(eventSvc(), OverlayEventModel::Overlay::GemOverlay);

    // get the tkrVector of the overlay
    unsigned short tkrVector = 0;
    if (gemOverlay!=0) tkrVector = gemOverlay->getTkrVector();

    // OR the two sources
    tkrVector |= towerBits;

    // Is there a Gem object in the TDS already? (should not be...)
    SmartDataPtr<LdfEvent::Gem> gem(eventSvc(), "Event/Gem");

    if (!gem)
    {
        // Make a new Gem object
        gem = new LdfEvent::Gem();

        // Register it in the TDS
        if ((sc = eventSvc()->registerObject("/Event/Gem", gem)).isFailure())
        {
            log << MSG::ERROR << "could not register /Event/Gem" << endreq;
            return sc;
        }
    }

    // store the result in the event
    gem->setTkrVector( tkrVector );
*/
    // Create a map of the simulation digis, indexing by identifier
    std::map<int, Event::TkrDigi*> idToDigiMap;

    for(Event::TkrDigiCol::iterator tkrIter = tkrDigiCol->begin(); tkrIter != tkrDigiCol->end(); tkrIter++)
    {
        Event::TkrDigi* tkrDigi = *tkrIter;
        int             digiId  = (*tkrDigi);

        // Should be done elsewhere...
        tkrDigi->addToStatus(Event::TkrDigi::DIGI_MC);

        // Add this digi to our map
        idToDigiMap[digiId] = tkrDigi;
    }

    // Now loop through the overlay digis to merge into our simulated ones
    for(Event::TkrOverlayCol::iterator overIter = overlayCol->begin(); overIter != overlayCol->end(); overIter++)
    {
        Event::TkrOverlay* tkrOverlay = *overIter;
        int                overId     = (*tkrOverlay);

        std::map<int,Event::TkrDigi*>::iterator tkrIter = idToDigiMap.find(overId);

        // If this digi is not already in the TkrDigiCol then no merging needed, just add it
        if (tkrIter == idToDigiMap.end())
        {
            Event::TkrDigi* digi = copyTkrOverlay(tkrOverlay);

            tkrDigiCol->push_back(digi);
        }
        // Otherwise, we need to merge the digi
        else
        {
            // Pointer to the TDS digi
            Event::TkrDigi* tkrDigi = tkrIter->second;

            // Retrieve tower, bilayer and view information
            idents::TowerId         towerId = tkrOverlay->getTower();
            int                     biLayer = tkrOverlay->getBilayer();
            idents::GlastAxis::axis view    = tkrOverlay->getView();

            // Get the break point for this tower, bi-layer and view
            int breakPoint = m_tspSvc->getSplitPoint(towerId.id(), biLayer, view);
            int lastStrip0 = tkrDigi->getLastController0Strip();

            // Recover the ToT (note that "addHit" will take care of ToT for us)
            int tot0 = tkrDigi->getToT(0) > tkrOverlay->getToT(0) ? tkrDigi->getToT(0) : tkrOverlay->getToT(0);
            int tot1 = tkrDigi->getToT(1) > tkrOverlay->getToT(1) ? tkrDigi->getToT(1) : tkrOverlay->getToT(1);

            // Merge hit strips, loop over hits in the overlay digi
            for(std::vector<int>::const_iterator overStripIter = tkrOverlay->begin(); overStripIter != tkrOverlay->end(); overStripIter++)
            {
                int overStripId = *overStripIter;

                // Check the last strip stuff
                if (overStripId < breakPoint && overStripId > lastStrip0) lastStrip0 = overStripId;

                // Go through following contortions to find correct slot to insert strip id
                bool notFound = true;
                for(std::vector<int>::iterator stripIter = tkrDigi->begin(); stripIter != tkrDigi->end(); stripIter++)
                {
                    if (overStripId >  *stripIter) continue;
                    
                    notFound = false;

                    if (overStripId == *stripIter) break;
                    tkrDigi->insert(stripIter, overStripId);
                    break;
                }

                if (notFound) tkrDigi->push_back(overStripId);
            }

            // Update the ToT and lastStrip info
            tkrDigi->setToT(0,tot0);
            tkrDigi->setToT(1,tot1);
            tkrDigi->setLastController0Strip(lastStrip0);
            tkrDigi->addToStatus(Event::TkrDigi::DIGI_OVERLAY);
        }
    }

    // sort by volume id
    std::sort(tkrDigiCol->begin(), tkrDigiCol->end(), Event::TkrDigi::digiLess());

    return sc;
}

Event::TkrDigi* TkrOverlayMergeAlg::copyTkrOverlay(Event::TkrOverlay* TkrOverlay)
{
    // This method will create a new TkrDigi object using the information from the 
    // input TkrOverlay object. 

    // Recover ToT information into local array
    int totTds[2] = { TkrOverlay->getToT(0), TkrOverlay->getToT(1)};

    // Get new TkrOverlay object
    Event::TkrDigi* tkrDigi = new Event::TkrDigi(TkrOverlay->getBilayer(),
                                                 TkrOverlay->getView(), 
                                                 TkrOverlay->getTower(), 
                                                 totTds, 
                                                 Event::TkrDigi::DIGI_OVERLAY);

    // Fill in the hit information
    int          lastController0Strip = TkrOverlay->getLastController0Strip();
    unsigned int numStrips            = TkrOverlay->getNumHits();

    // Loop over hits and add them
    for (unsigned int iHit = 0; iHit < numStrips; iHit++) 
    {
        int strip = TkrOverlay->getHit(iHit);

        if (strip <= lastController0Strip) 
        {
            tkrDigi->addC0Hit(strip);
        } 
        else 
        {
            tkrDigi->addC1Hit(strip);
        }
    }

    return tkrDigi;
}

StatusCode TkrOverlayMergeAlg::finalize() 
{
    MsgStream log(msgSvc(), name());
    log << MSG::INFO << "finalize" << endreq;
    return StatusCode::SUCCESS;
}
