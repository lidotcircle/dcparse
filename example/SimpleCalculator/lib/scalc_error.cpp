#include "scalc/scalc_error.h"
using namespace std;


SCalcError::SCalcError(const string& what): runtime_error(what) {}


#define EEntry(cname) \
    cname::cname(const string& what): SCalcError(what) {} \
    cname::cname(): SCalcError(#cname) {}
SCalcErrorList
#undef EEntry
