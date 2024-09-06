#include "Sha1.h"


#define RL( value, bits ) ( ( ( value ) << ( bits ) ) | ( ( ( value ) & 0xffffffff ) >> ( 32 - ( bits ) ) ) )
#define BL( i ) ( _block[ i & 15 ] = RL( _block[ ( i + 13 ) & 15 ] ^ _block[ ( i + 8 ) & 15 ] ^ _block[ ( i + 2 ) & 15 ] ^ _block[ i & 15 ], 1 ) )
#define R0( v, w, x, y, z, i ) z += ( ( w & ( x ^ y ) ) ^ y ) + _block[ i ] + 0x5a827999 + RL( v , 5 ); w = RL( w, 30 );
#define R1( v, w, x, y, z, i ) z += ( ( w & ( x ^ y ) ) ^ y ) + BL( i ) + 0x5a827999 + RL( v, 5 ); w = RL( w, 30 );
#define R2( v, w, x, y, z, i ) z += ( w ^ x ^ y ) + BL( i ) + 0x6ed9eba1 + RL( v, 5 ); w = RL( w, 30 );
#define R3( v, w, x, y, z, i ) z += ( ( ( w | x ) & y ) | ( w & x ) ) + BL( i ) + 0x8f1bbcdc + RL( v, 5 ); w = RL( w, 30 );
#define R4( v, w, x, y, z, i ) z += ( w ^ x ^ y ) + BL( i ) + 0xca62c1d6 + RL( v, 5 ); w = RL( w, 30 );


static void _Transform( unsigned long int * _block, unsigned long int * _digest, unsigned long long & _transforms )
{
    unsigned long int a{ _digest[ 0 ] }, b{ _digest[ 1 ] }, c{ _digest[ 2 ] }, d{ _digest[ 3 ] }, e{ _digest[ 4 ] };
    R0( a, b, c, d, e,  0 ); R0( e, a, b, c, d,  1 ); R0( d, e, a, b, c,  2 ); R0( c, d, e, a, b,  3 );
    R0( b, c, d, e, a,  4 ); R0( a, b, c, d, e,  5 ); R0( e, a, b, c, d,  6 ); R0( d, e, a, b, c,  7 );
    R0( c, d, e, a, b,  8 ); R0( b, c, d, e, a,  9 ); R0( a, b, c, d, e, 10 ); R0( e, a, b, c, d, 11 );
    R0( d, e, a, b, c, 12 ); R0( c, d, e, a, b, 13 ); R0( b, c, d, e, a, 14 ); R0( a, b, c, d, e, 15 );
    R1( e, a, b, c, d, 16 ); R1( d, e, a, b, c, 17 ); R1( c, d, e, a, b, 18 ); R1( b, c, d, e, a, 19 );
    R2( a, b, c, d, e, 20 ); R2( e, a, b, c, d, 21 ); R2( d, e, a, b, c, 22 ); R2( c, d, e, a, b, 23 );
    R2( b, c, d, e, a, 24 ); R2( a, b, c, d, e, 25 ); R2( e, a, b, c, d, 26 ); R2( d, e, a, b, c, 27 );
    R2( c, d, e, a, b, 28 ); R2( b, c, d, e, a, 29 ); R2( a, b, c, d, e, 30 ); R2( e, a, b, c, d, 31 );
    R2( d, e, a, b, c, 32 ); R2( c, d, e, a, b, 33 ); R2( b, c, d, e, a, 34 ); R2( a, b, c, d, e, 35 );
    R2( e, a, b, c, d, 36 ); R2( d, e, a, b, c, 37 ); R2( c, d, e, a, b, 38 ); R2( b, c, d, e, a, 39 );
    R3( a, b, c, d, e, 40 ); R3( e, a, b, c, d, 41 ); R3( d, e, a, b, c, 42 ); R3( c, d, e, a, b, 43 );
    R3( b, c, d, e, a, 44 ); R3( a, b, c, d, e, 45 ); R3( e, a, b, c, d, 46 ); R3( d, e, a, b, c, 47 );
    R3( c, d, e, a, b, 48 ); R3( b, c, d, e, a, 49 ); R3( a, b, c, d, e, 50 ); R3( e, a, b, c, d, 51 );
    R3( d, e, a, b, c, 52 ); R3( c, d, e, a, b, 53 ); R3( b, c, d, e, a, 54 ); R3( a, b, c, d, e, 55 );
    R3( e, a, b, c, d, 56 ); R3( d, e, a, b, c, 57 ); R3( c, d, e, a, b, 58 ); R3( b, c, d, e, a, 59 );
    R4( a, b, c, d, e, 60 ); R4( e, a, b, c, d, 61 ); R4( d, e, a, b, c, 62 ); R4( c, d, e, a, b, 63 );
    R4( b, c, d, e, a, 64 ); R4( a, b, c, d, e, 65 ); R4( e, a, b, c, d, 66 ); R4( d, e, a, b, c, 67 );
    R4( c, d, e, a, b, 68 ); R4( b, c, d, e, a, 69 ); R4( a, b, c, d, e, 70 ); R4( e, a, b, c, d, 71 );
    R4( d, e, a, b, c, 72 ); R4( c, d, e, a, b, 73 ); R4( b, c, d, e, a, 74 ); R4( a, b, c, d, e, 75 );
    R4( e, a, b, c, d, 76 ); R4( d, e, a, b, c, 77 ); R4( c, d, e, a, b, 78 ); R4( b, c, d, e, a, 79 ); 
    _digest[ 0 ] += a; _digest[ 1 ] += b; _digest[ 2 ] += c; _digest[ 3 ] += d; _digest[ 4 ] += e;
    _transforms++;
}


static void _BufferToBlock( const std::string & _buffer, unsigned long int * _block )
{
	for( size_t i( 0 ); i < 16; i++ )
		_block[ i ] = ( _buffer[ ( i << 2 ) + 3 ] & 0xff ) | ( _buffer[ ( i << 2 ) + 2 ] & 0xff ) << 8 | ( _buffer[ ( i << 2 ) + 1 ] & 0xff ) << 16 | ( _buffer[ ( i << 2 ) + 0 ] & 0xff ) << 24;
}


static void _Read( std::istream & _dataStream, std::string & _buffer, const size_t & _max )
{
    char * data{ new char[ _max ] };
	_dataStream.read( data, _max );
	_buffer.assign( data, unsigned int( _dataStream.gcount() ) );
	delete [] data;
}



std::string Sha1::Compute( const std::string & _data, const bool & _8bitsGrouping )
{
    std::istringstream dataStream{ _data };
	std::string buffer;
	_Read( dataStream, buffer, 64 - buffer.size() );
	unsigned long int digest[ 5 ] = { 0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0 };
	unsigned long long transforms( 0 );
	while( dataStream ) {
		unsigned long int block[ 16 ];
		_BufferToBlock( buffer, block );
		_Transform( block, digest, transforms );
		_Read( dataStream, buffer, 64 );
	}
    unsigned long long total{ ( ( transforms << 6 ) + buffer.size() ) << 3 };
    buffer += static_cast< unsigned char >( 0x80 );
    auto orig_size{ static_cast< unsigned int >( buffer.size() ) };
    while( buffer.size() < 64 )
        buffer += static_cast< char >( 0x00 ); 
    unsigned long int block[ 16 ];
	_BufferToBlock( buffer, block ); 
    if( orig_size > 64 - 8 ) {
		_Transform( block, digest, transforms );
        for( size_t i{ 0 }; i < 16 - 2; i++ )
            block[ i ] = 0;
    }
    block[ 16 - 1 ] = static_cast< unsigned long int >( total );
    block[ 16 - 2 ] = ( total >> 32 );
    _Transform( block, digest, transforms );
	if( !_8bitsGrouping ) {
        std::ostringstream resultStream;
        for( size_t i{ 0 }; i < 5; i++ )
            resultStream << std::hex << std::setfill( '0' ) << std::setw( 8 ) << ( digest[ i ] & 0xffffffff );
        return resultStream.str();
    }
    // convert SHA1 by grouping value as 8bits 0xFF values
    std::string result;
    for( size_t i{ 0 }; i < 5; i++ )
        for( size_t j{ 0 }; j < 4; j++ )
			result.push_back( reinterpret_cast< unsigned char * >( &digest[ i ] )[ 3 - j ] );
    return result;
}