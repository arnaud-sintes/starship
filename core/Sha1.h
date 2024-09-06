#include "std.h"


struct Sha1
{
    static std::string Compute( const std::string & _data, const bool & _8bitsGrouping = false );
};