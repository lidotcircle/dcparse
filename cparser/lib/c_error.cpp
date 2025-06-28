#include "c_error.h"
using namespace std;
using namespace cparser;


CError::CError(const string& what) : runtime_error(what)
{}
CError::CError(const char* what) : runtime_error(what)
{}


#define EEntry(cname)                                                                              \
    cname::cname(const string& what) : CError(what)                                                \
    {}                                                                                             \
    cname::cname() : CError(#cname)                                                                \
    {}
CErrorList
#undef EEntry
