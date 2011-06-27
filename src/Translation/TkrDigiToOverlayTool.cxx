/**  @file TkrDigiToOverlayTool.cxx
    @brief implementation of class TkrDigiToOverlayTool
    
  $Header: /nfs/slac/g/glast/ground/cvs/Overlay/src/Translation/TkrDigiToOverlayTool.cxx,v 1.1 2008/12/01 22:50:12 usher Exp $  
*/

#include "IDigiToOverlayTool.h"

#include "GaudiKernel/ToolFactory.h"
#include "GaudiKernel/AlgTool.h"
#include "GaudiKernel/SmartDataPtr.h"
#include "GaudiKernel/GaudiException.h" 
#include "GaudiKernel/IDataProviderSvc.h"
#include "GaudiKernel/DataSvc.h"

#include "Event/TopLevel/EventModel.h"
#include "Event/Digi/TkrDigi.h"

#include "OverlayEvent/OverlayEventModel.h"
#include "OverlayEvent/TkrOverlay.h"

/** @class BackgroundSelection
    @brief manage the selection of background events to merge with signal events
    @author Dan Flath

It uses the name of a tuple variable as a key to look up trigger and downlink rates of a background
source depending on the value of that variable. See the corresponding member functions. Given such
a value, it expects to find a source of such events generated with that value which can be inserted 
into the Gleam output tuple.

*/
class TkrDigiToOverlayTool : public AlgTool, virtual public IDigiToOverlayTool
{
public:

    // Standard Gaudi Tool constructor
    TkrDigiToOverlayTool(const std::string& type, const std::string& name, const IInterface* parent);

    // After building it, destroy it
    ~TkrDigiToOverlayTool();

    /// @brief Intialization of the tool
    StatusCode initialize();

    /// @brief Finalize method for the tool
    StatusCode finalize();

    ///! The current value of the quantity that we are selecting on
    StatusCode translate();

private:

    /// Pointer to the event data service
    IDataProviderSvc* m_edSvc;

    /// Pointer to the Overlay data service
    DataSvc*          m_dataSvc;
};

static ToolFactory<TkrDigiToOverlayTool> s_factory;
const IToolFactory& TkrDigiToOverlayToolFactory = s_factory;

//------------------------------------------------------------------------
TkrDigiToOverlayTool::TkrDigiToOverlayTool(const std::string& type, 
                                 const std::string& name, 
                                 const IInterface* parent) :
                                 AlgTool(type, name, parent)
{
    //Declare the additional interface
    declareInterface<IDigiToOverlayTool>(this);
}
//------------------------------------------------------------------------
TkrDigiToOverlayTool::~TkrDigiToOverlayTool()
{
    return;
}

StatusCode TkrDigiToOverlayTool::initialize()
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

    sc = serviceLocator()->service("OverlayDataSvc", iService, true);
    if ( sc.isFailure() ) {
        log << MSG::ERROR << "could not find EventDataSvc !" << endreq;
        return sc;
    }
    m_dataSvc = dynamic_cast<DataSvc*>(iService);

    return sc;
}

StatusCode TkrDigiToOverlayTool::finalize ()
{
    StatusCode  status = StatusCode::SUCCESS;
    
    return status;
}

//------------------------------------------------------------------------
StatusCode TkrDigiToOverlayTool::translate()
{
    MsgStream log(msgSvc(), name());
    StatusCode status = StatusCode::SUCCESS;

    // Recover the TkrDigi collection from the TDS
    SmartDataPtr<Event::TkrDigiCol> tkrDigiCol(m_edSvc, EventModel::Digi::TkrDigiCol);

    // Create a collection of TkrOverlays and register in the TDS
    SmartDataPtr<Event::TkrOverlayCol> overlayCol(m_dataSvc, m_dataSvc->rootName() + OverlayEventModel::Overlay::TkrOverlayCol);
    if(!overlayCol)
    {
        overlayCol = new Event::TkrOverlayCol();

        status = m_dataSvc->registerObject(m_dataSvc->rootName() + OverlayEventModel::Overlay::TkrOverlayCol, overlayCol);
        if( status.isFailure() ) 
        {
            log << MSG::ERROR << "could not register OverlayEventModel::Overlay::TkrOverlayCol" << endreq;
            return status;
        }
    }

    // Loop over available TkrDigis
    for(Event::TkrDigiCol::iterator tkrIter = tkrDigiCol->begin(); tkrIter != tkrDigiCol->end(); tkrIter++)
    {
        // Dereference our TkrDigi object
        Event::TkrDigi* tkrDigi = *tkrIter;

        // Extract the ToT into a local array
        int tot[2] = { tkrDigi->getToT(0), tkrDigi->getToT(1)};

        // Create a shiny new TkrOverlay object from the TkrDigi
        Event::TkrOverlay *tkrOverlay = new Event::TkrOverlay(tkrDigi->getBilayer(),
                                                              tkrDigi->getView(), 
                                                              tkrDigi->getTower(), 
                                                              tot, 
                                                              Event::TkrOverlay::DIGI_OVERLAY);

        // Set up to loop over hit strips, get the last controller strip and number of hits
        int          lastController0Strip = tkrDigi->getLastController0Strip();
        unsigned int numStrips            = tkrDigi->getNumHits();

        // Loop over hits and add them to our TkrOverlay object
        for (unsigned int iHit = 0; iHit < numStrips; iHit++) 
        {
            int strip = tkrDigi->getHit(iHit);

            if (strip <= lastController0Strip) 
            {
                tkrOverlay->addC0Hit(strip);
            } 
            else 
            {
                tkrOverlay->addC1Hit(strip);
            }
        }

        // Add the finished product to our collection
        overlayCol->push_back(tkrOverlay);
    }

    return status;
}
