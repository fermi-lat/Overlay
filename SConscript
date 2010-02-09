# -*- python -*-
# $Header: /nfs/slac/g/glast/ground/cvs/Overlay/SConscript,v 1.3 2010/01/27 22:47:15 usher Exp $
# Authors: Tracy Usher <usher@slac.stanford.edu>
# Version: Overlay-01-06-01
import os
Import('baseEnv')
Import('listFiles')
Import('packages')
progEnv = baseEnv.Clone()
libEnv = baseEnv.Clone()

libEnv.Tool('OverlayLib', depsOnly = 1)
OverlayLib = libEnv.SharedLibrary('Overlay',
                                   listFiles(['src/Dll/*.cxx',
                                              'src/cnv/*.cxx',
                                              'src/DataServices/*.cxx',
                                              'src/MergeAlgs/*.cxx',
                                              'src/InputControl/*.cxx',
                                              'src/Translation/*.cxx',
                                              'src/Tasks/*.cxx']))

progEnv.Tool('OverlayLib')
test_Overlay = progEnv.GaudiProgram('test_Overlay',
                                     listFiles(['src/test/*.cxx']), test = 1)

progEnv.Tool('registerTargets', package = 'Overlay',
             libraryCxts = [[OverlayLib, libEnv]],
             testAppCxts = [[test_Overlay, progEnv]],
             includes = listFiles(['Overlay/*']))




