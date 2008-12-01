// Mainpage for doxygen

/** @mainpage package Overlay

 @author Tracy Usher
 
 @section description Description

  The Overlay package is the top level package, of three, used to handle 
  the overlaying of background data (from Periodic Triggers) on top of simulated
  events. The goal is to provide a more realistic estimate of the background
  environment that the event reconstruction must work in. 

  The general scheme is to start with background data files in standard Glast 
  digi format and run a job which reads these in, performing enough processing to
  produce calibrated data which are then converted to "overlay" format and written
  back out to data files. The data files are organized according to some desired 
  formaat (e.g. organized in bins of McIlwain_L). During simulation/reconstruction
  jobs these files are then accessed, according to their organization, overlay 
  data read in and then merged into the simulated output before going to the full
  reconstruction. 

  This package covers all bases for Overlay - a section is aimed at translating
  from Event package TDS classes to OverlayEvent TDS classes, a section containing
  converters to handle input/output of OverlayEvent TDS classes, including input 
  annd output control services, a section which contains the input file control for
  the actual overlays, and, finally, a section containing code to merge the 
  OverlayEvent TDS information with the simulated event information. 

 @section algorithms Algorithms

  There are currently three merging algorithms:
  AcdOverlayMergeAlg - responsible for converting AcdOverlay data into McPositionHits
  which are added to the McPositionHit collection in the TDS (before ACD digitization)
  CalOverlayMergeAlg - responsible for converting and/or merging CalOverlay data
  into McIntegratingHits (before CalXtalRecAlg runs)
  TkrOverlayMergeAlg - responsible merging TkrOverlay data with TkrDigis (after 
  TkrDigiAlg runs)
  These algorithms are including in the Job Options file for merging 
  (src/MergeAlgs/Overlay.txt), which should be included in any simulation job which 
  runs from the sim/gen step and produces at least digis as output. See the sub-
  package src/MergeAlgs

  There is one algorithm, DigiToOverlayAlg, for overally control of the process of 
  translating the "raw" overlay data from input digi format (or a close relative) 
  to the Overlay output. The actual conversion of the needed objects is performed in 
  Tools specific to each task. See the subpackage src/Translation.

 @section services Services
  
  OverlayInputSvc controls the input of Overlay data when an object is requested. It
  interacts with the class XmlFetchEvents to handle any file selection that needs to 
  be done, and with the background specific tool (e.g. McIlwain_L_Tool) for selecting 
  the input file bin. 

  OverlayOutputSvc controls the output of Overlay data in jobs which are run to 
  convert the background data files into Overlay format. 

 @section tools Tools
  
  Translation tools:
  AcdHitToOverlayTool
  CalXtalToOverlayTool
  EventToOverlayTool
  GemToOverlayTool
  TkrDigiToOverlayTool

  These tools are accessed through the IDigiToOverlayTool interface

  Bin selection Tools
  McIlwain_L_Tool selects an input file bin by the McIlwain_L parameter

  These tool are accessed through the IBackgroundBinTool

 @section converters Converters
  
  AcdOverlayCnv
  CalOverlayCnv
  EventOverlayCnv
  GemOverlayCnv
  SrcOverlayCnv
  TkrOverlayCnv

  These are accessed through the IEventCnvSvc interface

  <hr>
 @section notes Release Notes
  release.notes
 <hr>
 @section Tests Tests
 There are no test routines in this package:
 <hr>
 @section requirements requirements
 @verbinclude requirements
 <hr>

 */
