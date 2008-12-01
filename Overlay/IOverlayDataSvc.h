/** @file IOverlayDataSvc.h

    @brief declaration of an interface to the OverlayDataSvc service

$Header: /nfs/slac/g/glast/ground/cvs/Overlay/src/IOverlayDataSvc.h,v 1.0 2008/10/15 15:14:30 usher Exp $

*/

#ifndef IOverlayDataSvc_h
#define IOverlayDataSvc_h

#include "GaudiKernel/IService.h"


/** @class  IOverlayDataSvc
    @brief  Interface to the OverlayDataSvc service
    @author Tracy Usher

Aims to provide the abstract interface to the Overlay Data service 

*/

class EventOverlay;

static const InterfaceID IID_IOverlayDataSvc("IOverlayDataSvc", 1 , 0);

class IOverlayDataSvc : virtual public IService
{
public:

    // Retrieve interface ID
    static const InterfaceID& interfaceID() { return IID_IOverlayDataSvc; }

    /** @brief Get pointer to a Root DigiEvent object
    */
    virtual EventOverlay* getRootEventOverlay() = 0;

    /** @brief select an event and copy the contents to the output tree
    */
    virtual StatusCode selectNextEvent() = 0;

    /** @brief Register a path with an output data service
    */
    virtual StatusCode registerOutputPath(const std::string& path) = 0;
};


#endif
