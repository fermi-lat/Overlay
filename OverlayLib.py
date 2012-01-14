#$Header: /nfs/slac/g/glast/ground/cvs/GlastRelease-scons/Overlay/OverlayLib.py,v 1.1 2009/12/08 06:39:20 jrb Exp $
def generate(env, **kw):
    if not kw.get('depsOnly', 0):
        env.Tool('addLibrary', library=['Overlay'])

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

def exists(env):
    return 1;
