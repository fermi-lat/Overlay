#$Header: /nfs/slac/g/glast/ground/cvs/GlastRelease-scons/Overlay/OverlayLib.py,v 1.2 2012/01/14 01:38:13 jrb Exp $
def generate(env, **kw):
    if not kw.get('depsOnly', 0):
        env.Tool('addLibrary', library=['Overlay'])
        if env['PLATFORM']=='win32' and env.get('CONTAINERNAME','')=='GlastRelease':
	    env.Tool('findPkgPath', package = 'Overlay') 

    env.Tool('RootConvertLib')
    env.Tool('rootUtilLib')
    env.Tool('OverlayEventLib')
    env.Tool('overlayRootDataLib')
    env.Tool('astroLib')
    ##env.Tool('configDataLib')   # doesn't help
    env.Tool('xmlUtilLib')
    env.Tool('AcdUtilLib')
    env.Tool('CalUtilLib')
    ###env.Tool('addLibrary', library = env['geant4Libs']) # neither does this

    env.Tool('addLibrary', library=env['gaudiLibs'])
    #env.Tool('addLibrary', library=env['obfLibs'])
    if env['PLATFORM']=='win32' and env.get('CONTAINERNAME','')=='GlastRelease':
        env.Tool('findPkgPath', package = 'TkrUtil') 
        env.Tool('findPkgPath', package = 'enums') 
        env.Tool('findPkgPath', package = 'ntupleWriterSvc') 
        env.Tool('findPkgPath', package = 'RootIo') 

def exists(env):
    return 1;
