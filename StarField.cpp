#include "StarField.h"


StarField::StarField( const size_t _windowWidth, const size_t _windowHeight )
    : m_windowWidth{ _windowWidth }
    , m_windowHeight{ _windowHeight }
    , m_rnd{ m_rndDev() }
    , m_dimension{ ( _windowWidth > _windowHeight ? _windowWidth : _windowHeight ) + 200 } // margin
    , m_rndPos{ 0, static_cast< unsigned int >( m_dimension ) }
{
    std::vector< Layer > layers{
        // d     speed   color        radius
        { 4000,  1,  2,  0.75, 1,     0.25, 0.5  },
        { 1000,  3,  5,  0.5,  1,     0.5,  0.75 },
        { 300,   5,  8,  0.75, 1,     0.75, 1    },
        { 50,    8, 21,  0.7,  0.9,   1,    1.5  },
    };
    for( auto & layer : layers )
        for( int d{ 0 }; d < layer.density; d++ ) {
            const double x{ static_cast< double >( m_rndPos( m_rnd ) ) };
            const double y{ static_cast< double >( m_rndPos( m_rnd ) ) };
            std::uniform_int_distribution< std::mt19937::result_type > speed{ static_cast< unsigned int >( layer.speedMin * 100 ), static_cast< unsigned int >( layer.speedMax * 100 ) };
            std::uniform_int_distribution< std::mt19937::result_type > color{ static_cast< unsigned int >( layer.colorMin * 100 ), static_cast< unsigned int >( layer.colorMax * 100 ) };
            std::uniform_int_distribution< std::mt19937::result_type > radius{ static_cast< unsigned int >( layer.radiusMin * 100 ), static_cast< unsigned int >( layer.radiusMax * 100 ) };
            m_field.emplace_back( Star{ x, y,
                static_cast< double >( speed( m_rnd ) ) / 100,
                static_cast< double >( color( m_rnd ) ) / 100,
                static_cast< double >( radius( m_rnd ) ) / 100 } );
        }
}


void StarField::Draw( cairo_t & _cairo, const Vector & _speed )
{
    const double hzSpeedFactor{ 0.1 };
    for( auto & star : m_field ) {
        cairo_set_source_rgb( &_cairo, star.c, star.c, 1 );
        cairo_arc( &_cairo, star.x - ( m_dimension >> 1 ) + ( m_windowWidth >> 1 ), star.y - ( m_dimension >> 1 ) + ( m_windowHeight >> 1 ), star.radius, 0, 2 * 3.1415 );
        cairo_fill( &_cairo );
        star.x += ( -star.speed * ( _speed.u * hzSpeedFactor ) );
        star.y += ( -star.speed * ( _speed.v * hzSpeedFactor ) );
        if( _speed.u < 0 && star.x >= m_dimension ) { star.x = 0; star.y = m_rndPos( m_rnd ); }
        if( _speed.u > 0 && star.x < 0 ) { star.x = static_cast< double >( m_dimension - 1 ); star.y = m_rndPos( m_rnd ); }
        if( _speed.v < 0 && star.y >= m_dimension ) { star.y = 0; star.x = m_rndPos( m_rnd ); }
        if( _speed.v > 0 && star.y < 0 ) { star.y = static_cast< double >( m_dimension - 1 ); star.x = m_rndPos( m_rnd ); }
    }
}