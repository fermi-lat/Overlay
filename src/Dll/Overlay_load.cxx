/** 
* @file RootIo_load.cpp
* @brief This is needed for forcing the linker to load all components
* of the library.
*
*  $Header: /nfs/slac/g/glast/ground/cvs/Overlay/src/Dll/Overlay_load.cxx,v 1.7 2010/04/27 16:53:33 usher Exp $
*/

#include "GaudiKernel/DeclareFactoryEntries.h"

DECLARE_FACTORY_ENTRIES(Overlay) {
    DECLARE_ALGORITHM( DoMergeAlg );
    DECLARE_ALGORITHM( AcdOverlayMergeAlg );
    DECLARE_ALGORITHM( CalOverlayMergeAlg );
    DECLARE_ALGORITHM( TkrOverlayMergeAlg );
    DECLARE_ALGORITHM( GemOverlayMergeAlg );
    DECLARE_ALGORITHM( DiagDataOverlayMergeAlg );
    DECLARE_ALGORITHM( DigiToOverlayAlg );
    DECLARE_ALGORITHM( SkimOverlayEventsAlg );
    DECLARE_SERVICE( OverlayCnvSvc );
    DECLARE_SERVICE( OverlayDataSvc );
    DECLARE_SERVICE( OverlayInputSvc );
    DECLARE_SERVICE( OverlayOutputSvc );
    DECLARE_TOOL( McIlwain_L_Tool );
    DECLARE_TOOL( EventToOverlayTool );
    DECLARE_TOOL( CalXtalToOverlayTool );
    DECLARE_TOOL( TkrDigiToOverlayTool );
    DECLARE_TOOL( AcdHitToOverlayTool );
    DECLARE_TOOL( GemToOverlayTool );
    DECLARE_TOOL( DiagnosticDataToOverlayTool );
    DECLARE_TOOL( PtToOverlayTool );
    DECLARE_TOOL( OverlayRandom);
    DECLARE_CONVERTER( SrcOverlayCnv );
    DECLARE_CONVERTER( EventOverlayCnv );
    DECLARE_CONVERTER( TkrOverlayCnv );
    DECLARE_CONVERTER( CalOverlayCnv );
    DECLARE_CONVERTER( AcdOverlayCnv );
    DECLARE_CONVERTER( GemOverlayCnv );
    DECLARE_CONVERTER( DiagDataOverlayCnv );
    DECLARE_CONVERTER( PtOverlayCnv );
//    DECLARE_CONVERTER( LdfGemCnv );
}
  
