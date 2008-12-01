/**  @file McIlwain_L_Tool.cxx
    @brief implementation of class McIlwain_L_Tool
    
  $Header: /nfs/slac/g/glast/ground/cvs/Overlay/src/McIlwain_L_Tool.cxx,v 1.1.1.1 2008/10/15 15:14:30 usher Exp $  
*/

#include "Overlay/IBackgroundBinTool.h"

#include "GaudiKernel/ToolFactory.h"
#include "GaudiKernel/AlgTool.h"
#include "GaudiKernel/SmartDataPtr.h"
#include "GaudiKernel/GaudiException.h" 
#include "GaudiKernel/IDataProviderSvc.h"

#include "Event/TopLevel/Event.h"
#include "Event/TopLevel/EventModel.h"

#include "astro/GPS.h"
#include "astro/EarthCoordinate.h"

/** @class BackgroundSelection
    @brief manage the selection of background events to merge with signal events
    @author Dan Flath

It uses the name of a tuple variable as a key to look up trigger and downlink rates of a background
source depending on the value of that variable. See the corresponding member functions. Given such
a value, it expects to find a source of such events generated with that value which can be inserted 
into the Gleam output tuple.

*/
class McIlwain_L_Tool : public AlgTool, virtual public IBackgroundBinTool
{
public:

    // Standard Gaudi Tool constructor
    McIlwain_L_Tool(const std::string& type, const std::string& name, const IInterface* parent);

    // After building it, destroy it
    ~McIlwain_L_Tool();

    /// @brief Intialization of the tool
    StatusCode initialize();

    /// @brief Finalize method for the tool
    StatusCode finalize();

    ///! The current value of the quantity that we are selecting on
    double value()const;

private:

    /// Pointer to the event data service (aka "eventSvc")
    IDataProviderSvc*   m_edSvc;

    /// Option string which will be passed to McEvent::Clear
    StringProperty       m_clearOption;
};

static ToolFactory<McIlwain_L_Tool> s_factory;
const IToolFactory& McIlwain_L_ToolFactory = s_factory;

//------------------------------------------------------------------------
McIlwain_L_Tool::McIlwain_L_Tool(const std::string& type, 
                                 const std::string& name, 
                                 const IInterface* parent) :
                                 AlgTool(type, name, parent)
{
    //Declare the additional interface
    declareInterface<IBackgroundBinTool>(this);

    // declare properties with setProperties calls
    declareProperty("clearOption",  m_clearOption="");
}
//------------------------------------------------------------------------
McIlwain_L_Tool::~McIlwain_L_Tool()
{
    return;
}

StatusCode McIlwain_L_Tool::initialize()
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

    return sc;
}

StatusCode McIlwain_L_Tool::finalize ()
{
    StatusCode  status = StatusCode::SUCCESS;
    
    return status;
}

//------------------------------------------------------------------------
double McIlwain_L_Tool::value()const
{
    // Retrieve the Event data for this event
    SmartDataPtr<Event::EventHeader> evt(m_edSvc, EventModel::EventHeader);

    // Set the time
    astro::GPS::instance()->time(evt->time());

    // Earth coordinates 
    const astro::EarthCoordinate& earth(astro::GPS::instance()->earthpos());
    
    float L = earth.L();

    double x = L;

    return x;
}
