/****************************************************************************
 **  TAU Portable Profiling Package                                        **
 **  http://tau.uoregon.edu                                                **
 ****************************************************************************
 **  Copyright 2021                                                        **
 **  Department of Computer and Information Science, University of Oregon  **
 ****************************************************************************/
/****************************************************************************
 **      File            : tau_wrap++.cpp                                  **
 **      Description     : Generates a wrapper library for external pkgs   **
 **                        for instrumentation with TAU.                   **
 **      Author          : Kevin Huck                                      **
 **      Contact         : khuck@cs.uoregon.edu                            **
 **      Documentation   : https://github.com/khuck/clangwrap              **
 ***************************************************************************/

#define _GLIBCXX_USE_CXX11_ABI 1

/* Headers */
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#if (!defined(TAU_WINDOWS))
#include <unistd.h>
#endif //TAU_WINDOWS
#include <cxxabi.h>
#include <limits.h>

#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <clang-c/Index.h>
#include <cctype>
#include <locale>
#include "string_alignment.h"
#include "json.h"
using json = nlohmann::json;
json configuration;

inline void _wrap_assert(const char* expression, const char* file, int line)
{
    fprintf(stderr, "Assertion '%s' failed, file '%s' line '%d'.",
        expression, file, line);
    abort();
}

#define WRAP_ASSERT(EXPRESSION) ((EXPRESSION) ? (void)0 : \
    _wrap_assert(#EXPRESSION, __FILE__, __LINE__))

// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
    ltrim(s);
    rtrim(s);
}

/* Check if a string ends with a substring */
inline bool ends_with(std::string const & value, std::string const & ending) {
    if (ending.size() > value.size()) return false;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

/* replace all occurrances of substring in string with another substring */
bool replace_all(std::string& instr, const std::string old, const std::string newstr) {
    std::string::size_type n = 0;
    bool changed{false};
    while ( ( n = instr.find( old, n ) ) != std::string::npos )
    {
        instr.replace( n, old.size(), newstr);
        n += newstr.size();
        changed = true;
    }
    return changed;
}

bool contains(std::string& instr, const std::string needle) {
    return ( ( instr.find( needle ) ) != std::string::npos );
}

/* Globals */
bool memory_flag = false;   /* by default, do not insert malloc.h in instrumented C/C++ files */
bool strict_typing = false; /* by default unless --strict option is used. */

/* useful constant strings */
const std::string _const{"const"};
const std::string _signed{"signed "};
const std::string _unsigned{"unsigned "};
const std::string _noexcept{"noexcept"};
const std::string _empty{""};
const std::string _space{" "};
const std::string _address{"*"};
const std::string _reference{"&"};

/* Custom string replacements - should be from config file */
const std::string c11_string{"std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >"};
const std::string old_string{"std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char> >"};
const std::string simple_string{"std::string"};
const std::string ompi_string{"ompi_communicator_t*"};
const std::string mpi_string{"MPI_Comm"};
const std::string parser_flags{"parser_flags"};
const std::string template_types{"template_types"};
const std::string skip_classes{"classes to skip"};
const std::string skip_methods{"methods to skip"};
const std::string tau_timer_group{"TAU timer group"};
const std::string enable_trace_plugin{"enable trace plugin"};
const std::string printable_trace_types{"printable trace types"};

/* This is the default configuration.
 * For different environments, use a configuration file.
 * Sections:
 *   symbol_map: used for mapping symbols in the library to something
 *       specified in the header.  After compilation and linking,
 *       some symbols get expanded to their true types, but when
 *       the header is parsed by clang, nothing is converted.
 *       These mappings assist the string alignment phase to
 *       increase accuracy.
 *   parser_flags: The libclang parser needs arguments to specify darn
 *       near everything. That includes the path to the library headers,
 *       the libclang headers (C++ and C), and system headers.  Also,
 *       the `-x c++` flag tells libclang that this is a C++ file.
 *       Enable any flags that would be used to compile an application
 *       that uses the library to be wrapped.
 */
const char * default_configuration = R"(
{
    "symbol_map": [
        {
            "from": "ompi_communicator_t*",
            "to": "MPI_Comm"
        }
    ],
    "parser_flags": [
        "-x",
        "c++",
        "-std=c++11",
        "-DADIOS2_USE_MPI",
        "-I.",
        "-I/packages/llvm/11.0.1/include/c++/v1",
        "-I/packages/llvm/11.0.1/include",
        "-I/usr/local/packages/gcc/7.3.0/lib/gcc/powerpc64le-unknown-linux-gnu/7.3.0/include",
        "-I/usr/local/packages/gcc/7.3.0/lib/gcc/powerpc64le-unknown-linux-gnu/7.3.0/include-fixed",
        "-I/usr/local/packages/openmpi/4.0.1-gcc7.3/include",
        "-I/usr/include",
        "-I/usr/local/include",
        "-I/usr/local/packages/ADIOS2/2021.02.05-Debug/mpi/include"
    ],
    "template_types": [
        "char",
        "signed char",
        "unsigned char",
        "short",
        "unsigned short",
        "int",
        "long int",
        "long long int",
        "long",
        "long long",
        "unsigned int",
        "unsigned long int",
        "unsigned long long int",
        "unsigned long",
        "unsigned long long",
        "float",
        "double",
        "long double",
        "std::complex<float>",
        "std::complex<double>",
        "std::string"
    ],
    "classes to skip": [
        "adios::Span::iterator",
        "adios::detail::Span::iterator"
    ],
    "methods to skip": [
        "adios::Variable::Operations",
        "adios::ADIOS::DefineCallBack"
    ]
})";

void show_usage(char const * argv0)
{
    std::cout <<"-----------------------------------------------------------------------------"<<std::endl;
    std::cout <<"Usage : "<< argv0 <<" <header> [-w <library>] [-n <namespace>] [-c <config_file>]"<<std::endl;
    std::cout <<" e.g., "<<std::endl;
    std::cout <<"   " << argv0 << " secret.h -w libsecret.so -n secret -c config.json" << std::endl;
    std::cout <<"-----------------------------------------------------------------------------"<<std::endl;
}

typedef struct symbolData {
    std::string mangledName;
    size_t nArgs;
} symbolData_t;

void readConfigFile(std::string filename) {
    if (filename.size() == 0) {
        std::cout << "Using default configuration." << std::endl;
        configuration = json::parse(default_configuration);
        return;
    }
    try {
        std::ifstream cfg(filename);
        // if the file doesn't exist, error.
        if (!cfg.good()) {
            std::cerr << "Error reading " << filename << std::endl;
            abort();
        }
        cfg >> configuration;
        cfg.close();
    } catch (...) {
        std::cerr << "Error reading config.json file!" << std::endl;
        abort();
    }
}

std::ofstream wrapper("wr.cpp", std::ofstream::out);
// Map from type signature to mangled name and argument list
std::map<std::string, symbolData_t> symbolMap;
// Map from "using" alias to actual type
std::map<std::string, std::string> aliasMap;
std::string mainNamespace{"secret"};

std::string get_tau_timer_group() {
    if (configuration.count(tau_timer_group) > 0) {
        std::string group{configuration[tau_timer_group]};
        return group;
    }
    std::string group{mainNamespace};
    transform(group.begin(), group.end(), group.begin(), ::toupper);
    return group;
}


/* Write the preamble to the source file */
void writePreamble(std::string header, std::vector<std::string> libraries) {
    constexpr const char * headers = R"(
#include <Profile/Profiler.h>
#include <Profile/TauPluginTypes.h>
#include <Profile/TauPluginInternals.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <iostream>
#include <complex>
#include "inttypes.h"
#include "mpi.h"

#ifdef _DEBUG
#define MARKER printf("*** %s %s: %d\n", __func__, __FILE__, __LINE__)
#else
#define MARKER
#endif
)";
    constexpr const char * loadHandle = R"(
void * load_handle(const char * tau_orig_libname) {
    MARKER;
    void *handle = (void *) dlopen(tau_orig_libname, RTLD_NOW);
    if (handle == NULL) {
        std::cerr << "Error opening library "
                  << tau_orig_libname
                  << " in dlopen call"
                  << std::endl;
    }
    return handle;
}
)";
    constexpr const char * loadHandles = R"(
std::vector<void*> load_handles() {
    std::vector<void*> handles;
    if (handles.empty()) {
        const char * names[] = {)";
    constexpr const char * loadHandles2 = R"( };
        for (auto n : names) {
            handles.push_back(load_handle(n));
        }
    }
    return handles;
}
)";
    constexpr const char * loadSymbol = R"(
template<class T> T* load_symbol(char const* name) {
    MARKER;
    static auto handles = load_handles();
    for(auto h : handles) {
        void * tmp = dlsym(h,name);
        if (tmp != NULL) {
            return reinterpret_cast<T*>(tmp);
        }
    }
    std::cerr << "Error obtaining symbol " << name
              << " from libraries!" << std::endl;
    return nullptr;
}
)";
    constexpr const char * helperFunctions = R"(
/* Helper to trace vector parameters */
template < class T >
std::string ToString(const std::vector<T>& v) {
    std::stringstream ss;
    std::string d{"["};
    for (const auto &e : v) {
        ss << d;
        ss << e;
        d = ",";
    }
    if (v.size() > 0) {
        ss << "]";
    }
    std::string tmp{ss.str()};
    return tmp;
}

template < class T >
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
    os << ToString(v);
    return os;
}

template < class T >
std::ostream& operator<<(std::ostream& os, std::vector<T>& v) {
    os << ToString(v);
    return os;
}

/* Helper to trace set parameters */
template < class T >
std::ostream& operator<<(std::ostream& os, const std::set<T>& s) {
    std::string d{"["};
    for (const auto &e : s) {
        os << d;
        os << e;
        d = ",";
    }
    if (s.size() > 0) {
        os << "]";
    }
    return os;
}

template < class T >
std::ostream& operator<<(std::ostream& os, std::set<T>& s) {
    std::string d{"["};
    for (const auto &e : s) {
        os << d;
        os << e;
        d = ",";
    }
    if (s.size() > 0) {
        os << "]";
    }
    return os;
}

/* Helper to trace pair parameters */
template < class T, class V >
std::ostream& operator<<(std::ostream& os, const std::pair<T,V>& p) {
    os << "(" << p.first << "," << p.second << ')';
    return os;
}

template < class T, class V >
std::ostream& operator<<(std::ostream& os, std::pair<T,V>& p) {
    os << "(" << p.first << "," << p.second << ')';
    return os;
}

/* Helper to trace map parameters */
template < class T, class V >
std::ostream& operator<<(std::ostream& os, const std::map<T,V>& m) {
    std::string d{"["};
    for (const auto &kv : m) {
        os << d;
        os << kv.first << ":" << kv.second;
        d = ",";
    }
    if (m.size() > 0) {
        os << "]";
    }
    return os;
}

template < class T, class V >
std::ostream& operator<<(std::ostream& os, std::map<T,V>& m) {
    std::string d{"["};
    for (const auto &kv : m) {
        os << d;
        os << kv.first << ":" << kv.second;
        d = ",";
    }
    if (m.size() > 0) {
        os << "]";
    }
    return os;
}

template<class T>
std::string escape_me(T var) {
    std::stringstream ss;
    ss << var;
    std::string tmp{ss.str()};
    std::string::size_type n = 0;
    const std::string doublequote{"\""};
    const std::string singlequote{"'"};
    while ( ( n = tmp.find( doublequote, n ) ) != std::string::npos )
    {
        tmp.replace( n, doublequote.size(), singlequote);
        n += singlequote.size();
    }
    return tmp;
}

/* To support the adios2::Dim type */
template<>
std::string escape_me <std::vector<long unsigned int>>(std::vector<long unsigned int> var) {
    std::string tmp{ToString(var)};
    return tmp;
}

std::string convert_comm(MPI_Comm comm) {
    char tmpstr[33];
    if (comm == MPI_COMM_WORLD) {
        sprintf(tmpstr, "MPI_COMM_WORLD");
    } else if (comm == MPI_COMM_SELF) {
        sprintf(tmpstr, "MPI_COMM_SELF");
    } else if (comm == MPI_COMM_NULL) {
        sprintf(tmpstr, "MPI_COMM_NULL");
    } else {
        sprintf(tmpstr, "0x%" PRIx64, (uint64_t)comm);
    }
    std::string ts{tmpstr};
    return ts;
}

)";

    constexpr const char * tauPluginFunction = R"(
void Tau_plugin_trace_current_timer(const char * name) {
    /*Invoke plugins only if both plugin path and plugins are specified*/
    if(TauEnv_get_plugins_enabled()) {
        Tau_plugin_event_current_timer_exit_data_t plugin_data;
        plugin_data.name_prefix = name;
        Tau_util_invoke_callbacks(TAU_PLUGIN_EVENT_CURRENT_TIMER_EXIT, name, &plugin_data);
    }
}
)";
    constexpr const char * tauMacro = R"(
#define WRAPPER(name) \
  static void *tauFI = 0; \
  if (tauFI == 0) tauCreateFI(&tauFI, name, "", (TauGroup_t)TAU_USER, "SECRET"); \
  Tau_Profile_Wrapper tauFProf(tauFI);
)";
#ifdef _WIN32
    char sep = '\\';
#else
    char sep = '/';
#endif

    // write the basic headers
    wrapper << headers;
    // write the library header
    wrapper << "#include \"";
    size_t i = header.rfind(sep, header.length());
    if (i != std::string::npos) {
        wrapper << (header.substr(i+1, header.length()));
    } else {
        wrapper << header;
    }
    wrapper  << "\"\n";
    // write our utility functions
    wrapper << loadHandle;
    wrapper << loadHandles;
    std::string delimiter = "\"";
    for(auto library : libraries) {
        wrapper << delimiter;
        i = library.rfind(sep, library.length());
        if (i != std::string::npos) {
            wrapper << (library.substr(i+1, library.length()));
        } else {
            wrapper << library;
        }
        wrapper << "\"";
        delimiter = ", \"";
    }
    wrapper << loadHandles2;
    wrapper << loadSymbol;
    wrapper << helperFunctions << "\n";
    bool do_trace = false;
    if (configuration.count(enable_trace_plugin) > 0) {
        do_trace = configuration[enable_trace_plugin];
    }
    if (do_trace) {
        wrapper << tauPluginFunction;
    }
    // write our tau macro
    std::string tmp{tauMacro};
    replace_all(tmp, "SECRET", get_tau_timer_group());
    wrapper << tmp << "\n";
    return;
}

bool methodIsConst(std::string methodType) {
    std::string paren{")"};
    std::size_t found = methodType.rfind(paren);
    if (found != std::string::npos) {
        found = methodType.find(_const, found);
        if (found != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool methodIsNoexcept(std::string methodType) {
    std::string paren{")"};
    std::size_t found = methodType.rfind(paren);
    if (found != std::string::npos) {
        found = methodType.find(_noexcept, found);
        if (found != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool typeIsReference(std::string intype) {
    return (ends_with(intype, _reference));
}

bool typeIsAddress(std::string intype) {
    return (ends_with(intype, _address));
}

void validateParameterNames(std::vector<std::string>& names) {
    size_t length = names.size();
    for (size_t index = 0 ; index < length ; index++) {
        if (names[index].size() == 0) {
            std::stringstream ss;
            ss << "arg" << index;
            std::string tmp{ss.str()};
            names.at(index) = tmp;
        }
    }
}

std::string expandType(std::vector<std::string> namespaceName, std::string intype,
bool isTemplate, std::string templateType, std::string instanceType) {
    //std::cout << "*** " << intype << std::endl;
    replace_all(intype, "typename ", "");
    std::string np{intype};
    // first, replace the fully qualified namespace, if used
    if (namespaceName.size() > 0) {
        std::stringstream ss;
        for (auto ns : namespaceName ) {
            ss << ns << "::";
        }
        for(auto at : aliasMap) {
            // add the namespace to the alias
            std::string tmp{ss.str()+at.first};
            //std::cout << "Replacing " << tmp << " with " << at.second << " in " << np << std::endl;
            replace_all(np, tmp, at.second);
        }
    }
    // second, replace the alias alone (if not already done)
    for(auto at : aliasMap) {
        //std::cout << "Replacing " << at.first << " with " << at.second << " in " << np << std::endl;
        replace_all(np, at.first, at.second);
    }
    //std::cout << "### " << np << std::endl;
    return np;
}

/* Change "const <type> &" to "<type> const &"
 * or     "const <type> *" to "<type> const *"
 * or     "const <type>"   to "<type> const"
 */
std::string standardizeConstType(std::string const & intype) {
    // if the type starts with "const "
    if (intype.find(_const, 0) != std::string::npos) {
        std::stringstream ss;
        // remove const from the name
        std::string np{intype};
        replace_all(np, _const, _empty);
        ltrim(np);
        // does it end with '&' or '*'?
        if (typeIsAddress(np)) {
            // remove the trailing '*'
            np = np.substr(0, np.size()-1);
            rtrim(np);
            ss << np << _space << _const;
            ss << _address;
        } else if (typeIsReference(np)) {
            // remove the trailing '&'
            np = np.substr(0, np.size()-1);
            rtrim(np);
            ss << np << _space << _const;
            ss << _reference;
        } else {
            ss << np << _space << _const;
        }
        // if not, just move const to the end
        std::string tmp{ss.str()};
        return tmp;
    }
    return intype;
}

void standardizeConstTypes(std::vector<std::string>& types) {
    size_t length = types.size();
    for (size_t index = 0 ; index < length ; index++) {
        types.at(index) = standardizeConstType(types[index]);
    }
}

std::string makeMangled(
    std::vector<std::string> namespaceName,
    std::vector<std::string> className,
    std::string methodName,
    std::string methodReturnType,
    std::string methodType,
    bool methodStatic,
    std::vector<std::string> parameterNames,
    std::vector<std::string> parameterTypes,
    bool isTemplate,
    std::string templateType,
    std::string instanceType){
    standardizeConstTypes(parameterTypes);
    // expand the types to aid the search
    methodReturnType = expandType(namespaceName, methodReturnType, isTemplate, templateType, instanceType);
    for(size_t i = 0 ; i < parameterTypes.size() ; i++) {
        parameterTypes[i] = expandType(namespaceName, parameterTypes[i], isTemplate, templateType, instanceType);
    }
    // declare the function type, class and name
    std::stringstream ss;
    //ss << methodReturnType << _space;
    std::stringstream justMethod;
    for (auto ns : namespaceName ) {
        justMethod << ns << "::";
    }
    for (auto cn : className ) {
        justMethod << cn << "::";
    }
    justMethod << methodName;
    std::string fullMethod{justMethod.str()};
    ss << fullMethod;
    // write the arguments
    std::stringstream args;
    args << "(";
    bool multiple{false};
    for (size_t i = 0 ; i < parameterTypes.size() ; i++) {
        if (multiple) { args << ", "; }
        //for (auto ns : namespaceName ) {
        //    args << ns << "::";
        //}
        args << parameterTypes[i];
        multiple = true;
    }
    args << ")";
    ss << args.rdbuf();
    if (methodIsConst(methodType)) {
        ss << _space << _const;
    }
    if (methodIsNoexcept(methodType)) {
        ss << _space << _noexcept;
    }
    std::string signature{ss.str()};
    std::string signatureWithReturn{methodReturnType+_space+signature};
    if (symbolMap.count(signature) == 1) {
        std::string mangled{symbolMap[signature].mangledName};
        symbolMap.erase(signature);
        return mangled;
    }
    if (symbolMap.count(signatureWithReturn) == 1) {
        std::string mangled{symbolMap[signatureWithReturn].mangledName};
        symbolMap.erase(signatureWithReturn);
        return mangled;
    }
    // no worries, we'll try string alignment.
    int pxy = 4;
    int pgap = 1;
    std::string minkey{""};
    int minval = INT_MAX;
    //std::cout << "Searching for: " << signature << std::endl;
    /* First, check if the demangled name includes the return type */
    for (auto mangle_pair : symbolMap) {
        if (mangle_pair.first.find(fullMethod, 0) != std::string::npos &&
            mangle_pair.second.nArgs == parameterTypes.size()) {
            int penalty = getMinimumPenalty(signatureWithReturn, mangle_pair.first, pxy, pgap);
            if (penalty < minval) {
                minval = penalty;
                minkey = mangle_pair.first;
            }
        }
    }
    /* Second, check through the demangled names that don't have return types.
     * how do we know if they don't have return types? Well, if we search for the
     * full method in the demangled name, and it doesn't start at 0, skip it. */
    for (auto mangle_pair : symbolMap) {
        size_t location = mangle_pair.first.find(fullMethod, 0);
        if (location != std::string::npos && location == 0 &&
            mangle_pair.second.nArgs == parameterTypes.size()) {
            int penalty = getMinimumPenalty(signature, mangle_pair.first, pxy, pgap);
            if (penalty < minval) {
                minval = penalty;
                minkey = mangle_pair.first;
            }
        }
    }
    //symbolMap.erase(minkey);
    if (minkey.size() > 0) {
        std::string mangled{symbolMap[minkey].mangledName};
        wrapper << "/* Target: " << signatureWithReturn << "*/\n";
        wrapper << "/* Found:  " << minkey << "*/\n";
        wrapper << "/* Score:  " << minval << "*/\n";
        return mangled;
    }
    //std::cout << "NF: " << signatureWithReturn << std::endl;
    return minkey;
}

bool hasReturnType(std::string methodReturnType, bool isConstructor, bool isDestructor) {
    if (isConstructor || isDestructor) {
        return false;
    }
    if (methodReturnType.size() == 0) {
        return false;
    }
    if (methodReturnType == "void") {
        return false;
    }
    return true;
}

std::set<std::string>& getNonMovableTypes() {
    static std::set<std::string> typeset;
    typeset.insert("void");
    typeset.insert("bool");
    typeset.insert("char");
    typeset.insert("short");
    typeset.insert("int");
    typeset.insert("long int");
    typeset.insert("long long int");
    typeset.insert("long");
    typeset.insert("long long");
    typeset.insert("float");
    typeset.insert("double");
    typeset.insert("size_t");
    typeset.insert("std::complex<float>");
    typeset.insert("std::complex<double>");
    typeset.insert("std::string");
    return typeset;
}

bool hasMovableReturnType(std::string methodReturnType) {
    static std::set<std::string> typeset = getNonMovableTypes();
    if (methodReturnType.size() == 0) {
        return false;
    }
    std::string tmp{methodReturnType};
    replace_all(tmp, _const, _empty);
    replace_all(tmp, _unsigned, _empty);
    replace_all(tmp, _signed, _empty);
    if (typeset.count(tmp) > 0) {
        return false;
    }
    return true;
}

bool isMpiComm(std::string type) {
    if (typeIsAddress(type)) {
        return false;
    }
    // remove any const, (un)signed, or reference (&)
    replace_all(type, _const, _empty);
    replace_all(type, _unsigned, _empty);
    replace_all(type, _signed, _empty);
    replace_all(type, _reference, _empty);
    trim(type);
    if (type.compare("MPI_Comm") == 0) {
        return true;
    }
    return false;
}

std::set<std::string> loadPrintableTraceTypes() {
    std::set<std::string> typeset;
    if (configuration.count(printable_trace_types) > 0) {
        auto printable_types = configuration[printable_trace_types];
        for (auto t : printable_types) {
            std::string tmp{t};
            typeset.insert(tmp);
        }
    }
    return typeset;
}

bool isPrintable(std::string type) {
    static std::set<std::string> typeset = loadPrintableTraceTypes();
    // addresses are printable.
    if (typeIsAddress(type)) {
        return true;
    }
    // remove any const, (un)signed, or reference (&)
    replace_all(type, _const, _empty);
    replace_all(type, _unsigned, _empty);
    replace_all(type, _signed, _empty);
    replace_all(type, _reference, _empty);
    trim(type);
    if (typeset.count(type) > 0) {
        return true;
    }
    return false;
}

std::string getClassFromMethod(std::string fullMethodName) {
    size_t i = fullMethodName.rfind("::", fullMethodName.length());
    if (i != std::string::npos) {
        std::string tmp{fullMethodName.substr(0, i)};
        // trim any template specializations
        size_t i = tmp.find("<", 0);
        if (i != std::string::npos) {
            return tmp.substr(0, i);
        }
        return tmp;
    }
    return fullMethodName;
}

std::string trimSpecialization(std::string fullType) {
    // trim any template specializations
    size_t i = fullType.find("<", 0);
    if (i != std::string::npos) {
        return fullType.substr(0, i);
    }
    return fullType;
}

void writeThisValue(
    std::ofstream& wrapper,
    std::string& fullMethodName,
    bool hasThis
    ) {
    if (hasThis) {
        std::string classType{getClassFromMethod(fullMethodName)};
        if (isPrintable(classType)) {
            wrapper << "std::string tv{escape_me(*this)};\n";
        } else {
            wrapper << "void* tv = (void*)(this);\n";
        }
    }
}

void writeReturnValue(
    std::ofstream& wrapper,
    std::string& methodReturnType,
    bool hasReturn
    ) {
    if (hasReturn) {
        if (isMpiComm(trimSpecialization(methodReturnType))) {
            wrapper << "std::string rv{convert_comm(retval)};\n";
        } else if (isPrintable(trimSpecialization(methodReturnType))) {
            wrapper << "std::string rv{escape_me(retval)};\n";
        } else {
            wrapper << "void* rv = (void*)(std::addressof(retval));\n";
        }
    }
}

void writeArgsValues(std::ofstream& wrapper,
    std::vector<std::string>& parameterNames,
    std::vector<std::string>& parameterTypes
    ) {
    wrapper << "std::stringstream ssargs;\n";
    wrapper << "ssargs << \"{";
    std::string d{" "};
    for (size_t i = 0; i < parameterTypes.size() ; i++) {
        wrapper << d << "\\\"" << i << "\\\": {\\\"name\\\": ";
        wrapper << "\\\"" << parameterNames[i] << "\\\", ";
        wrapper << "\\\"value\\\": \\\"\";\n";
        if (isMpiComm(trimSpecialization(parameterTypes[i]))) {
            wrapper << "std::string at" << i << "{";
            wrapper << "convert_comm(" << parameterNames[i] << ")};\n";
            wrapper << "ssargs << at" << i;
        } else if (isPrintable(trimSpecialization(parameterTypes[i]))) {
            wrapper << "std::string at" << i << "{";
            wrapper << "escape_me(" << parameterNames[i] << ")};\n";
            wrapper << "ssargs << at" << i;
        } else {
            wrapper << "ssargs << (void*)(std::addressof(";
            wrapper << parameterNames[i];
            wrapper << "))";
        }
        wrapper << " << \"\\\", \\\"type\\\": \\\"";
        wrapper << parameterTypes[i] << "\\\"}";
        d = ", ";
    }
    wrapper << "}\";\n";
    wrapper << "std::string argsv{ssargs.str()};\n";
}

void writeTraceEvent(std::ofstream& wrapper,
    std::string& fullMethodName,
    bool hasReturn, bool hasThis
    ) {
    wrapper << "std::stringstream ss;\n";
    wrapper << "ss << \"\\\"cat\\\": \\\"";
    wrapper << get_tau_timer_group();
    wrapper << "\\\", \\\"name\\\": \\\"";
    wrapper << fullMethodName;
    wrapper << "\\\"";
    if (hasReturn) {
        wrapper << ", \\\"return\\\": \\\"\" << rv << \"\\\"";
    }
    if (hasThis) {
        wrapper << ", \\\"this\\\": \\\"\" << tv << \"\\\"";
    }
    wrapper << ", \\\"args\\\": \" << argsv << \"";
    wrapper << "\";\n";
    wrapper << "std::string trace_string{ss.str()};\n";
    //wrapper << "Tau_plugin_trace_current_timer(trace_string.c_str());\n";
}

void writeMethod(
    std::vector<std::string> namespaceName,
    std::vector<std::string> className,
    std::string methodName,
    std::string methodMangled,
    std::string methodReturnType,
    std::string methodType,
    bool methodStatic,
    std::vector<std::string> parameterNames,
    std::vector<std::string> parameterTypes,
    bool isConstructor = false,
    bool isDestructor = false,
    size_t numSpecializations = 0,
    std::string templateType = "T",
    std::string instanceType = "char") {
/*
int Secret::foo1(int a1)  {
      MARKER;
      const char * mangled = "_ZN6secret6Secret4foo1Ei";
      const char * timer_name = "int secret::Secret::foo1(int)";
      using f_t = int(void*,int);
      static f_t* f{load_symbol<f_t>(mangled)};
      WRAPPER(timer_name);
      auto retval = f(this, a1);
      return retval;
  }
 */
    validateParameterNames(parameterNames);
    methodMangled = makeMangled(
        namespaceName,
        className,
        methodName,
        methodReturnType,
        methodType,
        methodStatic,
        parameterNames,
        parameterTypes,
        (numSpecializations > 0), // isTemplate
        templateType,
        instanceType);
    // no mangled exists?  don't need it.
    if (methodMangled.size() == 0) {
        return;
    }

    // Write a comment header
    wrapper << "/" << std::string(80,'*') << "\n";
    wrapper << _space;
    for (auto cn : className ) {
        wrapper << cn << "::";
    }
    wrapper << methodName << "\n";
    wrapper << _space << std::string(80,'*') << "/\n\n";

    // declare the function type, class and name
    std::stringstream ss;
    for (size_t i = 0 ; i < numSpecializations ; i++) {
        wrapper << "template <> ";
    }
    if (!isConstructor && !isDestructor) {
        ss << methodReturnType << _space;
    }
    for (auto cn : className ) {
        ss << cn << "::";
    }
    ss << methodName;

    // write the arguments
    std::stringstream args;
    args << "(";
    bool multiple{false};
    for (size_t i = 0 ; i < parameterTypes.size() ; i++) {
        if (multiple) { args << ", "; }
        args << parameterTypes[i];
        args << _space;
        args << parameterNames[i];
        multiple = true;
    }
    args << ")";
    ss << args.rdbuf();
    if (methodIsConst(methodType)) {
        ss << _space << _const;
    }
    if (methodIsNoexcept(methodType)) {
        ss << _space << _noexcept;
    }
    std::string signature{ss.str()};
    std::stringstream ss2;
    if (!isConstructor && !isDestructor) {
        ss2 << methodReturnType << _space;
    }
    std::stringstream ss3;
    for (auto ns : namespaceName ) {
        ss3 << ns << "::";
    }
    for (auto cn : className ) {
        ss3 << cn << "::";
    }
    ss3 << methodName;
    std::string fullMethodName{ss3.str()};
    ss2 << ss3.str();
    ss2 << args.str();
    if (methodIsConst(methodType)) {
        ss2 << _space << _const;
    }
    if (methodIsNoexcept(methodType)) {
        ss2 << _space << _noexcept;
    }
    // write the arguments
    std::string fullSignature{ss2.str()};
    wrapper << signature << " {\n";

    // write a debugger line
    wrapper << "    MARKER;\n";
    wrapper << "    const char * mangled = \"" << methodMangled << "\";\n";
    // write the timer name
    wrapper << "    const char * timer_name = \"[WRAPPER] " << fullSignature << "\";\n";
    // build a type, depending on whether the method is static.
    std::stringstream ctype;
    ctype << methodReturnType << "(";
    multiple = false;
    if (!methodStatic && className.size() > 0) {
        if (methodIsConst(methodType)) {
            ctype << "const ";
        }
        ctype << "void*";
        multiple = true;
    }
    for (auto p : parameterTypes) {
        if (multiple) { ctype << ","; }
        ctype << p;
        multiple = true;
    }
    ctype << ")";
    // declare a typedef
    wrapper << "    using f_t = " << ctype.str() << ";\n";
    // get the symbol if necessary
    wrapper << "    static f_t* f{load_symbol<f_t>(mangled)};\n";
    wrapper << "    if (Tau_time_traced_api_call() == 1) {\n";
    wrapper << "    Tau_traced_api_call_enter();\n";
    // declare and start the timer
    wrapper << "    WRAPPER(timer_name);\n";
    /* Optionally, generate a timer exit plugin call */
    bool do_trace = false;
    if (configuration.count(enable_trace_plugin) > 0) {
        do_trace = configuration[enable_trace_plugin];
    }
    // get the value of "this" NOW, in case the function destroys
    // any of the arguments, including the "this" pointer.
    // We won't be able to get it after the destructor is called.
    if (do_trace) {
        if (!isConstructor){
            writeThisValue(wrapper, fullMethodName,
                (!methodStatic && className.size() > 0));
        }
        writeArgsValues(wrapper, parameterNames, parameterTypes);
    }
    // call the actual function with timer
    wrapper << "    ";
    if (hasReturnType(methodReturnType, isConstructor, isDestructor)) {
        wrapper << methodReturnType << " retval = ";
    }
    wrapper << "f(";
    multiple = false;
    if (!methodStatic && className.size() > 0) {
        wrapper << "this";
        multiple = true;
    }
    for (auto p : parameterNames) {
        if (multiple) { wrapper << ", "; }
        wrapper << p;
        multiple = true;
    }
    wrapper << ");\n";
    if (do_trace) {
        // get the value of "this" NOW, in case this is a constructor!
        // We won't be able to get it before the constructor is called.
        if (isConstructor){
            writeThisValue(wrapper, fullMethodName,
                (!methodStatic && className.size() > 0));
        }
        // get the return value now, too - it wasn't available before the call
        writeReturnValue(wrapper, methodReturnType,
            (hasReturnType(methodReturnType, isConstructor, isDestructor)));
        // now build the full string and write a trace event.
        writeTraceEvent(wrapper, fullMethodName,
            (hasReturnType(methodReturnType, isConstructor, isDestructor)),
            (!methodStatic && className.size() > 0));
        wrapper << "Tau_plugin_trace_current_timer(trace_string.c_str());\n";
    }
    wrapper << "    Tau_traced_api_call_exit();\n";
    if (hasReturnType(methodReturnType, isConstructor, isDestructor)) {
        if(typeIsAddress(methodReturnType) ||
           typeIsReference(methodReturnType) ||
           !hasMovableReturnType(methodReturnType)) {
            wrapper << "    return retval;\n";
        } else {
            wrapper << "    return std::move(retval);\n";
        }
    }
    wrapper << "    } else {\n";
    wrapper << "    ";
    if (hasReturnType(methodReturnType, isConstructor, isDestructor)) {
        wrapper << methodReturnType << " retval = ";
    }
    wrapper << "f(";
    multiple = false;
    if (!methodStatic && className.size() > 0) {
        //if (!isConstructor) {
            wrapper << "this";
            multiple = true;
        //}
    }
    for (auto p : parameterNames) {
        if (multiple) { wrapper << ", "; }
        wrapper << p;
        multiple = true;
    }
    wrapper << ");\n";
    if (hasReturnType(methodReturnType, isConstructor, isDestructor)) {
        if(typeIsAddress(methodReturnType) ||
           typeIsReference(methodReturnType) ||
           !hasMovableReturnType(methodReturnType)) {
            wrapper << "    return retval;\n";
        } else {
            wrapper << "    return std::move(retval);\n";
        }
    }
    wrapper << "    }\n";
    wrapper << "}\n\n";
}

std::vector<std::string> getInstantiations() {
    auto types = configuration[template_types];
    size_t size = types.size();
    std::vector<std::string> currentTypes;
    for (size_t i = 0 ; i < size ; i++) {
        std::string tmp{types[i]};
        currentTypes.push_back(tmp);
    }
    return currentTypes;
}

void writeTemplate(
    std::vector<std::string> namespaceName,
    std::vector<std::string> className,
    std::vector<std::vector<std::string>> classTemplates,
    std::string methodName,
    std::string methodMangled,
    std::string methodReturnType,
    std::string methodType,
    bool methodStatic,
    std::vector<std::string> parameterNames,
    std::vector<std::string> parameterTypes,
    std::vector<std::string> templateTypes,
    bool modifyName = true) {
    //std::cout << __func__ << std::endl;
    /* get the template instantiation types to be tried */
    static auto currentTypes = getInstantiations();
    static size_t numTypes = currentTypes.size();

    size_t numTemplates = templateTypes.size();
    for (auto ct : classTemplates) {
        numTemplates += ct.size();
    }
    WRAP_ASSERT(numTemplates > 0);
    std::vector<size_t> indexes(numTemplates,0);
    bool done{false};
    while (!done) {
        size_t numSpecializations = 0;
        if (templateTypes.size() > 0) {
            numSpecializations++;
        }
        /* do replacements */
        // convert the class names
        size_t i = 0;
        std::vector<std::string> newClassName;
        size_t numClass = className.size();
        for (size_t j = 0 ; j < numClass ; j++) {
            std::stringstream ss;
            ss << className[j];
            if (classTemplates[j].size() > 0) {
                numSpecializations++;
                auto delimiter = "<";
                for (auto t : classTemplates[j]) {
                    ss << delimiter << currentTypes[indexes[i]];
                    delimiter = ", ";
                    i++;
                }
                ss << ">";
            }
            std::string tmp{ss.str()};
            newClassName.push_back(tmp);
        }

        // convert the function name
        std::stringstream newName;
        if (modifyName) {
            newName << methodName;
            auto delimiter = "<";
            for (auto t : templateTypes) {
                newName << delimiter << currentTypes[indexes[i]];
                delimiter = ", ";
                i++;
            }
            newName << ">";
        } else {
            newName << methodName;
        }

        /* Debug output */

/*
        for (auto c : newClassName) {
            std::cout << c << "::";
        }
        std::cout << newName.str() << std::endl;
*/

        /* update the types */

        i = 0;
        std::string newReturnType{methodReturnType};
        std::vector<std::string> newTypes{parameterTypes};
        for (size_t j = 0 ; j < numClass ; j++) {
            if (classTemplates[j].size() > 0) {
                for (auto t : classTemplates[j]) {
                    // replace_all...
                    bool changed = replace_all(newReturnType, t, currentTypes[indexes[i]]);
                    for (size_t z = 0 ; z < newTypes.size() ; z++) {
                        std::string np{newTypes[z]};
                        replace_all(np, t, currentTypes[indexes[i]]);
                        newTypes[z] = np;
                    }
                    // if there aren't any arguments, then we must
                    // have a template return value.  Assume we do.
                    // otherwise we won't have enough "template <>"
                    if ((changed ||
                         (contains(newReturnType, "<") &&
                          contains(newReturnType, ">"))) &&
                        (newTypes.size() < 0)) {
                        numSpecializations++;
                    }
                    i++;
                }
            }
            // save the new name
        }
        WRAP_ASSERT(numSpecializations > 0);

        // convert parameter template types
        for (auto t : templateTypes) {
            // replace_all...
            replace_all(newReturnType, t, currentTypes[indexes[i]]);
            for (size_t z = 0 ; z < newTypes.size() ; z++) {
                std::string np{newTypes[z]};
                replace_all(np, t, currentTypes[indexes[i]]);
                newTypes[z] = np;
            }
            i++;
        }

        /* process the instantiation */

        writeMethod(
            namespaceName,
            newClassName, // instantiated
            newName.str(), // instantiated
            methodMangled,
            newReturnType, // instantiated
            methodType,
            methodStatic,
            parameterNames,
            newTypes, // instantiated
            false, // is constructor
            false, // is destructor
            numSpecializations); // is template

        /* Advance to next instantiation permutation */

        indexes[0]++;
        // carry over to the next template variable
        for (size_t z = 0 ; z < indexes.size() ; z++) {
            // if we've reached the last option, carry over
            if (indexes[z] == numTypes) {
                // ...but only if there's another variable
                if (z < indexes.size() - 1) {
                    indexes[z+1]++;
                    indexes[z] = 0;
                }
            }
        }
        /* time to quit? */
        if (indexes[numTemplates-1] == numTypes) {
            done = true;
        }
    }
}

std::string getCursorKindName( CXCursorKind cursorKind )
{
  CXString kindName  = clang_getCursorKindSpelling( cursorKind );
  std::string result = clang_getCString( kindName );

  clang_disposeString( kindName );
  return result;
}

std::string getCursorName( CXCursor cursor )
{
  CXString cursorSpelling = clang_getCursorSpelling( cursor );
  std::string result      = clang_getCString( cursorSpelling );

  clang_disposeString( cursorSpelling );
  return result;
}

std::string getCursorType( CXCursor cursor )
{
  CXType cursorType = clang_getCursorType( cursor );
  CXString cursorString = clang_getTypeSpelling( cursorType );
  std::string result      = clang_getCString( cursorString );

  clang_disposeString( cursorString );
  return result;
}

bool getCursorStatic( CXCursor cursor )
{
  CX_StorageClass storageClass = clang_Cursor_getStorageClass( cursor );
  if (storageClass == CX_SC_Static) { return true; }
  return false;
}

std::string getCursorMangled( CXCursor cursor )
{
    CXString cursorString = clang_Cursor_getMangling( cursor );
    std::string result      = clang_getCString( cursorString );
    clang_disposeString( cursorString );
    return result;
}

std::string getCursorReturnType( CXCursor cursor )
{
  //CXType cursorType = clang_getCursorType( cursor );
  //CXType resultType = clang_getResultType( cursorType );
  CXType resultType = clang_getCursorResultType( cursor );
  CXString cursorString = clang_getTypeSpelling( resultType );
  std::string result      = clang_getCString( cursorString );

  clang_disposeString( cursorString );
  return result;
}

std::string validateReturnType(CXCursor c, std::vector<std::string> namespaceNames,
    std::string methodName, std::string methodReturnType) {
    if (methodReturnType.compare("int") != 0) {
        return methodReturnType;
    }
    // get translation unit
    CXTranslationUnit unit = clang_Cursor_getTranslationUnit(c);
    // get line of location
    CXFile file;
    unsigned line;
    unsigned column;
    unsigned offset;
    // get Range
    CXSourceRange range = clang_getCursorExtent(c);
    CXSourceLocation start = clang_getRangeStart(range);
    clang_getExpansionLocation(start, &file, &line, &column, &offset);
    //std::cout << "start: " << line << "," << column << "," << offset << std::endl;
    CXSourceLocation end = clang_getRangeEnd(range);
    clang_getExpansionLocation(end, &file, &line, &column, &offset);
    //std::cout << "end: " << line << "," << column << "," << offset << std::endl;
    // get Tokens
    CXToken * tokens;
    unsigned int numTokens;
    clang_tokenize(unit, range, &tokens, &numTokens);
    // iterate over the tokens
    std::stringstream ss;
    for (unsigned int i = 0; i < numTokens; i++) {
        CXString tokenString = clang_getTokenSpelling(unit, tokens[i]);
        std::string result = clang_getCString( tokenString );
        clang_disposeString( tokenString );
        if (result.compare(methodName) == 0) {
            if (methodReturnType.compare(ss.str()) == 0) {
                return methodReturnType;
            }
            std::stringstream fcn;
            for (auto n : namespaceNames) {
                fcn << n << "::";
            }
            fcn << ss.str();
            if (methodReturnType.compare(fcn.str()) == 0) {
                std::string foundtype{fcn.str()};
                return foundtype;
            }
            std::string foundtype{ss.str()};
            std::cout << "changing return type! " << methodReturnType << " to " << foundtype << std::endl;
            return foundtype;
        }
        ss << result;
    }
    return methodReturnType;
}

bool isMethodDeleted(CXCursor c) {
    // get translation unit
    CXTranslationUnit unit = clang_Cursor_getTranslationUnit(c);
    // get line of location
    CXFile file;
    unsigned line;
    unsigned column;
    unsigned offset;
    // get Range
    CXSourceRange range = clang_getCursorExtent(c);
    CXSourceLocation start = clang_getRangeStart(range);
    clang_getExpansionLocation(start, &file, &line, &column, &offset);
    //std::cout << "start: " << line << "," << column << "," << offset << std::endl;
    CXSourceLocation end = clang_getRangeEnd(range);
    clang_getExpansionLocation(end, &file, &line, &column, &offset);
    //std::cout << "end: " << line << "," << column << "," << offset << std::endl;
    // get Tokens
    CXToken * tokens;
    unsigned int numTokens;
    clang_tokenize(unit, range, &tokens, &numTokens);
    unsigned int foundEquals{numTokens+1};
    // iterate over the tokens, looking for "delete" or "default"
    for (unsigned int i = 0; i < numTokens; i++) {
        CXString tokenString = clang_getTokenSpelling(unit, tokens[i]);
        std::string result = clang_getCString( tokenString );
        clang_disposeString( tokenString );
        if (result.find("=") != std::string::npos) {
            foundEquals = i;
        }
        if (result.find("delete") != std::string::npos) {
            //std::cout << getCursorName(c) << "...deleted, ignoring" << std::endl;
            return true;
        }
        if (result.find("default") != std::string::npos) {
            //std::cout << getCursorName(c) << "...defaulted, ignoring" << std::endl;
            return true;
        }
    }
    return false;
}

bool parseUsingAlias(CXCursor c) {
    // get translation unit
    CXTranslationUnit unit = clang_Cursor_getTranslationUnit(c);
    // get line of location
    CXFile file;
    unsigned line;
    unsigned column;
    unsigned offset;
    // get Range
    CXSourceRange range = clang_getCursorExtent(c);
    CXSourceLocation start = clang_getRangeStart(range);
    clang_getExpansionLocation(start, &file, &line, &column, &offset);
    //std::cout << "start: " << line << "," << column << "," << offset << std::endl;
    CXSourceLocation end = clang_getRangeEnd(range);
    clang_getExpansionLocation(end, &file, &line, &column, &offset);
    //std::cout << "end: " << line << "," << column << "," << offset << std::endl;
    // get Tokens
    CXToken * tokens;
    unsigned int numTokens;
    clang_tokenize(unit, range, &tokens, &numTokens);
    bool foundEquals{false};
    // iterate over the tokens, looking for "delete" or "default"
    std::stringstream ss;
    std::stringstream ss2;
    for (unsigned int i = 0; i < numTokens; i++) {
        CXString tokenString = clang_getTokenSpelling(unit, tokens[i]);
        std::string result = clang_getCString( tokenString );
        clang_disposeString( tokenString );
        if (result.find("using") != std::string::npos) {
            continue;
        }
        if (result.find("=") != std::string::npos) {
            foundEquals = true;
            continue;
        }
        if (!foundEquals) {
            ss << result << _space;
        } else {
            ss2 << result << _space;
        }
    }
    std::string alias{ss.str()};
    trim(alias);
    std::string expanded{ss2.str()};
    trim(expanded);
    replace_all(expanded, " :: ", "::");
    replace_all(expanded, " < ", "<");
    replace_all(expanded, " >", ">");
    replace_all(expanded, " > ", ">");
    replace_all(expanded, " , ", ", ");
    //std::cout << std::endl << "Alias: " << alias << " = " << expanded << std::endl;
    aliasMap.insert(std::pair<std::string,std::string>(alias,expanded));
    return false;
}

class CursorLog {
public:
    std::ofstream _log;
    CursorLog() {
        _log.open("cursor.log");
        std::cout << "Writing the parser log to cursor.log" << std::endl;
    }
    ~CursorLog() {
        _log.close();
    }
};

class ASTState {
public:
    ASTState() : depth(0), inNamespace(false), inMethod(false),
        inClassTemplate(false), inFunctionTemplate(false) {};
    size_t depth;
    size_t inNamespace;
    std::vector<std::string> namespaceName;
    std::vector<std::string> className;
    std::vector<std::string> parameterNames;
    std::vector<std::string> parameterTypes;
    std::vector<std::vector<std::string>> classTemplates;
    std::vector<std::string> functionTemplates;
    bool inMethod;
    bool inClassTemplate;
    bool inFunctionTemplate;
    bool templateInstantiated;
};

std::string getCursorFileLocation(CXCursor c) {
    CXSourceLocation location = clang_getCursorLocation(c);
    CXFile file;
    unsigned line;
    unsigned column;
    unsigned offset;
    clang_getExpansionLocation(location, &file, &line, &column, &offset);
    CXString cursorString = clang_getFileName( file );
    std::string filename = clang_getCString( cursorString );
    clang_disposeString( cursorString );
    std::stringstream ss;
    ss << " " << filename << ", line: " << line << ", column: " << column;
    std::string tmp{ss.str()};
    return tmp;
}

void printCursor(ASTState* state, CXCursorKind kind, CXCursor c) {
    static CursorLog log;
    if (state->inNamespace) {
        log._log << std::string(state->depth, '-');
        log._log << getCursorKindName(kind)
                  << ": "
                  << getCursorName(c);
        if(getCursorType(c).size() > 0) {
            log._log << ", type: ["
                     << getCursorType(c)
                     << "]";
        }
        log._log << getCursorFileLocation(c) << std::endl;
    }
    std::cout << "." << std::flush;
}

CXChildVisitResult traverse(CXCursor c, CXCursor parent, CXClientData clientData);

void handleNamespace(CXCursor c, CXCursorKind kind, ASTState* state) {
    if ((getCursorName(c) == mainNamespace)) {
        state->inNamespace = true;
    }
    if (state->inNamespace) {
        wrapper << "namespace " << getCursorName(c) << " {\n\n";
        state->namespaceName.push_back(getCursorName(c));
        printCursor(state, kind, c);
        clang_visitChildren(c, traverse, state);
        if ((getCursorName(c) == mainNamespace)) {
            state->inNamespace = false;
        }
        wrapper << "} // end namespace "
                << getCursorName(c) << "\n\n";
        state->namespaceName.pop_back();
    }
}

bool skipThisClass(ASTState* state) {
    static auto classes = configuration[skip_classes];
    size_t size = classes.size();
    std::stringstream ss;
    std::string delimiter{""};
    for (auto c : state->namespaceName) {
        ss << delimiter << c;
        delimiter = "::";
    }
    for (auto c : state->className) {
        ss << delimiter << c;
    }
    std::string fullName{ss.str()};
    std::cout << std::endl << "Found Class: " << fullName;
    for (size_t i = 0 ; i < size ; i++) {
        std::string tmp{classes[i]};
        if (fullName.compare(tmp) == 0) {
            std::cout << " (skipped)";
            return true;
        }
    }
    return false;
}

bool skipThisMethod(ASTState* state, std::string methodName) {
    static auto classes = configuration[skip_methods];
    size_t size = classes.size();
    std::stringstream ss;
    for (auto c : state->namespaceName) {
        ss << c << "::";
    }
    for (auto c : state->className) {
        ss << c << "::";
    }
    ss << methodName;
    std::string fullName{ss.str()};
    //std::cout << fullName << std::endl;
    for (size_t i = 0 ; i < size ; i++) {
        std::string tmp{classes[i]};
        if (fullName.compare(tmp) == 0) {
            std::cout << std::endl << " " << fullName << "() (skipped)";
            return true;
        }
    }
    return false;
}

void handleClass(CXCursor c, CXCursorKind kind, ASTState* state) {
    state->className.push_back(getCursorName(c));
    std::vector<std::string> classTemplates;
    state->classTemplates.push_back(classTemplates);
    printCursor(state, kind, c);
    if (!skipThisClass(state)) {
        clang_visitChildren(c, traverse, state);
    }
    state->className.pop_back();
    state->classTemplates.pop_back();
}

void handleClassTemplate(CXCursor c, CXCursorKind kind, ASTState* state) {
    state->inClassTemplate = true;
    state->className.push_back(getCursorName(c));
    std::vector<std::string> classTemplates;
    state->classTemplates.push_back(classTemplates);
    printCursor(state, kind, c);
    if (!skipThisClass(state)) {
        clang_visitChildren(c, traverse, state);
    }
    state->className.pop_back();
    state->inClassTemplate = false;
    state->classTemplates.pop_back();
}

void handleMethod(CXCursor c, CXCursorKind kind, ASTState* state, bool isConstructor, bool isDestructor) {
    std::string methodName = getCursorName(c);
    std::string methodMangled = getCursorMangled(c);
    std::string methodType = getCursorType(c);
    std::string methodReturnType = getCursorReturnType(c);
    bool methodStatic = getCursorStatic(c);
    state->inMethod = true;
    state->parameterNames.clear();
    state->parameterTypes.clear();
    state->functionTemplates.clear();

    if (!skipThisMethod(state, methodName)) {
        printCursor(state, kind, c);
        wrapper << "/* " << getCursorFileLocation(c) << " */" << std::endl;
        clang_visitChildren(c, traverse, state);
        if (state->inClassTemplate) {
            //methodReturnType = validateReturnType(c, state->namespaceName, methodName, methodReturnType);
            writeTemplate(
                state->namespaceName,
                state->className,
                state->classTemplates,
                methodName,
                methodMangled,
                methodReturnType,
                methodType,
                methodStatic,
                state->parameterNames,
                state->parameterTypes,
                state->functionTemplates,
                false);
        } else {
            //methodReturnType = validateReturnType(c, state->namespaceName, methodName, methodReturnType);
            writeMethod(
                state->namespaceName,
                state->className,
                methodName,
                methodMangled,
                methodReturnType,
                methodType,
                methodStatic,
                state->parameterNames,
                state->parameterTypes,
                isConstructor,
                isDestructor,
                0); // num template specializations
        }
    }
    state->inMethod = false;
    state->parameterNames.clear();
    state->parameterTypes.clear();
    state->functionTemplates.clear();
}

void handleFunctionTemplate(CXCursor c, CXCursorKind kind, ASTState* state) {
    std::string methodName = getCursorName(c);
    std::string methodMangled = getCursorMangled(c);
    std::string methodType = getCursorType(c);
    std::string methodReturnType = getCursorReturnType(c);
    bool methodStatic = getCursorStatic(c);
    state->inMethod = true;
    state->inFunctionTemplate = true;
    state->parameterNames.clear();
    state->parameterTypes.clear();
    state->functionTemplates.clear();

    if (!skipThisMethod(state, methodName)) {
        printCursor(state, kind, c);
        wrapper << "/* " << getCursorFileLocation(c) << " */" << std::endl;
        clang_visitChildren(c, traverse, state);
        writeTemplate(
            state->namespaceName,
            state->className,
            state->classTemplates,
            methodName,
            methodMangled,
            methodReturnType,
            methodType,
            methodStatic,
            state->parameterNames,
            state->parameterTypes,
            state->functionTemplates);
    }
    state->inMethod = false;
    state->inFunctionTemplate = false;
    //std::cout << "end template: " << getCursorName(c) << std::endl;
    state->parameterNames.clear();
    state->parameterTypes.clear();
    state->functionTemplates.clear();
}

CXChildVisitResult traverse(CXCursor c, CXCursor parent, CXClientData clientData)
{
    ASTState* state = (ASTState*)(clientData);
    state->depth++;

    CXCursorKind kind = clang_getCursorKind(c);
    //printCursor(state, kind, c);

    /* Are we in a system header? */
    CXModule module = clang_Cursor_getModule(c);
    if (clang_Module_isSystem(module) > 0) {
        // ignore this cursor
        state->depth--;
        return CXChildVisit_Continue;
    }

    if (kind == CXCursorKind::CXCursor_Namespace) {
        handleNamespace(c, kind, state);
    }

    if (!state->inNamespace) {
        // ignore this cursor
        state->depth--;
        return CXChildVisit_Continue;
    }

    if (kind == CXCursorKind::CXCursor_ClassDecl) {
        handleClass(c, kind, state);
    }
    else if (kind == CXCursorKind::CXCursor_ClassTemplate) {
        handleClassTemplate(c, kind, state);
    }
    else if (kind == CXCursorKind::CXCursor_CXXMethod) {
        if (isMethodDeleted(c)) {
            state->depth--;
            return CXChildVisit_Continue;
        }
        handleMethod(c, kind, state, false, false);
    }
    else if (kind == CXCursorKind::CXCursor_Constructor) {
        if (isMethodDeleted(c)) {
            state->depth--;
            return CXChildVisit_Continue;
        }
        handleMethod(c, kind, state, true, false);
    }
    else if (kind == CXCursorKind::CXCursor_Destructor) {
        if (isMethodDeleted(c)) {
            state->depth--;
            return CXChildVisit_Continue;
        }
        handleMethod(c, kind, state, false, true);
    }
    else if (kind == CXCursorKind::CXCursor_FunctionTemplate) {
        handleFunctionTemplate(c, kind, state);
    }
    else if (kind == CXCursorKind::CXCursor_ParmDecl && state->inMethod && state->inNamespace) {
        state->parameterNames.push_back(getCursorName(c));
        state->parameterTypes.push_back(getCursorType(c));
    }
    else if (kind == CXCursorKind::CXCursor_TemplateTypeParameter) {
        // handle "enable_if" cases
        if (getCursorName(c) != "Enable") {
            if (state->inFunctionTemplate) {
                state->functionTemplates.push_back(getCursorName(c)); // or type, same thing
            } else {
                size_t index = state->className.size() - 1;
                state->classTemplates[index].push_back(getCursorName(c)); // or type, same thing
            }
            printCursor(state, kind, c);
        }
    }
    else if (kind == CXCursorKind::CXCursor_FunctionDecl && state->inNamespace) {
        printCursor(state, kind, c);
    }
    else if (kind == CXCursorKind::CXCursor_TypeAliasDecl) {
        printCursor(state, kind, c);
        parseUsingAlias(c);
    } else {
        printCursor(state, kind, c);
    }

    state->depth--;
    return CXChildVisit_Continue;
}

void printDiagnostics(CXTranslationUnit translationUnit){
    int nbDiag = clang_getNumDiagnostics(translationUnit);
    printf("There are %i diagnostics:\n",nbDiag);

    unsigned int currentDiag = 0;
    bool foundError = false;
    for (currentDiag = 0; currentDiag < nbDiag; ++currentDiag) {
        CXDiagnostic diagnotic = clang_getDiagnostic(translationUnit, currentDiag);
        CXString errorString = clang_formatDiagnostic(diagnotic,clang_defaultDiagnosticDisplayOptions());
        std::string tmp{clang_getCString(errorString)};
        clang_disposeString(errorString);
        if (tmp.find("error:") != std::string::npos) {
            foundError = true;
        }
        std::cerr << tmp << std::endl;
    }
    if (currentDiag > 0 && foundError) {
        std::cerr << "Please resolve these issues and try again." <<std::endl;
        exit(-1);
    }
}

void parse_header(const std::string& filename) {
    if( access( filename.c_str(), F_OK ) != 0 ) {
        // file doesn't exist
        std::cerr << "Error: " << filename << " not found." << std::endl;
        exit(-1);
    }
    CXIndex index = clang_createIndex(1,0);
    if (configuration.count(parser_flags) == 0) {
        std::cerr << "Configuration has no parser flags specified!" << std::endl;
        abort();
    }
    auto flags = configuration[parser_flags];
    size_t size = flags.size();
    auto arguments = new char*[size];
    for (size_t i = 0 ; i < size ; i++) {
        std::string flag{flags[i]};
        arguments[i] = strdup(flag.c_str());
    }
    CXTranslationUnit unit = clang_parseTranslationUnit(
            index,
            filename.c_str(),
            arguments,
            size,
            nullptr,
            0,
            CXTranslationUnit_None);
    printDiagnostics(unit);
    if (unit == nullptr) {
        std::cerr << "Unable to parse translation unit. Quitting." << std::endl;
        exit(-1);
    }

    CXCursor cursor = clang_getTranslationUnitCursor(unit);
    ASTState state;
    clang_visitChildren(cursor, traverse, &state);
}

void parse_symbols(std::string libname) {
    if( access( libname.c_str(), F_OK ) != 0 ) {
        // file doesn't exist
        std::cerr << "Error: " << libname << " not found." << std::endl;
        exit(-1);
    }
    std::ofstream symbolLog;
    symbolLog.open("symbol.log", std::fstream::out | std::fstream::app);
    std::cout << "Writing the library symbol log to cursor.log" << std::endl;
    // call `nm libsecret.so | grep " [TW] " | grep "_Z"` and capture output
    std::stringstream ss;
    ss << "nm " << libname << R"( | grep " [TW] " | grep "_Z" | grep )" << mainNamespace;
    std::string command{ss.str()};
    std::cout << "Parsing symbols in namespace " << mainNamespace << " from library " << libname << std::endl;
    //std::cout << command << std::endl;
    FILE * symbols = popen(command.c_str(),"r");
    // iterate over results, and get symbol as last token
    char * line = nullptr;
    size_t len = 0;
    ssize_t read;
    while ((read = getline(&line, &len, symbols)) != -1) {
        std::string tmp{line};
        std::string last_element(tmp.substr(tmp.rfind(_space)));
        last_element.erase(remove_if(last_element.begin(), last_element.end(), isspace), last_element.end());
        symbolLog << last_element << std::endl;
        size_t buff_size = 128; // not long enough, but it'll get reallocated if necessary
        auto buf = reinterpret_cast<char*>(std::malloc(buff_size));
        int stat = 0;
        buf = abi::__cxa_demangle(last_element.c_str(), buf, &buff_size, &stat);
        if (stat != 0) {
            free(buf);
            continue;
        }
        std::string demangled(buf);
        free(buf);
        std::string needle{mainNamespace+"::"};
        // if this symbol isn't in our namespace, don't track it
        if (demangled.find(needle) == std::string::npos) {
            continue;
        }
        // fix strings, llvm uses -lc++ and gcc uses =lstdc++
        replace_all(demangled, c11_string, simple_string);
        replace_all(demangled, old_string, simple_string);
        replace_all(demangled, ompi_string, mpi_string);
        // count the number of arguments in the symbol.  trickier than it sounds.
        size_t count = 0;
        size_t inTemplate{0};
        bool inParens{false};
        size_t parenStart = 0;
        // first, check for zero
        size_t implicitVoid = demangled.find("()");
        size_t explicitVoid = demangled.find("(void)");
        if (implicitVoid == std::string::npos && explicitVoid == std::string::npos ) {
            count++; // at least one!
            for(size_t i = 0 ; i < demangled.size() ; i++) {
                // first check for template characters
                if (demangled[i] == '<') {
                    inTemplate++;
                } else if (demangled[i] == '>') {
                    inTemplate--;
                } else if (demangled[i] == '(') {
                    inParens = true;
                    parenStart = i;
                } else if (demangled[i] == ')') {
                    inParens = false;
                // if in the arguments, and not in a template, count the comma
                } else if (inParens && inTemplate == 0 && demangled[i] == ',') {
                    count++;
                }
            }
        }
        symbolData_t result;
        result.nArgs = count;
        result.mangledName = last_element;
        symbolLog<< " has " << count << " arguments" << std::endl;
        symbolLog << demangled << std::endl;
        symbolMap.insert(std::pair<std::string,symbolData_t>(demangled, result));
    }
    pclose(symbols);
    symbolLog.close();
    // demangle, and put the results into the map
}

/* -------------------------------------------------------------------------- */
/* -- Instrument the program using C, C++ or F90 instrumentation routines --- */
/* -------------------------------------------------------------------------- */
int main(int argc, char **argv)
{
    std::vector<std::string> libNames;
    std::string configFile("");

    if (argc < 2) {
        show_usage(argv[0]);
        return 1;
    }

    std::string headerName = argv[1];
    std::cout << "Header file to be parsed: " << argv[1] << std::endl;

    for(int i=1; i<argc; i++) {
        if (strcmp(argv[i], "-w") == 0) {
            libNames.push_back(std::string(argv[i+1]));
            std::cout << "Library to be wrapped: " << argv[i+1] << std::endl;
        }
        else if (strcmp(argv[i], "-n") == 0) {
            mainNamespace = std::string(argv[i+1]);
            std::cout << "Namespace to be wrapped: " << mainNamespace << std::endl;
        }
        else if (strcmp(argv[i], "-c") == 0) {
            configFile = std::string(argv[i+1]);
            std::cout << "Configuration file to be used: " << configFile << std::endl;
        }
    }

    readConfigFile(configFile);
    writePreamble(headerName, libNames);
    std::remove("symbols.log");
    for(auto lib : libNames) {
        parse_symbols(lib);
    }
    parse_header(headerName);
    wrapper.close();
    std::cout << std::endl;
    std::cout << "Wrote library wrapper to wr.cpp" << std::endl;
} /* end of main */

/* EOF */

