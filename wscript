
import os, re, sys

def options(opt):
    opt.load ('compiler_c')

def configure (conf):
    conf.load ('compiler_c')
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

def build (bld):
    mod = bld (
        features    = 'c cshlib',
        source      = [ 'decibels.c' ],
        name        = 'decibels',
        target      = 'decibels',
        env         = module_env (bld),
        use         = [ 'LUA' ],
        cflags      = [],
        linkflags   = [],
        install_path = '%s/lib/lua/5.3' % bld.env.PREFIX
    )
    
    if sys.platform == 'linux':
        pass
    elif sys.platform == 'windows':
        pass
    elif sys.platform == 'darwin':
        mod.linkflags.append('-fPIC')
        mod.mac_bundle = True
