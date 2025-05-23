#######################################################################
# Makefile for mzscheme test-suite
#######################################################################

LANGUAGE     = mzscheme
MZSCHEME     = mzscheme
SCRIPTSUFFIX = _runme.scm

HAVE_CXX11   = 1
HAVE_CXX14   = 1
HAVE_CXX17   = 1
HAVE_CXX20   = 1
srcdir       = .
top_srcdir   = ../../..
top_builddir = ../../..

FAILING_CPP_TESTS = \
allowexcept \
apply_strings \
arrays_dimensionless \
arrays_global \
char_strings \
chartest \
class_scope_weird \
constant_pointers \
cpp11_alternate_function_syntax \
cpp11_director_enums \
cpp11_ref_qualifiers \
cpp11_rvalue_reference2 \
cpp11_strongly_typed_enumerations \
cpp_basic \
cpp_enum \
curiously_recurring_template_pattern \
default_arg_expressions \
default_constructor \
derived_nested \
director_ignore \
enum_thorough \
extend \
friends \
global_scope_types \
inherit_member \
li_attribute \
li_attribute_template \
li_boost_shared_ptr \
li_std_combinations \
li_std_map \
li_std_pair \
li_std_pair_using \
li_std_string \
li_std_vector \
li_windows \
member_funcptr_galore \
member_pointer \
member_pointer_const \
memberin_extend \
namespace_spaces \
naturalvar \
naturalvar_more \
nested_class \
nested_template_base \
ordering \
preproc_constants \
samename \
static_const_member \
string_constants \
template_default2 \
template_specialization_defarg \
template_typemaps \
typemap_variables \
valuewrapper_opaque \

FAILING_C_TESTS = \
enums \
integers \
preproc_constants_c \
preproc_line_file \

FAILING_MULTI_CPP_TESTS = \
multi_import \

include $(srcdir)/../common.mk

# Overridden variables here
SWIGOPT += -w524 # Suppress SWIGWARN_LANG_EXPERIMENTAL warning

# Custom tests - tests with additional commandline options
# none!

# Rules for the different types of tests
%.cpptest:
	$(setup)
	+$(swig_and_compile_cpp)
	$(run_testcase)

%.ctest:
	$(setup)
	+$(swig_and_compile_c)
	$(run_testcase)

%.multicpptest:
	$(setup)
	+$(swig_and_compile_multi_cpp)
	$(run_testcase)

# Runs the testcase. A testcase is only run if
# a file is found which has _runme.scm appended after the testcase name.
run_testcase = \
	if [ -f $(SCRIPTDIR)/$(SCRIPTPREFIX)$*$(SCRIPTSUFFIX) ]; then \
	  env LD_LIBRARY_PATH=.:$$LD_LIBRARY_PATH $(RUNTOOL) $(MZSCHEME) -r $(SCRIPTDIR)/$(SCRIPTPREFIX)$*$(SCRIPTSUFFIX); \
	fi

# Clean
%.clean:
	@exit 0

clean:
	$(MAKE) -f $(top_builddir)/$(EXAMPLES)/Makefile SRCDIR='$(SRCDIR)' mzscheme_clean
