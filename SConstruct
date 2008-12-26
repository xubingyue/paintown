import os
# os.sep = '/'

def checkPython(context):
    import distutils.sysconfig
    context.Message("Checking if python is embeddable... ")

    include_path = distutils.sysconfig.get_config_var('INCLUDEPY')
    link_stuff = distutils.sysconfig.get_config_var('LINKFORSHARED')
    libs = distutils.sysconfig.get_config_var('LDLIBRARY')
    lib_path = distutils.sysconfig.get_config_var('LIBP')

    tmp = context.env.Clone()
    env = context.env
    env.Append(CPPPATH = [include_path])
    if link_stuff != None:
        env.Append(LINKFLAGS = link_stuff.split(' '))
    env.Append(LIBPATH = [lib_path])
    env.Append(LIBS = libs)
    env.Append(CPPDEFINES = ['HAVE_PYTHON'])
    ret = context.TryLink("""
        #include <Python.h>
        int main(int argc, char *argv[]) {
            Py_Initialize();
            return 0;
        }
    """, ".c");
    if not ret:
        context.sconf.env = tmp

    context.Result(ret)
    return ret

def isWindows():
    import re
    import sys
    return "win32" in sys.platform

def isOSX():
    import re
    import sys
    return "darwin" in sys.platform

def isCygwin():
    try:
        return os.environ['CYGWIN'] == '1'
    except:
        return False    

def useMingw():
    try:
        return "mingw" in ARGUMENTS[ 'env' ]
    except KeyError:
        return False

def readExec( program ):
    try:
        return os.popen( program ).readline().replace("\n",'')
    except OSError:
        return ""

def getEnvironment():
    if isCygwin():
        import SCons.Tool.zip
        env = Environment(ENV = os.environ, tools = ['mingw'])
        env['CXX'] = 'C:\\MinGW\\bin\\g++.exe'
        env['CC'] = 'C:\\MinGW\\bin\\gcc.exe'
        env['AR'] = 'C:\\MinGW\\bin\\ar.exe'
        SCons.Tool.zip.generate(env)
        return env
    elif useMingw():
        return Environment(ENV = os.environ, tools = ['mingw'])
    else:
        return Environment(ENV = os.environ)

if isWindows():
    print "Try 'scons env=mingw' if you want to use mingw's gcc instead of visual studio or borland"
    if not isCygwin():
        print "Cygwin not detected. If are you using cygwin set"
        print "export CYGWIN=1"
    else:
        print "Cygwin detected"
    
env = getEnvironment()

def getDebug():
    try:
        return int(os.environ[ 'DEBUG' ])
    except KeyError:
        return 0

def getDataPath():
    try:
        return ARGUMENTS['data']
    except KeyError:
        return 'data'

debug = getDebug()
dataPath = getDataPath()

cflags = [ '-Wall' ]
cppflags = [ '-fno-rtti', '-Woverloaded-virtual' ]
cdefines = ['DATA_PATH=\\\"%s\\\"' % dataPath]
# cppflags = [ '-Woverloaded-virtual' ]

if debug:
    cflags.append( ['-g3','-ggdb'] )
else:
    cflags.append( '-O2' )

# env.Append( CCFLAGS = '-m32' )
# env.Append( LINKFLAGS = '-m32' )

# env.Append( CCFLAGS = cflags, CXXFLAGS = cppflags, CPPPATH = [ ".", 'sockets' ] )
env.Append( CCFLAGS = cflags,
            CXXFLAGS = cppflags,
            CPPPATH = [ ".", 'hawknl' ],
            CPPDEFINES = cdefines )

def buildDumb(where, env):
    return SConscript( "src/dumb/SConscript", build_dir = '%s/dumb' % where, exports = 'env' )

def buildHawknl(where, env):
    return SConscript( "src/hawknl/SConscript", build_dir = '%s/hawknl' % where, exports = 'env' )

dumbEnv = getEnvironment()
hawkEnv = getEnvironment()
dumbStaticEnv = getEnvironment()
hawkStaticEnv = getEnvironment()

if isOSX():
    dumbStaticEnv[ 'CXX' ] = 'misc/g++'
    dumbStaticEnv[ 'CC' ] = 'misc/gcc'
    hawkStaticEnv[ 'CXX' ] = 'misc/g++'
    hawkStaticEnv[ 'CC' ] = 'misc/gcc'
    hawkStaticEnv.Append( CPPDEFINES = 'MACOSX' )
    hawkEnv.Append( CPPDEFINES = 'MACOSX' )

hawknl = buildHawknl('build', hawkEnv)
dumb = buildDumb( 'build', dumbEnv )

hawknl_static = buildHawknl('build-static', hawkStaticEnv)
dumb_static = buildDumb('build-static', dumbStaticEnv)

if False:
    env.Append( CCFLAGS = '-pg' )
    env.Append( LINKFLAGS = '-pg' )

staticEnv = env.Clone()

# env.Append( LIBS = [ 'aldmb', 'dumb' ] );

env.Append( LIBS = [dumb,hawknl] )

if isWindows():
    env.Append( LIBS = [ 'alleg', 'pthreadGC2', 'png', 'freetype', 'z', 'wsock32' ] )
    env.Append( CPPDEFINES = 'WINDOWS' )
    env.Append( CCFLAGS = ['-mwindows','-mthreads'] )
    env.Append( LINKFLAGS = ['-mwindows','-mthreads'] )

    config = env.Configure(custom_tests = {"CheckPython" : checkPython})
    config.CheckPython()
    env = config.Finish()

    # env.Append( CCFLAGS = ['-mthreads'] )
    # env.Append( LINKFLAGS = ['-mthreads'] )
    staticEnv.Append( LIBS = [ 'alleg', 'pthreadGC2', 'png', 'freetype', 'z', 'wsock32' ] )
    staticEnv.Append( CPPDEFINES = 'WINDOWS' )
else:
    env.Append( LIBS = [ 'pthread' ] )
    staticEnv.Append( LIBS = [ 'pthread' ] )

    staticEnv.Append( LIBS = [ hawknl_static, dumb_static ] )

    try:
        dumbStaticEnv.ParseConfig( 'allegro-config --cflags' )
    except OSError:
        pass

    if isOSX():
        staticEnv[ 'CXX' ] = 'misc/g++'
        staticEnv[ 'CC' ] = 'misc/gcc'
    
    config = env.Configure(custom_tests = {"CheckPython" : checkPython})
    try:
        config.env.ParseConfig( 'allegro-config --libs --cflags' )
        config.env.ParseConfig( 'freetype-config --libs --cflags' )
        config.env.ParseConfig( 'libpng-config --libs --cflags' )
        
        staticEnv.ParseConfig( 'allegro-config --static --libs --cflags' )
        staticEnv.ParseConfig( 'freetype-config --cflags' )
        staticEnv.ParseConfig( 'libpng-config --cflags' )
    except OSError:
        pass

    ## This is a hack. Copy the static libraries to misc and then link
    ## those in, otherwise gcc will try to pick the .so's from /usr/lib
    png = staticEnv.Install( 'misc', readExec( 'libpng-config --libdir' ) + '/libpng.a' )
    freetype = staticEnv.Install( 'misc', readExec( 'freetype-config --prefix' ) + '/lib/libfreetype.a' )
    staticEnv.Append( LIBS = [png,'z','m'] )
    staticEnv.Append( LIBS = freetype )

    if False:
        ## why doesnt this work!
        if not config.TryCompile("int main(){ return 0; }\n", ".c"):
            print "You need a C compiler such as gcc installed"
            Exit( 1 )
        if not config.TryCompile("class a{}; int main(){ a x = new a(); delete x; return 0; }\n", ".cpp" ):
            print "You need a C++ compiler such as g++ installed"
            Exit( 1 )
    if not config.CheckHeader( 'allegro.h' ):
        print "You need the header files for Allegro. Get it from http://alleg.sf.net"
        Exit( 1 )
    if not config.CheckHeader( 'ft2build.h' ):
        print "You need freetype. Install freetype and/or X11"
        Exit( 1 )
    if not config.CheckHeader( 'png.h' ):
        print "You need libpng. Get it from http://www.libpng.org/pub/png/libpng.html"
        Exit( 1 )
    config.CheckPython()

    env = config.Finish()

    static_config = staticEnv.Configure(custom_tests = {"CheckPython" : checkPython})
    static_config.CheckPython()
    staticEnv = static_config.Finish()

if not isWindows():
   env.Append(CCFLAGS = ['-Werror'])
staticEnv.Append(CCFLAGS = ['-Werror'])

use = env
shared = SConscript( 'src/SConstruct', build_dir='build', exports = ['use'] );

use = staticEnv
static = SConscript( 'src/SConstruct', build_dir='build-static', exports = ['use'] )

for i in shared:
    Default(env.Install( '.', i ))

for i in static:
    Alias('static',env.InstallAs( i[0].name + '-static', i ))

# if False:
# 	if isWindows():
# 		env.Install( '.', 'build/paintown.exe' )
# 		env.Install( '.', 'build/test.exe' )
# 	else:
# 		env.Install( '.', 'build/paintown' )
# 		env.Install( '.', 'build/test' )
