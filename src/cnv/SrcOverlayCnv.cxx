// $Header: /nfs/slac/g/glast/ground/cvs/GlastRelease-scons/Overlay/src/cnv/SrcOverlayCnv.cxx,v 1.4 2011/11/03 18:21:15 usher Exp $
/**
            @file  SrcOverlayCnv.cxx

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
#include "OverlayEvent/SrcOverlay.h"

#include "overlayRootData/EventOverlay.h"

class  SrcOverlayCnv : virtual public IGlastCnv, public Converter 
{
public:

    /**
        Constructor for this converter
        @param svc a ISvcLocator interface to find services
        @param clid the type of object the converter is able to convert
    */
    SrcOverlayCnv(ISvcLocator* svc);

    virtual ~SrcOverlayCnv();

    /// Query interfaces of Interface
    //virtual StatusCode queryInterface(const InterfaceID& riid, void** ppvInterface);
    static const CLID&         classID()     {return Event::SrcOverlay::classID();}
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


 //static CnvFactory<SrcOverlayCnv> s_factory;
 //const ICnvFactory& SrcOverlayCnvFactory = s_factory;
DECLARE_CONVERTER_FACTORY(SrcOverlayCnv);

 SrcOverlayCnv::SrcOverlayCnv( ISvcLocator* svc) : 
                 Converter (EXCEL_StorageType, Event::SrcOverlay::classID(), svc) 
{
    m_path = OverlayEventModel::Overlay::SrcOverlay;

    return;
}

SrcOverlayCnv::~SrcOverlayCnv() 
{
    return;
}

StatusCode SrcOverlayCnv::initialize() 
{
    MsgStream log(msgSvc(), "SrcOverlayCnv");
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
        m_overlayOutputSvc = dynamic_cast<IOverlayDataSvc*>(tmpService);
    }

    return status;
}

StatusCode SrcOverlayCnv::finalize() 
{
    return Converter::finalize();
}

// (To TDS) Conversion stuff
StatusCode SrcOverlayCnv::createObj(IOpaqueAddress* pOpaque, DataObject*& refpObject) 
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

    IOverlayDataSvc* inputDataSvc = dynamic_cast<IOverlayDataSvc*>(pDataSvc);

    // Create the new SrcOverlay event to put in the TDS
    Event::SrcOverlay* overlayTds = new Event::SrcOverlay();

    // Retrieve the pointer to the digi
    EventOverlay* overlayRoot = inputDataSvc->getRootEventOverlay();

    // Initialize the overlay object
    overlayTds->initialize(overlayRoot->getFromMc());

    // Return the pointer to it
    refpObject = overlayTds;

    return status;
}
    
StatusCode SrcOverlayCnv::createRep(DataObject*, IOpaqueAddress*&)
{
    StatusCode status = StatusCode::SUCCESS;

    if (!m_overlayOutputSvc) return status;

    return status;
}
