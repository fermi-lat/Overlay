// $Header: /nfs/slac/g/glast/ground/cvs/Overlay/src/cnv/EventOverlayCnv.cxx,v 1.2 2009/09/15 19:20:05 usher Exp $
/**
            @file  EventOverlayCnv.cxx

   Implementation file for Root calibration converter base class
*/
#include "Overlay/IOverlayDataSvc.h"

#include "GaudiKernel/Converter.h"
#include "GaudiKernel/SvcFactory.h"
#include "GaudiKernel/CnvFactory.h"
#include "GaudiKernel/SmartIF.h"
#include "GaudiKernel/DataObject.h"
#include "GaudiKernel/MsgStream.h"
#include "GaudiKernel/SmartDataPtr.h"

#include "GaudiKernel/IOpaqueAddress.h"
#include "GaudiKernel/IAddressCreator.h"
#include "GaudiKernel/IDataProviderSvc.h"
#include "GaudiKernel/IConversionSvc.h"
#include "GaudiKernel/IDataManagerSvc.h"

#include "GlastSvc/EventSelector/IGlastCnv.h"

#include "OverlayEvent/OverlayEventModel.h"
#include "OverlayEvent/EventOverlay.h"
#include "OverlayEvent/SrcOverlay.h"

#include "Trigger/TriRowBits.h"
#include "LdfEvent/LdfTime.h"

#include "overlayRootData/EventOverlay.h"

class  EventOverlayCnv : virtual public IGlastCnv, public Converter 
{
public:

    /**
        Constructor for this converter
        @param svc a ISvcLocator interface to find services
        @param clid the type of object the converter is able to convert
    */
    EventOverlayCnv(ISvcLocator* svc);

    virtual ~EventOverlayCnv();

    /// Query interfaces of Interface
    //virtual StatusCode queryInterface(const InterfaceID& riid, void** ppvInterface);
    static const CLID&         classID()     {return Event::EventOverlay::classID();}
    static const unsigned char storageType() {return SICB_StorageType;}

    /// Initialize the converter
    virtual StatusCode initialize();

    /// Initialize the converter
    virtual StatusCode finalize();

    /// Retrieve the class type of objects the converter produces. 
    virtual const CLID& objType() const {return classID();}

    /// Retrieve the class type of the data store the converter uses.
    // MSF: Masked to generate compiler error due to interface change
    virtual long repSvcType() const {return Converter::i_repSvcType();}

    /** Create the transient representation of an object.
        The transient representation is created by loading the 
        persistent object using the source information
        contained in the address.
        @return    Status code indicating success or failure
        @param     pAddress   Opaque address information to retrieve the 
                              requested object from the store in order to 
                              produce the transient object.
        @param     refpObject Reference to location of pointer of the 
                              created object.
     */
    virtual StatusCode createObj(IOpaqueAddress* pAddress,DataObject*& refpObject);

    /** Convert the transient object to the requested representation.
        e.g. conversion to persistent objects.
        @return    Status code indicating success or failure
        @param     pObject     Pointer to location of the object 
        @param     refpAddress Reference to location of pointer with the 
                               object address.
     */
    virtual StatusCode createRep(DataObject* pObject, IOpaqueAddress*& refpAddress);

    /// Methods to set and return the path in TDS for output of this converter
    virtual void setPath(const std::string& path) {m_path = path;}
    virtual const std::string& getPath() const    {return m_path;}

private:
    std::string      m_path;

    IOverlayDataSvc* m_overlayInputSvc;
    IOverlayDataSvc* m_overlayOutputSvc;
};


 //static CnvFactory<EventOverlayCnv> s_factory;
 //const ICnvFactory& EventOverlayCnvFactory = s_factory;
DECLARE_CONVERTER_FACTORY(EventOverlayCnv);

 EventOverlayCnv::EventOverlayCnv( ISvcLocator* svc) : Converter (SICB_StorageType, Event::EventOverlay::classID(), svc) 
{
    m_path = OverlayEventModel::Overlay::EventOverlay;

    return;
}

EventOverlayCnv::~EventOverlayCnv() 
{
    return;
}

StatusCode EventOverlayCnv::initialize() 
{
    MsgStream log(msgSvc(), "EventOverlayCnv");
    StatusCode status = Converter::initialize();

    //SmartIF<IDataManagerSvc> iaddrReg(IID_IDataManagerSvc, dataProvider());

    // We're going rogue here, look up the OverlayDataSvc and use this as 
    // our data provider insteand of EventCnvSvc
    IService* tmpService = 0;
    if (service("OverlayInputSvc", tmpService, false).isFailure())
    {
        log << MSG::INFO << "No OverlayInputSvc available, no input conversion will be performed" << endreq;
        m_overlayInputSvc = 0;
    }
    else m_overlayInputSvc = SmartIF<IOverlayDataSvc>(IID_IOverlayDataSvc, tmpService);

    // Now look up the output data service
    if (service("OverlayOutputSvc", tmpService, false).isFailure())
    {
        log << MSG::INFO << "No OverlayOutputSvc available, no input conversion will be performed" << endreq;
        m_overlayOutputSvc = 0;
    }
    else m_overlayOutputSvc = SmartIF<IOverlayDataSvc>(IID_IOverlayDataSvc, tmpService);

    if (m_overlayOutputSvc) m_overlayOutputSvc->registerOutputPath(m_path);

    return status;
}

StatusCode EventOverlayCnv::finalize() 
{
    return Converter::finalize();
}

// (To TDS) Conversion stuff
StatusCode EventOverlayCnv::createObj(IOpaqueAddress*, DataObject*& refpObject) 
{
    StatusCode status = StatusCode::SUCCESS;

    // If no service then we are not inputting from PDS
    if (!m_overlayInputSvc) return StatusCode::FAILURE;

    // Create the new DigiOverlay event to put in the TDS
    Event::EventOverlay* eventTds = new Event::EventOverlay();

    // Retrieve the pointer to the digi
    EventOverlay* overlayRoot = m_overlayInputSvc->getRootEventOverlay();

    // Initialize the overlay object

    unsigned int eventIdRoot = overlayRoot->getEventId();
    unsigned int runIdRoot   = overlayRoot->getRunId();

    // Check to see if the event and run ids have already been set.
    eventTds->setEvent(eventIdRoot);
    eventTds->setRun(runIdRoot);

    TimeStamp timeObj(overlayRoot->getTimeStamp());
    eventTds->setTime(timeObj);

    eventTds->setLivetime(overlayRoot->getLiveTime());

//    eventTds->setTrigger(overlayRoot->getL1T().getTriggerWord());
//    eventTds->setTriggerWordTwo(overlayRoot->getL1T().getTriggerWordTwo());
//    eventTds->setGemPrescale(overlayRoot->getL1T().getGemPrescale());
//    eventTds->setGltPrescale(overlayRoot->getL1T().getGltPrescale());
//    eventTds->setPrescaleExpired(overlayRoot->getL1T().getPrescaleExpired());

    // Return the pointer to it
    refpObject = eventTds;

    return status;
}
    
StatusCode EventOverlayCnv::createRep(DataObject* pObject, IOpaqueAddress*&)
{
    StatusCode status = StatusCode::SUCCESS;

    if (!m_overlayOutputSvc) return status;

    // Retrieve the pointer to the digi
    EventOverlay* overlayRoot = m_overlayOutputSvc->getRootEventOverlay();

    // Cast back to our TDS object
    Event::EventOverlay* evtTds = dynamic_cast<Event::EventOverlay*>(pObject);

    UInt_t    evtId    = evtTds->event();
    UInt_t    runId    = evtTds->run();
    TimeStamp timeObj  = evtTds->time();
    Double_t  liveTime = evtTds->livetime();

    SmartDataPtr<Event::SrcOverlay> srcTds(dataProvider(), OverlayEventModel::Overlay::SrcOverlay);
    Bool_t fromMc = (srcTds) ? srcTds->fromMc() : true;

//    SmartDataPtr<TriRowBitsTds::TriRowBits> triRowBitsTds(dataProvider(), "/Event/TriRowBits");
//    UInt_t digiRowBits[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
//    UInt_t trgReqRowBits[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
//
//    if (triRowBitsTds) 
//    {
//        unsigned int iTower;
//        for(iTower = 0; iTower < 16; iTower++)
//        {
//            digiRowBits[iTower] = triRowBitsTds->getDigiTriRowBits(iTower);
//            trgReqRowBits[iTower] = triRowBitsTds->getTrgReqTriRowBits(iTower);
//        }
//    }
//
//    L1T levelOne(evtTds->trigger(), digiRowBits, trgReqRowBits);
//    levelOne.setTriggerWordTwo(evtTds->triggerWordTwo());
//    levelOne.setPrescale(evtTds->gemPrescale(), evtTds->gltPrescale(), evtTds->prescaleExpired());

    overlayRoot->initialize(evtId, runId, timeObj.time(), liveTime, fromMc);
    
//    SmartDataPtr<LdfEvent::LdfTime> timeTds(dataProvider(), "/Event/Time");
//    if (timeTds) 
//    {
//        overlayRoot->setEbfTime(timeTds->timeSec(), timeTds->timeNanoSec(),
//                             timeTds->upperPpcTimeBaseWord(), 
//                             timeTds->lowerPpcTimeBaseWord());
//    }

    return status;
}
