#$Header: $
def generate(env, **kw):
    if not kw.get('depsOnly', 0):
        env.Tool('addLibrary', library=['Overlay'])
    env.Tool('TriggerLib')
    env.Tool('EventLib')
    env.Tool('LdfEventLib')
    env.Tool('OverlayEventLib')
    env.Tool('RootIoLib')
    env.Tool('overlayRootDataLib')
    env.Tool('TkrUtilLib')
    env.Tool('AcdUtilLib')
    env.Tool('GlastSvcLib')
    env.Tool('astroLib')
    env.Tool('xmlBaseLib')
    env.Tool('facilitiesLib')
    env.Tool('addLibrary', library=env['gaudiLibs'])

def exists(env):
    return 1;
