
import os, re, sys
from subprocess import call

APPNAME = 'lua-kv'
VERSION = '0.1.0'

def options(opt):
    opt.load ('compiler_c compiler_cxx')
    opt.add_option ('--debug', default=False, action="store_true", dest="debug", \
        help="Compile debuggable binaries [ Default: False ]")
    opt.add_option ('--test', default=False, action='store_true', dest='test', \
        help="Build the test suite [ Default: False ]")

def configure (conf):
    conf.load ('compiler_c compiler_cxx')

    if conf.options.debug:
        conf.define ("DEBUG", 1)
        conf.define ("_DEBUG", 1)
        conf.env.append_unique ('CXXFLAGS', ['-g', '-ggdb', '-O0'])
        conf.env.append_unique ('CFLAGS', ['-g', '-ggdb', '-O0'])
    else:
        conf.define ("NDEBUG", 1)
        conf.env.append_unique ('CXXFLAGS', ['-Os'])
        conf.env.append_unique ('CFLAGS', ['-Os'])
    
    conf.check (header_name='lua.h', uselib_store='LUA', mandatory=True)
    conf.check (lib='lua', uselib_store='LUA_LIB', mandatory=True)
    conf.check (header_name="sol/forward.hpp", uselib_store="SOL", mandatory=False)

    # conf.check_cfg (package='juce_debug-6' if conf.options.debug else 'juce-6', 
    #                 uselib_store='JUCE', args=['--libs', '--cflags'], mandatory=False)
    
    conf.env.append_unique ('CXXFLAGS', ['-std=c++17'])

    conf.env.LUA_VERSION = '5.4'
    conf.env.TEST = bool (conf.options.test)
    conf.env.SOL  = bool (conf.env.HAVE_SOL)
    conf.env.JUCE = bool (conf.env.HAVE_JUCE) and bool (conf.env.HAVE_SOL)

    if conf.env.JUCE:
        conf.define ('LKV_JUCE_HEADER', 'juce/juce.h')

    if conf.env.SOL:
        ## Sol3 Safety options
        # https://sol2.readthedocs.io/en/latest/safety.html
        conf.define ('SOL_SAFE_USERTYPE', 1)

def module_env (bld):
    env = bld.env.derive()
    if sys.platform == 'windows':
        env.cshlib_PATTERN = env.cxxshlib_PATTERN = '%s.dll'
    else:
        env.cshlib_PATTERN = env.cxxshlib_PATTERN = '%s.so'
        env.macbundle_PATTERN = '%s.so'
    return env

def setup_module (mod):
    if 'linux' in sys.platform or 'darwin' in sys.platform:
        mod.env.append_unique ('LINKFLAGS', [ '-fvisibility=hidden', '-fPIC'])
        mod.env.append_unique ('CFLAGS',    [ '-fvisibility=hidden', '-fPIC'])
        mod.env.append_unique ('CXXFLAGS',  [ '-fvisibility=hidden', '-fPIC'])
    elif sys.platform == 'windows':
        pass

    if 'darwin' in sys.platform:
        mod.mac_bundle = True

    return mod

def build_classmod (bld, name):
    mod = bld (
        features    = 'cxx cxxshlib',
        source      =  [ 'src/kv/%s.cpp' % name],
        includes    = [ '.', 'src' ],
        name        = name,
        target      = 'lib/lua/kv/%s' % name,
        env         = module_env (bld),
        use         = [ 'LUA' ],
        cflags      = [],
        cxxflags    = [],
        linkflags   = [],
        install_path = '%s/lib/lua/%s/kv' % (bld.env.PREFIX, bld.env.LUA_VERSION)
    )
    return setup_module (mod)

def build_cmodule (bld, name):
    mod = bld (
        features    = 'c cshlib',
        source      = [ 'src/kv/%s.c' % name ],
        includes    = [ '.', 'src' ],
        name        = name,
        target      = 'lib/lua/kv/%s' % name,
        env         = module_env (bld),
        use         = [ 'LUA' ],
        cflags      = [],
        cxxflags    = [],
        linkflags   = [],
        install_path = '%s/lib/lua/%s/kv' % (bld.env.PREFIX, bld.env.LUA_VERSION)
    )
    return setup_module (mod)

def build_lib (bld):
    lib = bld (
        features    = 'cxx cxxshlib',
        source      =  bld.path.ant_glob ('src/*.c') +
                       bld.path.ant_glob ('src/*.cpp'),
        includes    = [ '.', 'src' ],
        target      = 'lib/kv-lua-0',
        name        = 'KVLUA',
        install_path= '%s/lib' % bld.env.PREFIX,
        vnum        = VERSION
    )

    lib.env = bld.env.derive()
    for k in 'CFLAGS CXXFLAGS LINKFLAGS'.split():
        lib.env.append_unique (k, [ '-fPIC', '-fvisibility=hidden' ])

    bld.install_files ('%s/include/kv-0/kv/lua' % bld.env.PREFIX,
                       ['src/lua-kv.h', 'src/lua-kv.hpp', 'kv/juce_rectangle.hpp' ])
    bld.add_group()
    return lib

def build (bld):
    for module in 'byte midi round'.split():
        build_cmodule (bld, module)
    bld.add_group()

    if bld.env.JUCE:
        for mod in '''AudioBuffer64
                      AudioBuffer32
                      Bounds
                      Component
                      DocumentWindow
                      File
                      Graphics
                      MidiBuffer
                      MidiMessage
                      MouseEvent
                      Point
                      Rectangle'''.split():
            obj = build_classmod (bld, mod)
            obj.use += ['JUCE']
        bld.add_group()

    bld.install_files ('%s/share/lua/5.4/kv' % bld.env.PREFIX,
                       bld.path.ant_glob ('src/kv/*.lua'))

    if bld.env.TEST:
        tests = bld.program (
            source       = [ 'tests/test.c' ],
            includes     = [ '.', 'src' ],
            name         = 'test',
            target       = 'test',
            use          = [ 'LUA', 'LUA_LIB' ],
            linkflags    = [],
            install_path = None
        )

        if 'linux' in sys.platform:
            tests.linkflags.append ('-Wl,--no-as-needed')
            tests.linkflags.append ('-lm')
            tests.linkflags.append ('-ldl')

def check (ctx):
    if 0 != call (["lua", "./test/run.lua"]):
        ctx.fatal ("Tests failed")
