/** @file IBbgndTupleSelectTool.h

    @brief declaration of the IBkgndSelectTool class

$Header: /nfs/slac/g/glast/ground/cvs/Interleave/src/IBackgroundSelectTool.h,v 1.1 2007/11/09 19:06:19 usher Exp $

*/

#ifndef IBackgroundSelectTool_h
#define IBackgroundSelectTool_h

#include "GaudiKernel/IAlgTool.h"

#include <string>
#include <vector>

class DigiEvent;


/** @class IBkgndSelectTool
    @brief manage the selection of background events to merge with signal events
    @author Dan Flath

It uses the name of a tuple variable as a key to look up trigger and downlink rates of a background
source depending on the value of that variable. See the corresponding member functions. Given such
a value, it expects to find a source of such events generated with that value which can be inserted 
into the Gleam output tuple.

*/

static const InterfaceID IID_IBackgroundSelectTool("IBackgroundSelectTool", 1 , 0);

class IBackgroundSelectTool : virtual public IAlgTool
{
public:

    // Retrieve interface ID
    static const InterfaceID& interfaceID() { return IID_IBackgroundSelectTool; }

    /** @brief select an event and copy the contents to the output tree
    */
    virtual DigiEvent* selectEvent() = 0;
};


#endif
