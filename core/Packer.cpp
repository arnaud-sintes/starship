#include "Packer.h"

#include "File.h"
#include "Win32.h"
#include "Base64.h"



template<>
size_t Packer::Read( const char * &_pData )
{
    const auto length{ *reinterpret_cast< const size_t * >( _pData ) };
    _pData += sizeof( size_t );
    return length;
}


template<>
std::string Packer::Read( const char * &_pData )
{
    const auto length{ Read< size_t >( _pData ) };
    std::string data{ _pData, length };
    _pData += length;
    return data;
}


std::optional< std::unordered_map< std::string, std::string > > Packer::UnPack( const std::string & _dataFilePath )
{
    const auto data{ File::Load( _dataFilePath ) };
    if( !data )
        return {};
    const char * pData{ ( *data ).c_str() };
    Read< std::string >( pData ); // tag
    auto fileCount{ Read< size_t >( pData ) };
    std::unordered_map< std::string, std::string > resources;
    const auto tempFolder{ Win32::GetTemporaryFolder() };
    if( !tempFolder )
        return {};
    while( fileCount-- != 0 ) {
        const auto fileName{ Read< std::string >( pData ) };
        const auto filePath{ *tempFolder + fileName };
        if( !File::Save( filePath, Base64::Decode( Read< std::string >( pData ) ) ) )
            return {};
        resources.emplace( fileName, filePath );
    }
    return resources;
}