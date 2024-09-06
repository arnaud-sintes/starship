#pragma once
#include "std.h"


struct Packer
{
    using Resources = std::unordered_map< std::string, std::string >;
    static std::optional< Resources > UnPack( const std::string & _dataFilePath );

    template< typename _Type >
    static _Type Read( const char * &_pData );

    template<>
    size_t Read( const char * &_pData );

    template<>
    std::string Read( const char * &_pData );
};
