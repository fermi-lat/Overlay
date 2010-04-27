// $Header: /nfs/slac/g/glast/ground/cvs/Overlay/src/cnv/DiagDataOverlayCnv.cxx,v 1. 2008/12/02 15:27:17 usher Exp $
/**
            @file  DiagDataOverlayCnv.cxx

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
#include "OverlayEvent/DiagDataOverlay.h"

#include "overlayRootData/EventOverlay.h"

class  DiagDataOverlayCnv : virtual public IGlastCnv, public Converter 
{
public:

    /**
        Constructor for this converter
        @param svc a ISvcLocator interface to find services
        @param clid the type of object the converter is able to convert
    */
    DiagDataOverlayCnv(ISvcLocator* svc);

    virtual ~DiagDataOverlayCnv();

    /// Query interfaces of Interface
    //virtual StatusCode queryInterface(const InterfaceID& riid, void** ppvInterface);
    static const CLID&         classID()     {return Event::DiagDataOverlay::classID();}
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


 static CnvFactory<DiagDataOverlayCnv> s_factory;
 const ICnvFactory& DiagDataOverlayCnvFactory = s_factory;

 DiagDataOverlayCnv::DiagDataOverlayCnv( ISvcLocator* svc) : Converter (SICB_StorageType, Event::DiagDataOverlay::classID(), svc) 
{
    m_path = OverlayEventModel::Overlay::DiagDataOverlay;

    return;
}

DiagDataOverlayCnv::~DiagDataOverlayCnv() 
{
    return;
}

StatusCode DiagDataOverlayCnv::initialize() 
{
    MsgStream log(msgSvc(), "DiagDataOverlayCnv");
    StatusCode status = Converter::initialize();

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

StatusCode DiagDataOverlayCnv::finalize() 
{
    return Converter::finalize();
}

// (To TDS) Conversion stuff
StatusCode DiagDataOverlayCnv::createObj(IOpaqueAddress*, DataObject*& refpObject) 
{
    StatusCode status = StatusCode::SUCCESS;

    // If no service then we are not inputting from PDS
    if (!m_overlayInputSvc) return StatusCode::FAILURE;

    // Retrieve the pointer to the digi
    EventOverlay* overlayRoot = m_overlayInputSvc->getRootEventOverlay();

    // Extract the Diagnostic Data from our input EventOverlay object
    const DiagDataOverlay& diagDataRoot = overlayRoot->getDiagDataOverlay();

    // Create the new DiagDataOverlay object to put in the TDS
    Event::DiagDataOverlay* diagDataTds = new Event::DiagDataOverlay();

    // Copy over Cal diagnostic info first
    int numCalDiags = diagDataRoot.getNumCalDiagnostic();

    for(int idx = 0; idx < numCalDiags; idx++)
    {
        const CalDiagDataOverlay& calDiagDataRoot = diagDataRoot.getCalDiagnosticByIndex(idx);

        Event::CalDiagDataOverlay cal(calDiagDataRoot.dataWord(), calDiagDataRoot.tower(), calDiagDataRoot.layer());

        diagDataTds->addCalDiagnostic(cal);
    }

    // Now come back and copy the Tkr diagnostic info
    int numTkrDiags = diagDataRoot.getNumTkrDiagnostic();

    for(int idx = 0; idx < numTkrDiags; idx++)
    {
        const TkrDiagDataOverlay& tkrDiagDataRoot = diagDataRoot.getTkrDiagnosticByIndex(idx);

        Event::TkrDiagDataOverlay tkr(tkrDiagDataRoot.dataWord(), tkrDiagDataRoot.tower(), tkrDiagDataRoot.gtcc());

        diagDataTds->addTkrDiagnostic(tkr);
    }

    // Return the pointer to it
    refpObject = diagDataTds;

    return status;
}
    
StatusCode DiagDataOverlayCnv::createRep(DataObject* pObject, IOpaqueAddress*&)
{
    StatusCode status = StatusCode::SUCCESS;

    if (!m_overlayOutputSvc) return status;

    // Retrieve the pointer to the digi
    EventOverlay* overlayRoot = m_overlayOutputSvc->getRootEventOverlay();

    // Cast our object back to a DiagDataOverlay TDS object
    Event::DiagDataOverlay* diagDataOverlayTds = dynamic_cast<Event::DiagDataOverlay*>(pObject);

    // Local object to fill
    DiagDataOverlay diagDataOverlayRoot;

    // Copy over Cal diagnostic info first
    int numCalDiags = diagDataOverlayTds->getNumCalDiagnostic();

    for(int idx = 0; idx < numCalDiags; idx++)
    {
        const Event::CalDiagDataOverlay& calDiagDataTds = diagDataOverlayTds->getCalDiagnosticByIndex(idx);

        CalDiagDataOverlay cal(calDiagDataTds.dataWord(), calDiagDataTds.tower(), calDiagDataTds.layer());

        diagDataOverlayRoot.addCalDiagnostic(cal);
    }

    // Now come back and copy the Tkr diagnostic info
    int numTkrDiags = diagDataOverlayTds->getNumTkrDiagnostic();

    for(int idx = 0; idx < numTkrDiags; idx++)
    {
        const Event::TkrDiagDataOverlay& tkrDiagDataTds = diagDataOverlayTds->getTkrDiagnosticByIndex(idx);

        TkrDiagDataOverlay tkr(tkrDiagDataTds.dataWord(), tkrDiagDataTds.tower(), tkrDiagDataTds.gtcc());

        diagDataOverlayRoot.addTkrDiagnostic(tkr);
    }

    // Save the info 
    overlayRoot->setDiagDataOverlay(diagDataOverlayRoot);

    return status;
}
