# ----------------------------------------------------------------
# Compile a custom javascript interpreter
# ----------------------------------------------------------------
#
# Note:
#		There is no common CLI Javascript interpreter.
#   V8 comes with one 'd8' which however does not provide a means
#		to load extensions. Therefore, by default we use nodejs as
#		environment.
#   For testing native v8 and jsc extensions we provide our own
#		interpreter (see 'Tools/javascript').
#
# ----------------------------------------------------------------
all: javascript

CC         = gcc
# HACK: under Mac OS X a g++ compiled interpreter is seg-faulting when loading module libraries
# with 'c++' it works... probably some missing flags?
JSCXX      = $(CXX)
CPPFLAGS   = -I/usr/include
CFLAGS     = 
CXXFLAGS   = -std=c++20
LDFLAGS    =
LINKFLAGS = -ldl

ROOT_DIR = /home/harrington/scale_iot_design_04-30-2025/swig-4.3.1
JSINCLUDES =  -I/usr/include/v8
JSDYNAMICLINKING =  -lv8
JSV8ENABLED = 1
JSCENABLED = 

srcdir = .


# Regenerate Makefile if Makefile.in or config.status have changed.
Makefile: $(srcdir)/Makefile.in ../../config.status
	cd ../.. && $(SHELL) ./config.status Tools/javascript/Makefile

# These settings are provided by 'configure' (see '/configure.in')
ifeq (1, $(JSV8ENABLED))
JS_INTERPRETER_SRC_V8 = v8_shell.cxx
JS_INTERPRETER_ENABLE_V8 = -DENABLE_V8 -DV8_DEPRECATION_WARNINGS
endif

ifeq (1, $(JSCENABLED))
JS_INTERPRETER_SRC_JSC = jsc_shell.cxx
JS_INTERPRETER_ENABLE_JSC = -DENABLE_JSC
endif

JS_INTERPRETER_DEFINES = $(JS_INTERPRETER_ENABLE_JSC) $(JS_INTERPRETER_ENABLE_V8)
JS_INTERPRETER_SRC = javascript.cxx js_shell.cxx $(JS_INTERPRETER_SRC_JSC) $(JS_INTERPRETER_SRC_V8)

JS_INTERPRETER_OBJS = $(JS_INTERPRETER_SRC:.cxx=.o)

%.o: $(srcdir)/%.cxx
	$(JSCXX) $(JS_INTERPRETER_DEFINES) $(CPPFLAGS) $(CXXFLAGS) $(JSINCLUDES) -o $@ -c $<

javascript: $(JS_INTERPRETER_OBJS)
	$(JSCXX) $^ $(CXXFLAGS) $(LDFLAGS) -o javascript $(JSDYNAMICLINKING) $(LINKFLAGS)

clean:
	rm -f *.o
	rm -f javascript

distclean: clean
	rm -f Makefile
