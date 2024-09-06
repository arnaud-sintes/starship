#include "../core/std.h"
#include "../core/Win32.h"
#include "../core/File.h"
#include "../core/Base64.h"
#include "../core/Sha1.h"
#include "../core/Packer.h"


std::string Tag( const std::string & _resourcePath )
{
    std::string tag;
    for( const auto & entry : std::filesystem::directory_iterator( _resourcePath ) )
        tag.append( File::Stat( entry.path().string() ) );
    return Sha1::Compute( tag );
}


void Write( std::string & _fileContent, const size_t _data )
{
    _fileContent.append( reinterpret_cast< const char * >( &_data ), sizeof( size_t ) );
}


void Write( std::string & _fileContent, const std::string & _data )
{
    Write( _fileContent, _data.size() );
    _fileContent.append( _data.c_str(), _data.size() );
}


int main( int _argc, char * _argv[] )
{
    Win32::ShowConsole( true );
    std::list< std::string > arguments;
	for( int i{ 1 }; i < _argc; ++i )
		arguments.emplace_back( _argv[ i ] );
    if( arguments.size() != 2 ) {
        std::cout << "[ERROR] usage is: packer resource_folder data_file_path" << std::endl;
        return -1;
    }
    std::cout << "packing '" << arguments.front() << "' folder content in '" << arguments.back() << "' data file..." << std::endl;

    std::cout << "checking for resource changes..." << std::endl;
    const auto filesTag{ Tag( arguments.front() ) };
    if( !std::filesystem::exists( arguments.back() ) ) {
        std::cout << "no existing data file." << std::endl;
    }
    else {
        std::cout << "a data file already exists." << std::endl;
        const auto data{ File::Load( arguments.back() ) };
        if( !data ) {
            std::cout << "[ERROR] while loading existing data file!" << std::endl;
            return -1;
        }
        const char * pData{ ( *data ).c_str() };
        if( filesTag == Packer::Read< std::string >( pData ) ) {
            std::cout << "no change detected." << std::endl;
            return 0;
        }
    }

    std::string fileContent;
    Write( fileContent, filesTag );
    Write( fileContent, static_cast< size_t >( std::distance( std::filesystem::directory_iterator( arguments.front() ), std::filesystem::directory_iterator{} ) ) );
    for( const auto & entry : std::filesystem::directory_iterator( arguments.front() ) ) {
        std::cout << "packing resource '" << entry.path().filename().string() << "'..." << std::endl;
        Write( fileContent, entry.path().filename().string() );
        const auto data{ File::Load( entry.path().string() ) };
        if( !data ) {
            std::cout << "[ERROR] while packing resource!" << std::endl;
            return -1;
        }
        Write( fileContent, Base64::Encode( *data ) );
    }

    std::cout << "writing data file..." << std::endl;
    if( !File::Save( arguments.back(), fileContent ) ) {
        std::cout << "[ERROR] while writing data file!" << std::endl;
        return -1;
    }
    std::cout << "done." << std::endl;
    return 0;
}