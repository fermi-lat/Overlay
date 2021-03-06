// $Header: /nfs/slac/g/glast/ground/cvs/Overlay/src/cnv/GemOverlayCnv.cxx,v 1.5 2011/12/12 20:54:56 heather Exp $
/**
            @file  GemOverlayCnv.cxx

   Implementation file for Root calibration converter base class
*/
#include "Overlay/IOverlayDataSvc.h"

#include "GaudiKernel/Converter.h"
#include "GaudiKernel/SvcFactory.h"
#include "GaudiKernel/CnvFactory.h"
#include "GaudiKernel/SmartIF.h"
#include "GaudiKernel/DataObject.h"
#include "GaudiKernel/MsgStream.h"

#include "GaudiKernel/IOpaqueAddress.h"
#include "GaudiKernel/IRegistry.h"
#include "GaudiKernel/IAddressCreator.h"
#include "GaudiKernel/IDataProviderSvc.h"
#include "GaudiKernel/IConversionSvc.h"
#include "GaudiKernel/IDataManagerSvc.h"

#include "GlastSvc/EventSelector/IGlastCnv.h"

#include "OverlayEvent/OverlayEventModel.h"
#include "OverlayEvent/GemOverlay.h"

#include "overlayRootData/EventOverlay.h"

class  GemOverlayCnv : virtual public IGlastCnv, public Converter 
{
public:

    /**
        Constructor for this converter
        @param svc a ISvcLocator interface to find services
        @param clid the type of object the converter is able to convert
    */
    GemOverlayCnv(ISvcLocator* svc);

    virtual ~GemOverlayCnv();

    /// Query interfaces of Interface
    //virtual StatusCode queryInterface(const InterfaceID& riid, void** ppvInterface);
    static const CLID&         classID()     {return Event::GemOverlay::classID();}
    static const unsigned char storageType() {return EXCEL_StorageType;}

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

    IOverlayDataSvc* m_overlayOutputSvc;
};


 //static CnvFactory<GemOverlayCnv> s_factory;
 //const ICnvFactory& GemOverlayCnvFactory = s_factory;
DECLARE_CONVERTER_FACTORY(GemOverlayCnv);

 GemOverlayCnv::GemOverlayCnv( ISvcLocator* svc) : Converter (EXCEL_StorageType, Event::GemOverlay::classID(), svc) 
{
    m_path = OverlayEventModel::Overlay::GemOverlay;

    return;
}

GemOverlayCnv::~GemOverlayCnv() 
{
    return;
}

StatusCode GemOverlayCnv::initialize() 
{
    MsgStream log(msgSvc(), "GemOverlayCnv");
    StatusCode status = Converter::initialize();

    //SmartIF<IDataManagerSvc> iaddrReg(IID_IDataManagerSvc, dataProvider());

    // We're going rogue here, look up the OverlayDataSvc and use this as 
    // our data provider insteand of EventCnvSvc
    IDataProviderSvc* tmpService = 0;
    if (serviceLocator()->service("OverlayOutputSvc", tmpService, false).isFailure())
    {
        log << MSG::INFO << "No OverlayOutputSvc available, no output conversion will be performed" << endreq;
        m_overlayOutputSvc = 0;
    }
    else 
    {
        // Need to up convert to point to the OverlayDataSvc
        m_overlayOutputSvc = SmartIF<IOverlayDataSvc>(tmpService);
    }

    if (m_overlayOutputSvc) m_overlayOutputSvc->registerOutputPath(m_path);

    return status;
}

StatusCode GemOverlayCnv::finalize() 
{
    return Converter::finalize();
}

// (To TDS) Conversion stuff
StatusCode GemOverlayCnv::createObj(IOpaqueAddress* pOpaque, DataObject*& refpObject) 
{
    StatusCode status = StatusCode::SUCCESS;

    // If no opaque address then there is nothing to do
    if (!pOpaque) return StatusCode::FAILURE;

    // Recover the pointer to the registry
    IRegistry* pRegistry = pOpaque->registry();

    if (!pRegistry) return StatusCode::FAILURE;

    // Recover pointer to the data provider service
    IDataProviderSvc* pDataSvc = pRegistry->dataSvc();

    if (!pDataSvc) return StatusCode::FAILURE;

    IOverlayDataSvc* inputDataSvc = SmartIF<IOverlayDataSvc>(pDataSvc);

    // Retrieve the pointer to the digi
    EventOverlay* overlayRoot = inputDataSvc->getRootEventOverlay();

    // Extract GEM information from input digis
    const GemOverlay &gemRoot = overlayRoot->getGemOverlay();

    // Create the new GemOverlay event to put in the TDS
    Event::GemOverlay* gemTds = new Event::GemOverlay();

    GemOverlayTileList tileListRoot = gemRoot.getTileList();
    Event::GemOverlayTileList tileListTds(tileListRoot.getXzm(), 
                                          tileListRoot.getXzp(), 
                                          tileListRoot.getYzm(), 
                                          tileListRoot.getYzp(), 
                                          tileListRoot.getXy(), 
                                          tileListRoot.getRbn(), 
                                          tileListRoot.getNa());

    gemTds->initTrigger(gemRoot.getTkrVector(), gemRoot.getRoiVector(),
            gemRoot.getCalLeVector(), gemRoot.getCalHeVector(),
            gemRoot.getCnoVector(), gemRoot.getConditionSummary(),
            gemRoot.getMissed(), tileListTds);

    Event::GemOverlayOnePpsTime ppsTimeTds(gemRoot.getOnePpsTime().getTimebase(),
                            gemRoot.getOnePpsTime().getSeconds());
    Event::GemOverlayDataCondArrivalTime gemCondTimeTds;
    gemCondTimeTds.init(gemRoot.getCondArrTime().condArr());
    gemTds->initSummary(gemRoot.getLiveTime(), gemRoot.getPrescaled(),
                        gemRoot.getDiscarded(), gemCondTimeTds,
                        gemRoot.getTriggerTime(), ppsTimeTds, 
                        gemRoot.getDeltaEventTime(), 
                        gemRoot.getDeltaWindowOpenTime());

    // Return the pointer to it
    refpObject = gemTds;

    return status;
}
    
StatusCode GemOverlayCnv::createRep(DataObject* pObject, IOpaqueAddress*&)
{
    StatusCode status = StatusCode::SUCCESS;

    if (!m_overlayOutputSvc) return status;

    // Retrieve the pointer to the digi
    EventOverlay* overlayRoot = m_overlayOutputSvc->getRootEventOverlay();

    // Cast our object back to a TkrDigiCol object
    Event::GemOverlay* gemOverlayTds = dynamic_cast<Event::GemOverlay*>(pObject);

    // Local object to fill
    GemOverlay gemOverlayRoot;

    // Copy information from TDS to Root
    Event::GemOverlayTileList tileListTds = gemOverlayTds->getTileList();
    GemOverlayTileList tileListRoot(tileListTds.getXzm(), 
                                    tileListTds.getXzp(), 
                                    tileListTds.getYzm(), 
                                    tileListTds.getYzp(), 
                                    tileListTds.getXy(), 
                                    tileListTds.getRbn(), 
                                    tileListTds.getNa());

    gemOverlayRoot.initTrigger(gemOverlayTds->getTkrVector(), 
                               gemOverlayTds->getRoiVector(),
                               gemOverlayTds->getCalLEvector(), 
                               gemOverlayTds->getCalHEvector(),
                               gemOverlayTds->getCnoVector(), 
                               gemOverlayTds->getConditionSummary(),
                               gemOverlayTds->getMissed(), 
                               tileListRoot);

    GemOverlayOnePpsTime ppsTimeRoot(gemOverlayTds->getOnePpsTime().getTimebase(),
                                     gemOverlayTds->getOnePpsTime().getSeconds());

    gemOverlayRoot.initSummary(gemOverlayTds->getLiveTime(), 
                               gemOverlayTds->getPrescaled(),
                               gemOverlayTds->getDiscarded(), 
                               gemOverlayTds->getCondArrTime().condArr(),
                               gemOverlayTds->getTriggerTime(), 
                               ppsTimeRoot, 
                               gemOverlayTds->getDeltaEventTime(), 
                               gemOverlayTds->getDeltaWindowOpenTime());

    // Save the info 
    overlayRoot->setGemOverlay(gemOverlayRoot);

    return status;
}
