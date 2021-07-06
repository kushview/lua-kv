
import os, re, sys
from subprocess import call
from waflib.Build import BuildContext

APPNAME = 'lua-kv'
VERSION = '0.0.1'

def options (opt):
    opt.load ('compiler_c compiler_cxx')
    opt.add_option ('--debug', default=False, action="store_true", dest="debug", \
        help="Compile debuggable binaries [ Default: False ]")
    opt.add_option ('--test', default=False, action='store_true', dest='test', \
        help="Build the test suite [ Default: False ]")
    opt.add_option ('--with-juce', default='', dest='juce', type='string', 
        help='Path to JUCE')

def configure (conf):
    conf.load ('compiler_c compiler_cxx')
    conf.find_program ('ldoc', uselib_store="LDOC", mandatory=False)

    if conf.options.debug:
        conf.define ("DEBUG", 1)
        conf.define ("_DEBUG", 1)
        conf.env.append_unique ('CXXFLAGS', ['-g', '-ggdb', '-O0'])
        conf.env.append_unique ('CFLAGS', ['-g', '-ggdb', '-O0'])
    else:
        conf.define ("NDEBUG", 1)
        conf.env.append_unique ('CXXFLAGS', ['-Os'])
        conf.env.append_unique ('CFLAGS', ['-Os'])
    conf.env.append_unique ('CXXFLAGS', ['-std=c++17'])
    conf.env.append_unique ('CPPFLAGS', ['-DLKV_MODULE'])
    
    if 'darwin' in sys.platform:
        osARCHS = os.getenv ('ARCHS', '')
        if isinstance(osARCHS, str) and len(osARCHS) > 0:
            conf.env.ARCH = osARCHS.split()
        else:
            conf.env.ARCH = []
        
    conf.check_cfg (package='lua', msg='Checking for lua header',  uselib_store='LUA',    args=['--cflags'], mandatory=True)
    conf.check_cfg (package='lua', msg='Checking for lua library', uselib_store='LUALIB', args=['--libs'],   mandatory=True)

    jucepath = conf.options.juce
    if len(jucepath) <= 0:
        jucepath = os.path.join (os.path.expanduser("~"), 'SDKs/JUCE')
    conf.env.JUCE_MODULE_PATH = os.path.join (jucepath, 'modules')
    if not os.path.exists (os.path.join (conf.env.JUCE_MODULE_PATH, 'juce_core/juce_core.h')):
        conf.fatal ("Could not find JUCE modules in %s" % conf.env.JUCE_MODULE_PATH)
    
    conf.env.LUA_VERSION = '5.4'
    conf.env.TEST = bool (conf.options.test)   

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
        mod.env.FRAMEWORK_ACCELERATE     = 'Accelerate'
        mod.env.FRAMEWORK_AUDIO_TOOLBOX  = 'AudioToolbox'
        mod.env.FRAMEWORK_AUDIO_UNIT     = 'AudioUnit'
        mod.env.FRAMEWORK_CORE_AUDIO     = 'CoreAudio'
        mod.env.FRAMEWORK_CORE_AUDIO_KIT = 'CoreAudioKit'
        mod.env.FRAMEWORK_CORE_MIDI      = 'CoreMIDI'
        mod.env.FRAMEWORK_COCOA          = 'Cocoa'
        mod.env.FRAMEWORK_CARBON         = 'Carbon'
        mod.env.FRAMEWORK_DISC_RECORDING = 'DiscRecording'
        mod.env.FRAMEWORK_FOUNDATION     = 'Foundation'
        mod.env.FRAMEWORK_IO_KIT         = 'IOKit'
        mod.env.FRAMEWORK_OPEN_GL        = 'OpenGL'
        mod.env.FRAMEWORK_QT_KIT         = 'QTKit'
        mod.env.FRAMEWORK_QuickTime      = 'QuickTime'
        mod.env.FRAMEWORK_QUARTZ_CORE    = 'QuartzCore'
        mod.env.FRAMEWORK_WEB_KIT        = 'WebKit'
        mod.env.FRAMEWORK_PYTHON         = 'Python'
        mod.use += [ 'COCOA', 'FOUNDATION', 'IO_KIT' ]

    return mod

def juce_module_code (path, module):
    extension = 'cpp'
    if 'darwin' in sys.platform:
        extension = 'mm'

    cpp_only = [ 'juce_analytics', 'juce_osc', 'jlv2_host' ]
    extension = 'cpp' if module in cpp_only else extension
    return os.path.join (path, 'include_%s.%s' % (module, extension))

def build_cmodule (bld):
    mod = bld (
        features    = 'cxx cxxshlib',
        source      =  bld.path.ant_glob ("src/kv/**/*.c") +
                       bld.path.ant_glob ("src/kv/**/*.cpp") + [
                       juce_module_code ('jucer/lua-kv/JuceLibraryCode', 'juce_audio_basics'),
                       juce_module_code ('jucer/lua-kv/JuceLibraryCode', 'juce_core'),
                       juce_module_code ('jucer/lua-kv/JuceLibraryCode', 'juce_data_structures'),
                       juce_module_code ('jucer/lua-kv/JuceLibraryCode', 'juce_events'),
                       juce_module_code ('jucer/lua-kv/JuceLibraryCode', 'juce_graphics'),
                       juce_module_code ('jucer/lua-kv/JuceLibraryCode', 'juce_gui_basics')
                       ],
        includes    = [ 'include', 'src', 'jucer/lua-kv/JuceLibraryCode',
                        bld.env.JUCE_MODULE_PATH ],
        name        = 'KVCMODULE',
        target      = 'lib/lua/kv',
        env         = module_env (bld),
        use         = [ 'LUA' ],
        cflags      = [],
        cxxflags    = [],
        linkflags   = [],
        install_path = '%s/lib/lua/%s/kv' % (bld.env.PREFIX, bld.env.LUA_VERSION)
    )
    return setup_module (mod)

def build_lua_docs (bld):
    if bool(bld.env.LDOC):
        call ([bld.env.LDOC[0], '-f', 'markdown', '.' ])

def docs (ctx):
    ctx.add_pre_fun (build_lua_docs)

def build (bld):
    build_cmodule (bld)
    bld.add_group()
    bld.install_files ('%s/share/lua/5.4/kv' % bld.env.PREFIX,
                       bld.path.ant_glob ('src/kv/*.lua'))

    if False: # bld.env.TEST:
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

class DocsBuildContext (BuildContext):
    cmd = 'docs'
    fun = 'docs'

from waflib import TaskGen
@TaskGen.extension ('.mm')
def juce_mm_hook (self, node):
    return self.create_compiled_task ('cxx', node)

from waflib import TaskGen
@TaskGen.extension ('.m')
def juce_m_hook (self, node):
    return self.create_compiled_task ('c', node)
