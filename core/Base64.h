#include "std.h"

struct Base64
{
	static std::string Encode( const std::string & _data );
	static std::string Decode( const std::string & _encodedData );
};