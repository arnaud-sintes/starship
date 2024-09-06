#include "File.h"


std::optional< std::string > File::Load( const std::string & _filePath )
{
    std::ifstream srcStream;
	srcStream.exceptions( srcStream.exceptions() | std::ifstream::failbit );
    try {
		srcStream.open( _filePath, std::ifstream::binary );
	}
	catch( std::ios_base::failure & ) {
		return {};
	}

	std::filebuf * pFileBuffer{ srcStream.rdbuf() };
	const auto size{ pFileBuffer->pubseekoff( 0, srcStream.end, srcStream.in ) };
	pFileBuffer->pubseekpos( 0, srcStream.in );
	std::string fileData;
	fileData.resize( size );
	if( pFileBuffer->sgetn( reinterpret_cast< char * >( fileData.data() ), size ) != size )
		return {};
	return { std::move( fileData ) };
}


bool File::Save( const std::string & _filePath, const std::string & _data )
{
    std::ofstream dstStream;
	dstStream.exceptions( dstStream.exceptions() | std::ifstream::failbit );
    try
    {
        dstStream.open( _filePath, std::ofstream::binary | std::ofstream::trunc );
    }
    catch( std::ios_base::failure & )
    {
        return false;
    }

    dstStream.write( const_cast< char * >( reinterpret_cast< const char * >( _data.data() ) ), _data.size() );
    return true;
}


std::string File::Stat( const std::string & _filePath )
{
    struct ::_stat64 status;
	::_stat64( _filePath.c_str(), &status );
	std::stringstream ss;
	ss << std::hex << status.st_mtime << status.st_size;
	return ss.str();
}