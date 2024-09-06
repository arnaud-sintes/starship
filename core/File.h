#include "std.h"

struct File
{
    static std::optional< std::string > Load( const std::string & _filePath );
    static bool Save( const std::string & _filePath, const std::string & _data );
    static std::string Stat( const std::string & _filePath );
};
