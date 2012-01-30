// $Header: /nfs/slac/g/glast/ground/cvs/Overlay/src/cnv/CalOverlayiCnv.cxx,v 1.2 2009/09/15 19:20:05 usher Exp $
/**
            @file  CalOverlayCnv.cxx

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
#include "GaudiKernel/IAddressCreator.h"
#include "GaudiKernel/IDataProviderSvc.h"
#include "GaudiKernel/IConversionSvc.h"
#include "GaudiKernel/IDataManagerSvc.h"

#include "GlastSvc/EventSelector/IGlastCnv.h"

#include "OverlayEvent/OverlayEventModel.h"
#include "OverlayEvent/CalOverlay.h"

#include "overlayRootData/EventOverlay.h"

class  CalOverlayCnv : virtual public IGlastCnv, public Converter 
{
public:

    /**
        Constructor for this converter
        @param svc a ISvcLocator interface to find services
        @param clid the type of object the converter is able to convert
    */
    CalOverlayCnv(ISvcLocator* svc);

    virtual ~CalOverlayCnv();

    /// Query interfaces of Interface
    //virtual StatusCode queryInterface(const InterfaceID& riid, void** ppvInterface);
    static const CLID&         classID()     {return ObjectVector<Event::CalOverlay>::classID();}
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


 //static CnvFactory<CalOverlayCnv> s_factory;
 //const ICnvFactory& CalOverlayCnvFactory = s_factory;
DECLARE_CONVERTER_FACTORY(CalOverlayCnv);

 CalOverlayCnv::CalOverlayCnv( ISvcLocator* svc) : Converter (SICB_StorageType, ObjectVector<Event::CalOverlay>::classID(), svc) 
{
    m_path = OverlayEventModel::Overlay::CalOverlayCol;

    return;
}

CalOverlayCnv::~CalOverlayCnv() 
{
    return;
}

StatusCode CalOverlayCnv::initialize() 
{
    MsgStream log(msgSvc(), "CalOverlayCnv");
    StatusCode status = Converter::initialize();

    //SmartIF<IDataManagerSvc> iaddrReg(IID_IDataManagerSvc, dataProvider());

    // We're going rogue here, look up the OverlayDataSvc and use this as 
    // our data provider insteand of EventCnvSvc
    SmartIF<IDataManagerSvc> iaddrReg(IID_IDataManagerSvc, dataProvider());

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

StatusCode CalOverlayCnv::finalize() 
{
    return Converter::finalize();
}

// (To TDS) Conversion stuff
StatusCode CalOverlayCnv::createObj(IOpaqueAddress*, DataObject*& refpObject) 
{
    StatusCode status = StatusCode::SUCCESS;

    // If no service then we are not inputting from PDS
    if (!m_overlayInputSvc) return StatusCode::FAILURE;

    // Retrieve the pointer to the digi
    EventOverlay* overlayRoot = m_overlayInputSvc->getRootEventOverlay();

    // Check that we have a TkrDigi collection in the input root data
    const TObjArray *calOverlayRootCol = overlayRoot->getCalOverlayCol();
    if (!calOverlayRootCol) return status;

    // Make a TIter object for traversing the collection
    TIter CalOverlayIter(calOverlayRootCol);

    // Create the new TkrDigi event to put in the TDS
    Event::CalOverlayCol* calOverlayTdsCol = new Event::CalOverlayCol;

    CalOverlay *calOverlayRoot = 0;
    while ((calOverlayRoot = (CalOverlay*)CalOverlayIter.Next())!=0) 
    {
        CalXtalId idRoot = calOverlayRoot->getPackedId();
        idents::CalXtalId idTds(idRoot.getTower(), idRoot.getLayer(), idRoot.getColumn());

        const TVector3& posRoot = calOverlayRoot->getPosition();
        Point posTds(posRoot.X(),posRoot.Y(),posRoot.Z());

        Event::CalOverlay *calOverlayTds = new Event::CalOverlay(idTds, posTds, calOverlayRoot->getEnergy());

        calOverlayTds->setStatus(calOverlayRoot->getStatus());

        calOverlayTds->addToStatus(Event::CalOverlay::DIGI_OVERLAY);

        calOverlayTdsCol->push_back(calOverlayTds);
    }

    // Return the pointer to it
    refpObject = calOverlayTdsCol;

    return status;
}
    
StatusCode CalOverlayCnv::createRep(DataObject* pObject, IOpaqueAddress*&)
{
    StatusCode status = StatusCode::SUCCESS;

    if (!m_overlayOutputSvc) return status;

    // Retrieve the pointer to the digi
    EventOverlay* overlayRoot = m_overlayOutputSvc->getRootEventOverlay();

    // Cast our object back to a TkrDigiCol object
    Event::CalOverlayCol* CalOverlayColTds = dynamic_cast<Event::CalOverlayCol*>(pObject);

    // Loop over the collection and creat root versions of TkrDigi objects
    for (Event::CalOverlayCol::const_iterator calOverlayTds  = CalOverlayColTds->begin(); 
                                              calOverlayTds != CalOverlayColTds->end(); 
                                              calOverlayTds++) 
    {
        // Convert the volume identifier info
        idents::CalXtalId idTds = (*calOverlayTds)->getCalXtalId();
        CalXtalId idRoot(idTds.getTower(), idTds.getLayer(), idTds.getColumn());

        // Position next
        const Point positionTds = (*calOverlayTds)->getPosition();
        TVector3 positionRoot(positionTds.x(), positionTds.y(), positionTds.z());

        // Energy
        Double_t energy = (*calOverlayTds)->getEnergy();

        // Status bits
        UInt_t status = (*calOverlayTds)->getStatus();
       
        CalOverlay *calOverlayRoot = new CalOverlay();
        
        calOverlayRoot->initialize(idRoot, positionRoot, energy, status);

        overlayRoot->addCalOverlay(calOverlayRoot);
    }

    return status;
}
