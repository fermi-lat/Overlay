# -*- python -*-
# $Header: /nfs/slac/g/glast/ground/cvs/Overlay/SConscript,v 1.35 2012/07/24 20:33:44 jrb Exp $
# Authors: Tracy Usher <usher@slac.stanford.edu>
# Version: Overlay-02-03-00
import os
Import('baseEnv')
Import('listFiles')
Import('packages')
progEnv = baseEnv.Clone()
libEnv = baseEnv.Clone()

libEnv.Tool('addLinkDeps', package='Overlay', toBuild='component')

OverlayLib = libEnv.ComponentLibrary('Overlay',
                              listFiles(['src/cnv/EventOverlayCnv.cxx',
                                         'src/cnv/AcdOverlayCnv.cxx',
                                         'src/cnv/CalOverlayiCnv.cxx',
                                         'src/cnv/DiagDataOverlayCnv.cxx',
                                         'src/cnv/GemOverlayCnv.cxx', 
                                         'src/cnv/PtOverlayCnv.cxx', 
                                         'src/cnv/TkrOverlayCnv.cxx',
                                         'src/cnv/SrcOverlayCnv.cxx',
                                         #'src/cnv/*.cxx',
                                         'src/DataServices/*.cxx',
                                         'src/MergeAlgs/*.cxx',
                                         'src/InputControl/*.cxx',
                                         'src/Translation/*.cxx',
                                         'src/Tasks/*.cxx'
                                         ]))
 
progEnv.Tool('OverlayLib', depsOnly = 1)
test_Overlay = progEnv.GaudiProgram('test_Overlay',
                                    listFiles(['src/test/*.cxx']), test = 1,
                                    package='Overlay')

progEnv.Tool('registerTargets', package = 'Overlay',
             libraryCxts = [[OverlayLib, libEnv]],
             testAppCxts = [[test_Overlay, progEnv]],
             includes = listFiles(['Overlay/*']),
             xml = listFiles(['xml/*.xml', 'xml/*.xsd', 'xml/test/*.xml']),
             data = listFiles(['data/test/overlay.root']),
             jo = listFiles(['src/*/*.txt']))




