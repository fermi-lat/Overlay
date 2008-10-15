/** 
* @file RootIo_load.cpp
* @brief This is needed for forcing the linker to load all components
* of the library.
*
*  $Header: /nfs/slac/g/glast/ground/cvs/RootIo/src/Dll/RootIo_load.cxx,v 1.16 2008/04/04 18:34:45 heather Exp $
*/

#include "GaudiKernel/DeclareFactoryEntries.h"

DECLARE_FACTORY_ENTRIES(Overlay) {
    DECLARE_ALGORITHM( overlayRootReaderAlg );
    DECLARE_TOOL( BackgroundSelectTool );
    DECLARE_TOOL( McIlwain_L_Tool );
}
  
