#include "UpdateChecker.h"

#include "core/Win32.h"


namespace
{
    constexpr const char * repository{ "arnaud-sintes/starship" };

    // the tag comes from the network: nothing but the characters of a version goes any further
    bool IsVersionLike( const std::string & _text )
    {
        return !_text.empty() && _text.size() <= 24 && std::isdigit( static_cast< unsigned char >( _text.at( _text.front() == 'v' && _text.size() > 1 ? 1 : 0 ) ) )
            && std::ranges::all_of( _text, []( const char _c ) { return std::isalnum( static_cast< unsigned char >( _c ) ) || _c == '.' || _c == '-'; } );
    }

    std::vector< int > Numbers( const std::string & _version )
    {
        std::vector< int > numbers;
        std::istringstream stream{ _version.starts_with( 'v' ) ? _version.substr( 1 ) : _version };
        for( std::string item; std::getline( stream, item, '.' ); ) {
            const auto digits{ item.substr( 0, item.find_first_not_of( "0123456789" ) ) };
            numbers.emplace_back( digits.empty() ? 0 : std::stoi( digits ) );
        }
        return numbers;
    }
}


UpdateChecker::UpdateChecker( const std::string & _currentVersion )
    : m_currentVersion{ _currentVersion }
    , m_shared{ std::make_shared< Shared >() }
{
    std::thread{ [ shared = m_shared, current = _currentVersion ]() {
        const auto answer{ Win32::HttpsGet( L"api.github.com", std::wstring{ L"/repos/" } + std::wstring{ repository, repository + std::strlen( repository ) } + L"/releases/latest", L"Starship" ) };
        if( answer ) {
            // {"tag_name":"0.0.15", ...}: no need for a JSON parser to read one string
            const auto key{ answer->find( "\"tag_name\"" ) };
            const auto open{ key == std::string::npos ? key : answer->find( '"', answer->find( ':', key ) ) };
            const auto close{ open == std::string::npos ? open : answer->find( '"', open + 1 ) };
            if( close != std::string::npos ) {
                auto tag{ answer->substr( open + 1, close - open - 1 ) };
                if( IsVersionLike( tag ) && IsNewer( tag, current ) ) {
                    shared->latest = tag.starts_with( 'v' ) ? tag.substr( 1 ) : tag;
                    shared->newer = true;
                }
            }
        }
        shared->done = true;
    } }.detach();
}


std::string UpdateChecker::Url() const
{
    return std::format( "https://github.com/{}/releases/latest", repository );
}


bool UpdateChecker::IsNewer( const std::string & _version, const std::string & _than )
{
    return Numbers( _version ) > Numbers( _than ); // lexicographic: 0.0.15 > 0.0.14, 0.1 > 0.0.9, 0.0.14.1 > 0.0.14
}
