// $Header: /nfs/slac/g/glast/ground/cvs/Overlay/src/cnv/PtOverlayCnv.cxx,v 1.7 2012/07/24 16:00:34 usher Exp $
/**
            @file  PtOverlayCnv.cxx

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
#include "OverlayEvent/PtOverlay.h"

#include "overlayRootData/EventOverlay.h"

class  PtOverlayCnv : virtual public IGlastCnv, public Converter 
{
public:

    /**
        Constructor for this converter
        @param svc a ISvcLocator interface to find services
        @param clid the type of object the converter is able to convert
    */
    PtOverlayCnv(ISvcLocator* svc);

    virtual ~PtOverlayCnv();

    /// Query interfaces of Interface
    //virtual StatusCode queryInterface(const InterfaceID& riid, void** ppvInterface);
    static const CLID&         classID()     {return Event::PtOverlay::classID();}
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


 //static CnvFactory<PtOverlayCnv> s_factory;
 //const ICnvFactory& PtOverlayCnvFactory = s_factory;
DECLARE_CONVERTER_FACTORY(PtOverlayCnv);

 PtOverlayCnv::PtOverlayCnv( ISvcLocator* svc) : Converter (storageType(), classID(), svc) 
{
    m_path = OverlayEventModel::Overlay::PtOverlay;

    return;
}

PtOverlayCnv::~PtOverlayCnv() 
{
    return;
}

StatusCode PtOverlayCnv::initialize() 
{
    MsgStream log(msgSvc(), "PtOverlayCnv");
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

StatusCode PtOverlayCnv::finalize() 
{
    return Converter::finalize();
}

// (To TDS) Conversion stuff
StatusCode PtOverlayCnv::createObj(IOpaqueAddress* pOpaque, DataObject*& refpObject) 
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
    const PtOverlay &ptRoot = overlayRoot->getPtOverlay();

    // Create the new PtOverlay event to put in the TDS
    Event::PtOverlay* ptTds = new Event::PtOverlay();

    float sc_position[3];

    sc_position[0] = ptRoot.getSC_Position()[0];
    sc_position[1] = ptRoot.getSC_Position()[1];
    sc_position[2] = ptRoot.getSC_Position()[2];

    ptTds->initPtOverlay(ptRoot.getStartTime(), 
                         sc_position,
                         ptRoot.getLatGeo(),
                         ptRoot.getLonGeo(),
                         ptRoot.getLatMag(),
                         ptRoot.getRadGeo(),
                         ptRoot.getRaScz(),
                         ptRoot.getDecScz(),
                         ptRoot.getRaScx(),
                         ptRoot.getDecScx(),
                         ptRoot.getZenithScz(),
                         ptRoot.getB(),
                         ptRoot.getL(),
                         ptRoot.getLambda(),
                         ptRoot.getR(),
                         ptRoot.getBEast(),
                         ptRoot.getBNorth(),
                         ptRoot.getBUp());


    // Return the pointer to it
    refpObject = ptTds;

    return status;
}
    
StatusCode PtOverlayCnv::createRep(DataObject* pObject, IOpaqueAddress*&)
{
    StatusCode status = StatusCode::SUCCESS;

    if (!m_overlayOutputSvc) return status;

    // Retrieve the pointer to the digi
    EventOverlay* overlayRoot = m_overlayOutputSvc->getRootEventOverlay();

    // Cast our object back to a TkrDigiCol object
    Event::PtOverlay* ptOverlayTds = dynamic_cast<Event::PtOverlay*>(pObject);

    // Local object to fill
    PtOverlay ptOverlayRoot;

    // Copy information from TDS to Root
    ptOverlayRoot.initialize(ptOverlayTds->getStartTime(), 
                             ptOverlayTds->getSC_Position(),
                             ptOverlayTds->getLatGeo(),
                             ptOverlayTds->getLonGeo(),
                             ptOverlayTds->getLatMag(),
                             ptOverlayTds->getRadGeo(),
                             ptOverlayTds->getRaScz(),
                             ptOverlayTds->getDecScz(),
                             ptOverlayTds->getRaScx(),
                             ptOverlayTds->getDecScx(),
                             ptOverlayTds->getZenithScz(),
                             ptOverlayTds->getB(),
                             ptOverlayTds->getL(),
                             ptOverlayTds->getLambda(),
                             ptOverlayTds->getR(),
                             ptOverlayTds->getBEast(),
                             ptOverlayTds->getBNorth(),
                             ptOverlayTds->getBUp());

    // Save the info 
    overlayRoot->setPtOverlay(ptOverlayRoot);

    return status;
}
