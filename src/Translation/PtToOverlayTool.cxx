/**  @file PtToOverlayTool.cxx
    @brief implementation of class PtToOverlayTool
    
  $Header: /nfs/slac/g/glast/ground/cvs/Overlay/src/Translation/PtToOverlayTool.cxx,v 1.4 2011/12/12 20:54:56 heather Exp $  
*/

#include "IDigiToOverlayTool.h"

#include "GaudiKernel/ToolFactory.h"
#include "GaudiKernel/AlgTool.h"
#include "GaudiKernel/SmartDataPtr.h"
#include "GaudiKernel/GaudiException.h" 
#include "GaudiKernel/IDataProviderSvc.h"
#include "GaudiKernel/DataSvc.h"

#include "ntupleWriterSvc/INTupleWriterSvc.h"

#include "OverlayEvent/OverlayEventModel.h"
#include "OverlayEvent/PtOverlay.h"

/** @class BackgroundSelection
    @brief manage the selection of background events to merge with signal events
    @author Tracy Usher

It uses the name of a tuple variable as a key to look up trigger and downlink rates of a background
source depending on the value of that variable. See the corresponding member functions. Given such
a value, it expects to find a source of such events generated with that value which can be inserted 
into the Gleam output tuple.

*/
class PtToOverlayTool : public AlgTool, virtual public IDigiToOverlayTool
{
public:

    // Standard Gaudi Tool constructor
    PtToOverlayTool(const std::string& type, const std::string& name, const IInterface* parent);

    // After building it, destroy it
    ~PtToOverlayTool();

    /// @brief Intialization of the tool
    StatusCode initialize();

    /// @brief Finalize method for the tool
    StatusCode finalize();

    ///! The current value of the quantity that we are selecting on
    StatusCode translate();

private:

    /// Pointer to the event data service (aka "eventSvc")
    IDataProviderSvc* m_edSvc;

    /// Pointer to the Overlay data service
    DataSvc*          m_dataSvc;

    /// Access to the ntuple
    INTupleWriterSvc* m_tuple;

    StringProperty    m_treename;

    /// The Pt variables we need to copy from the ntuple to PtOverlay
    // Variables to be used to calculate the Pt variables and stored into the ntuple
    double*           m_start;
    float*            m_sc_position;
    float*            m_lat_geo;
    float*            m_lon_geo;
    float*            m_lat_mag;
    float*            m_rad_geo;
    float*            m_ra_scz;
    float*            m_dec_scz;
    float*            m_ra_scx; 
    float*            m_dec_scx;
    float*            m_zenith_scz;       ///< space craft zenith angle
    float*            m_B;                ///< magnetic field
    float*            m_L;                ///< McIllwain L parameter

    float*            m_lambda;
    float*            m_R;
    float*            m_bEast;
    float*            m_bNorth;
    float*            m_bUp;

	int*              m_lat_mode;
	int*              m_lat_config;
	int*              m_data_qual;
	float*            m_rock_angle;
	float*            m_livetime_frac;
};

//static ToolFactory<PtToOverlayTool> s_factory;
//const IToolFactory& PtToOverlayToolFactory = s_factory;
DECLARE_TOOL_FACTORY(PtToOverlayTool);

//------------------------------------------------------------------------
PtToOverlayTool::PtToOverlayTool(const std::string& type, 
                                 const std::string& name, 
                                 const IInterface* parent) :
                                 AlgTool(type, name, parent)
{
    //Declare the additional interface
    declareInterface<IDigiToOverlayTool>(this);

    declareProperty("TreeName", m_treename="MeritTuple");

    m_start       = 0;
    m_sc_position = 0;
    m_lat_geo     = 0;
    m_lon_geo     = 0;
    m_lat_mag     = 0;
    m_rad_geo     = 0;
    m_ra_scz      = 0;
    m_dec_scz     = 0;
    m_ra_scx      = 0; 
    m_dec_scx     = 0;
    m_zenith_scz  = 0;
    m_B           = 0;
    m_L           = 0;
    m_lambda      = 0;
    m_R           = 0;
    m_bEast       = 0;
    m_bNorth      = 0;
    m_bUp         = 0;

	m_lat_mode    = 0;
	m_lat_config  = 0;
	m_data_qual   = 0;
	m_rock_angle  = 0;
	m_livetime_frac = 0;

    return;
}
//------------------------------------------------------------------------
PtToOverlayTool::~PtToOverlayTool()
{
    return;
}

StatusCode PtToOverlayTool::initialize()
{
    MsgStream log(msgSvc(), name());

    // Set the properties
    setProperties();

    IService* iService = 0;
    StatusCode sc = serviceLocator()->service("EventDataSvc", iService, true);
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

    // Retrieve the ntuple service

    iService = 0;
    if( serviceLocator()->service("RootTupleSvc", iService, true).isFailure() ) 
    {
        log << MSG::ERROR << " failed to get the RootTupleSvc" << endreq;
        return StatusCode::FAILURE;
    }
    if (m_tuple = dynamic_cast<INTupleWriterSvc*>(iService))
    {
        void*       dummy = 0;
        std::string type  = "";

        // Recover pointers to the Pt variables
        try {type = m_tuple->getItem(m_treename, "PtTime", dummy);}
        catch(...) {type = "";}

        if (type == "Double_t") m_start = reinterpret_cast<double*>(dummy);
        else
        {
            m_start = new double;
            *m_start = 0;
        }

        try {type = m_tuple->getItem(m_treename, "PtPos", dummy);}
        catch(...) {type = "";}

        if (type == "Float_t") m_sc_position = reinterpret_cast<float*>(dummy);
        else
        {
            m_sc_position = new float[3];
            m_sc_position[0] = 0;
            m_sc_position[1] = 0;
            m_sc_position[2] = 0;
        }

        try {type = m_tuple->getItem(m_treename, "PtLat", dummy);}
        catch(...) {type = "";}

        if (type == "Float_t") m_lat_geo = reinterpret_cast<float*>(dummy);
        else
        {
            m_lat_geo  = new float;
            *m_lat_geo = 0;
        }

        try {type = m_tuple->getItem(m_treename, "PtLon", dummy);}
        catch(...) {type = "";}

        if (type == "Float_t") m_lon_geo = reinterpret_cast<float*>(dummy);
        else
        {
            m_lon_geo  = new float;
            *m_lon_geo = 0;
        }

        try {type = m_tuple->getItem(m_treename, "PtMagLat", dummy);}
        catch(...) {type = "";}

        if (type == "Float_t") m_lat_mag = reinterpret_cast<float*>(dummy);
        else
        {
            m_lat_mag  = new float;
            *m_lat_mag = 0;
        }

        try {type = m_tuple->getItem(m_treename, "PtAlt", dummy);}
        catch(...) {type = "";}

        if (type == "Float_t") m_rad_geo = reinterpret_cast<float*>(dummy);
        else
        {
            m_rad_geo  = new float;
            *m_rad_geo = 0;
        }

        try {type = m_tuple->getItem(m_treename, "PtRaz", dummy);}
        catch(...) {type = "";}

        if (type == "Float_t") m_ra_scz = reinterpret_cast<float*>(dummy);
        else
        {
            m_ra_scz  = new float;
            *m_ra_scz = 0;
        }

        try {type = m_tuple->getItem(m_treename, "PtDecz", dummy);}
        catch(...) {type = "";}

        if (type == "Float_t") m_dec_scz = reinterpret_cast<float*>(dummy);
        else
        {
            m_dec_scz  = new float;
            *m_dec_scz = 0;
        }

        try {type = m_tuple->getItem(m_treename, "PtRax", dummy);}
        catch(...) {type = "";}

        if (type == "Float_t") m_ra_scx = reinterpret_cast<float*>(dummy);
        else
        {
            m_ra_scx  = new float;
            *m_ra_scx = 0;
        }

        try {type = m_tuple->getItem(m_treename, "PtDecx", dummy);}
        catch(...) {type = "";}

        if (type == "Float_t") m_dec_scx = reinterpret_cast<float*>(dummy);
        else
        {
            m_dec_scx  = new float;
            *m_dec_scx = 0;
        }

        try {type = m_tuple->getItem(m_treename, "PtSCzenith", dummy);}
        catch(...) {type = "";}

        if (type == "Float_t") m_zenith_scz = reinterpret_cast<float*>(dummy);
        else
        {
            m_zenith_scz  = new float;
            *m_zenith_scz = 0;
        }

        try {type = m_tuple->getItem(m_treename, "PtMcIlwainB", dummy);}
        catch(...) {type = "";}

        if (type == "Float_t") m_B = reinterpret_cast<float*>(dummy);
        else
        {
            m_B  = new float;
            *m_B = 0;
        }

        try {type = m_tuple->getItem(m_treename, "PtMcIlwainL", dummy);}
        catch(...) {type = "";}

        if (type == "Float_t") m_L = reinterpret_cast<float*>(dummy);
        else
        {
            m_L  = new float;
            *m_L = 0;
        }

        try {type = m_tuple->getItem(m_treename, "PtLambda", dummy);}
        catch(...) {type = "";}

        if (type == "Float_t") m_lambda = reinterpret_cast<float*>(dummy);
        else
        {
            m_lambda  = new float;
            *m_lambda = 0;
        }

        try {type = m_tuple->getItem(m_treename, "PtR", dummy);}
        catch(...) {type = "";}

        if (type == "Float_t") m_R = reinterpret_cast<float*>(dummy);
        else
        {
            m_R  = new float;
            *m_R = 0;
        }

        try {type = m_tuple->getItem(m_treename, "PtBEast", dummy);}
        catch(...) {type = "";}

        if (type == "Float_t") m_bEast = reinterpret_cast<float*>(dummy);
        else
        {
            m_bEast  = new float;
            *m_bEast = 0;
        }

        try {type = m_tuple->getItem(m_treename, "PtBNorth", dummy);}
        catch(...) {type = "";}

        if (type == "Float_t") m_bNorth = reinterpret_cast<float*>(dummy);
        else
        {
            m_bNorth  = new float;
            *m_bNorth = 0;
        }

        try {type = m_tuple->getItem(m_treename, "PtBUp", dummy);}
        catch(...) {type = "";}

        if (type == "Float_t") m_bUp = reinterpret_cast<float*>(dummy);
        else
        {
            m_bUp  = new float;
            *m_bUp = 0;
        }

		try {type = m_tuple->getItem(m_treename, "PtLATMode", dummy);}
        catch(...) {type = "";}

        if (type == "Int_t") m_lat_mode = reinterpret_cast<int*>(dummy);
        else
        {

            m_lat_mode  = new int; 
            *m_lat_mode = 0;
        }

		try {type = m_tuple->getItem(m_treename, "PtLATConfig", dummy);}
        catch(...) {type = "";}

        if (type == "Int_t") m_lat_config = reinterpret_cast<int*>(dummy);
        else
        {
            m_lat_config  = new int;
            *m_lat_config = 0;
        }

		try {type = m_tuple->getItem(m_treename, "PtDataQual", dummy);}
        catch(...) {type = "";}

        if (type == "Int_t") m_data_qual = reinterpret_cast<int*>(dummy);
        else
        {
            m_data_qual  = new int;
            *m_data_qual = 0;
        }

		try {type = m_tuple->getItem(m_treename, "PtRockAngle", dummy);}
        catch(...) {type = "";}

        if (type == "Float_t") m_rock_angle = reinterpret_cast<float*>(dummy);
        else
        {
            m_rock_angle  = new float;
            *m_rock_angle = 0;
        }

		try {type = m_tuple->getItem(m_treename, "PtLivetimeFrac", dummy);}
        catch(...) {type = "";}

        if (type == "Float_t") m_livetime_frac = reinterpret_cast<float*>(dummy);
        else
        {
            m_livetime_frac  = new float;
            *m_livetime_frac = 0;
        }	
	}	
	else
    {
        log << MSG::ERROR << " failed to recast pointer to tuple service" << std::endl;
        return StatusCode::FAILURE;
    }

    return StatusCode::SUCCESS;
}

StatusCode PtToOverlayTool::finalize ()
{
    StatusCode  status = StatusCode::SUCCESS;
    
    return status;
}

//------------------------------------------------------------------------
StatusCode PtToOverlayTool::translate()
{
    MsgStream log(msgSvc(), name());
    StatusCode status = StatusCode::SUCCESS;

    // Create a collection of AcdOverlays and register in the TDS
    SmartDataPtr<Event::PtOverlay> ptOverlay(m_dataSvc, m_dataSvc->rootName() + OverlayEventModel::Overlay::PtOverlay);
    if(!ptOverlay)
    {
        ptOverlay = new Event::PtOverlay();

        status = m_dataSvc->registerObject(m_dataSvc->rootName() + OverlayEventModel::Overlay::PtOverlay, ptOverlay);
        if( status.isFailure() ) 
        {
            log << MSG::ERROR << "could not register OverlayEventModel::Overlay::PtOverlay" << endreq;
            return status;
        }
    }

    ptOverlay->initPtOverlay(*m_start,
                              m_sc_position,
                             *m_lat_geo,
                             *m_lon_geo,
                             *m_lat_mag,
                             *m_rad_geo,
                             *m_ra_scz,
                             *m_dec_scz,
                             *m_ra_scx, 
                             *m_dec_scx,
                             *m_zenith_scz,
                             *m_B, 
                             *m_L,
                             *m_lambda,
                             *m_R,
                             *m_bEast,
                             *m_bNorth,
                             *m_bUp,
							 *m_lat_mode,
							 *m_lat_config,
							 *m_data_qual,
							 *m_rock_angle,
							 *m_livetime_frac
                            );

    return status;
}
