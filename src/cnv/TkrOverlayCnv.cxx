// $Header: /nfs/slac/g/glast/ground/cvs/Overlay/src/cnv/TkrOverlayCnv.cxx,v 1.1 2008/12/02 15:27:17 usher Exp $
/**
            @file  TkrOverlayCnv.cxx

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

#include "Event/TopLevel/EventModel.h"
#include "OverlayEvent/OverlayEventModel.h"
#include "OverlayEvent/TkrOverlay.h"

#include "overlayRootData/EventOverlay.h"

class  TkrOverlayCnv : virtual public IGlastCnv, public Converter 
{
public:

    /**
        Constructor for this converter
        @param svc a ISvcLocator interface to find services
        @param clid the type of object the converter is able to convert
    */
    TkrOverlayCnv(ISvcLocator* svc);

    virtual ~TkrOverlayCnv();

    /// Query interfaces of Interface
    //virtual StatusCode queryInterface(const InterfaceID& riid, void** ppvInterface);
    static const CLID&         classID()     {return ObjectVector<Event::TkrOverlay>::classID();}
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


 static CnvFactory<TkrOverlayCnv> s_factory;
 const ICnvFactory& TkrOverlayCnvFactory = s_factory;

 TkrOverlayCnv::TkrOverlayCnv( ISvcLocator* svc) : Converter (SICB_StorageType, ObjectVector<Event::TkrOverlay>::classID(), svc) 
{
    m_path = OverlayEventModel::Overlay::TkrOverlayCol;

    return;
}

TkrOverlayCnv::~TkrOverlayCnv() 
{
    return;
}

StatusCode TkrOverlayCnv::initialize() 
{
    MsgStream log(msgSvc(), "TkrOverlayCnv");
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

StatusCode TkrOverlayCnv::finalize() 
{
    return Converter::finalize();
}

// (To TDS) Conversion stuff
StatusCode TkrOverlayCnv::createObj(IOpaqueAddress*, DataObject*& refpObject) 
{
    StatusCode status = StatusCode::SUCCESS;

    // If no service then we are not inputting from PDS
    if (!m_overlayInputSvc) return StatusCode::FAILURE;

    Event::TkrOverlayCol* TkrOverlayTdsCol = 0;

    // Retrieve the pointer to the digi
    EventOverlay* overlayRoot = m_overlayInputSvc->getRootEventOverlay();

    // If no overlayRoot then we are not inputting from a file
    if (overlayRoot)
    {
        // Check that we have a TkrDigi collection in the input root data
        const TObjArray *tkrOverlayRootCol = overlayRoot->getTkrOverlayCol();
        if (!tkrOverlayRootCol) return status;

        // Make a TIter object for traversing the collection
        TIter tkrOverlayIter(tkrOverlayRootCol);

        // Create the new TkrDigi event to put in the TDS
        TkrOverlayTdsCol = new Event::TkrOverlayCol;

        // Loop through input digis and make TDS object
        TkrOverlay *tkrOverlayRoot = 0;
        while ((tkrOverlayRoot = (TkrOverlay*)tkrOverlayIter.Next())!=0) 
        {
            TowerId towerRoot = tkrOverlayRoot->getTower();
            idents::TowerId towerTds(towerRoot.ix(), towerRoot.iy());
            GlastAxis::axis axisRoot = tkrOverlayRoot->getView();
            idents::GlastAxis::axis axisTds = 
                (axisRoot == GlastAxis::X) ? idents::GlastAxis::X : idents::GlastAxis::Y;
            int totTds[2] = { tkrOverlayRoot->getToT(0), tkrOverlayRoot->getToT(1)};
            Event::TkrOverlay *tkrDigiTds = new Event::TkrOverlay(tkrOverlayRoot->getBilayer(),
                                                                  axisTds, 
                                                                  towerTds, 
                                                                  totTds, 
                                                                  Event::TkrOverlay::DIGI_OVERLAY);
            int lastController0Strip = tkrOverlayRoot->getLastController0Strip();
            unsigned int numStrips = tkrOverlayRoot->getNumHits();
            unsigned int iHit;
            for (iHit = 0; iHit < numStrips; iHit++) 
            {
                int strip = tkrOverlayRoot->getHit(iHit);
                if (strip <= lastController0Strip) 
                {
                    tkrDigiTds->addC0Hit(strip);
                } 
                else 
                {
                    tkrDigiTds->addC1Hit(strip);
                }
            }

            TkrOverlayTdsCol->push_back(tkrDigiTds);
        }
    }

    // Return the pointer to it
    refpObject = TkrOverlayTdsCol;

    return status;
}
    
StatusCode TkrOverlayCnv::createRep(DataObject* pObject, IOpaqueAddress*&)
{
    StatusCode status = StatusCode::SUCCESS;

    if (!m_overlayOutputSvc) return StatusCode::FAILURE;

    // Retrieve the pointer to the digi
    EventOverlay* eventRoot = m_overlayOutputSvc->getRootEventOverlay();

    // Cast our object back to a TkrDigiCol object
    Event::TkrOverlayCol* TkrOverlayColTds = dynamic_cast<Event::TkrOverlayCol*>(pObject);

    // Loop over the collection and creat root versions of TkrDigi objects
    for (Event::TkrOverlayCol::const_iterator TkrOverlayTds  = TkrOverlayColTds->begin(); 
                                              TkrOverlayTds != TkrOverlayColTds->end(); 
                                              TkrOverlayTds++) 
    {
        idents::GlastAxis::axis axisTds = (*TkrOverlayTds)->getView();
        GlastAxis::axis axisRoot = (axisTds == idents::GlastAxis::X) ? GlastAxis::X : GlastAxis::Y;
        idents::TowerId idTds = (*TkrOverlayTds)->getTower();
        TowerId towerRoot(idTds.ix(), idTds.iy());
        Int_t totRoot[2] = {(*TkrOverlayTds)->getToT(0), (*TkrOverlayTds)->getToT(1)};
        Int_t lastController0Strip = (*TkrOverlayTds)->getLastController0Strip();
       
        TkrOverlay *tkrOverlayRoot = new TkrOverlay();

        tkrOverlayRoot->initialize((*TkrOverlayTds)->getBilayer(), axisRoot, towerRoot, totRoot);
        UInt_t numHits = (*TkrOverlayTds)->getNumHits();

        for (unsigned int iHit = 0; iHit < numHits; iHit++) 
        {
            Int_t strip = (*TkrOverlayTds)->getHit(iHit);
            if (strip <= lastController0Strip) 
            {
                tkrOverlayRoot->addC0Hit(strip);
            } else {
                tkrOverlayRoot->addC1Hit(strip);
            }
        }
        eventRoot->addTkrOverlay(tkrOverlayRoot);
    }

    return status;
}
