/** @file IDigiToOverlayTool.h

    @brief declaration of the IDigiToOverlayTool class

$Header: /nfs/slac/g/glast/ground/cvs/Overlay/src/IDigiToOverlayTool.h,v 1.0 2008/10/15 15:14:30 usher Exp $

*/

#ifndef IDigiToOverlayTool_h
#define IDigiToOverlayTool_h

#include "GaudiKernel/IAlgTool.h"

/** @class IBackgroundBintTool
    @brief Interface to tools to determine bin for overlay input
*/

static const InterfaceID IID_IDigiToOverlayTool("IDigiToOverlayTool", 1 , 0);

class IDigiToOverlayTool : virtual public IAlgTool
{
public:

    // Retrieve interface ID
    static const InterfaceID& interfaceID() { return IID_IDigiToOverlayTool; }

    ///! The current value of the quantity that we are selecting on
    virtual StatusCode translate() = 0;
};


#endif
