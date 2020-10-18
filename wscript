
import os, re, sys
from subprocess import call

def options(opt):
    opt.load ('compiler_c')
    opt.add_option ('--debug', default=False, action="store_true", dest="debug", \
        help="Compile debuggable binaries [ Default: False ]")

def configure (conf):
    conf.load ('compiler_c')
    if conf.options.debug:
        conf.define ("DEBUG", 1)
        conf.define ("_DEBUG", 1)
        conf.env.append_unique ('CXXFLAGS', ['-g', '-ggdb', '-O0'])
        conf.env.append_unique ('CFLAGS', ['-g', '-ggdb', '-O0'])
    else:
        conf.define ("NDEBUG", 1)
        conf.env.append_unique ('CXXFLAGS', ['-Os'])
        conf.env.append_unique ('CFLAGS', ['-Os'])
    conf.check(header_name='lua.h', uselib_store='LUA', mandatory=True)
    conf.check(lib='lua', uselib_store='LUA_LIB', mandatory=True)

def module_env (bld):
    env = bld.env.derive()
    if sys.platform == 'windows':
        env.cshlib_PATTERN = env.cxxshlib_PATTERN = '%s.dll'
    else:
        env.cshlib_PATTERN = env.cxxshlib_PATTERN = '%s.so'
        env.macbundle_PATTERN = '%s.so'
    return env

def build_module (bld, name, source):
    mod = bld (
        features    = 'c cshlib',
        source      = source,
        includes    = [ '.', 'src' ],
        name        = name,
        target      = 'modules/%s' % name,
        env         = module_env (bld),
        use         = [ 'LUA' ],
        cflags      = [ ],
        linkflags   = [],
        install_path = '%s/lib/lua/5.3' % bld.env.PREFIX
    )
    if sys.platform == 'linux':
        mod.linkflags.append('-fPIC')
        mod.linkflags.append('-fvisibility=hidden')
        mod.cflags.append('-fvisibility=hidden')
    elif sys.platform == 'windows':
        pass
    elif sys.platform == 'darwin':
        mod.linkflags.append('-fPIC')
        mod.linkflags.append('-fvisibility=hidden')
        mod.cflags.append('-fvisibility=hidden')
        mod.mac_bundle = True

    return mod

def build (bld):
    build_module (bld, 'kv', '''
        src/audio.c
        src/midi.c
        src/vector.c
        src/kvmod.c'''
    .split())
    
    tests = bld.program (
        source      = [ 'tests/test.c' ],
        includes    = [ '.', 'src' ],
        name        = 'test',
        target      = 'test',
        use         = [ 'LUA', 'LUA_LIB' ],
        linkflags   = []
    )

    if 'linux' in sys.platform:
        tests.linkflags.append ('-Wl,--no-as-needed')
        tests.linkflags.append ('-lm')
        tests.linkflags.append ('-ldl')

def check (ctx):
    if not os.path.exists('build/test'):
        ctx.fatal ("Binary tests not compiled")
        return
    if 0 != call (["build/test"]):
        ctx.fatal ("Tests failed")
    if not os.path.exists('build/test'):
        ctx.fatal ("Binary tests not compiled")
        return
    if 0 != call (["tests/testmods"]):
        ctx.fatal ("Tests failed")
