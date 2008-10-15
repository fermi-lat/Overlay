/** @file IBackgroundBinTool.h

    @brief declaration of the IBackgroundBinTool class

$Header: /nfs/slac/g/glast/ground/cvs/Interleave/src/IBackgroundBinTool.h,v 1.0 2007/11/09 19:06:19 usher Exp $

*/

#ifndef IBackgroundBinTool_h
#define IBackgroundBinTool_h

#include "GaudiKernel/IAlgTool.h"

/** @class IBackgroundBintTool
    @brief Interface to tools to determine bin for overlay input
*/

static const InterfaceID IID_IBackgroundBinTool("IBackgroundBinTool", 1 , 0);

class IBackgroundBinTool : virtual public IAlgTool
{
public:

    // Retrieve interface ID
    static const InterfaceID& interfaceID() { return IID_IBackgroundBinTool; }

    ///! The current value of the quantity that we are selecting on
    virtual double value()const = 0;
};


#endif
