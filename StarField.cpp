#include "StarField.h"


StarField::StarField( const Dimension_ui & _dimension )
    : m_dimension{ _dimension }
    , m_rnd{ m_rndDev() }
    , m_maxDimension{ ( m_dimension.width > m_dimension.height ? m_dimension.width : m_dimension.height ) + 200 } // margin
    , m_rndPos{ 0, static_cast< unsigned int >( m_maxDimension ) }
{
    std::vector< Layer > layers{
        // d     speed   color        radius
        { 2000,  1,  2,  0.25, 0.5,   0.5,  0.75 },
        { 500,   3,  5,  0.25, 0.75,  0.75, 1    },
        { 150,   5,  8,  0.5,  0.75,  1,    1.5  },
        { 25,    8, 21,  0.5,  1,     1.5,  2    },
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


void StarField::Draw( const NanoVGRenderer::Frame & _frame, const Vector & _speed )
{
    const double hzSpeedFactor{ 0.1 };
    for( auto & star : m_field ) {
        _frame.FillCircle( { star.x - ( m_maxDimension >> 1 ) + ( m_dimension.width >> 1 ), star.y - ( m_maxDimension >> 1 ) + ( m_dimension.height >> 1 ) }, star.radius, { star.c, star.c, 1 } );
        star.x += ( -star.speed * ( _speed.u * hzSpeedFactor ) );
        star.y += ( -star.speed * ( _speed.v * hzSpeedFactor ) );
        if( _speed.u < 0 && star.x >= m_maxDimension ) { star.x = 0; star.y = m_rndPos( m_rnd ); }
        if( _speed.u > 0 && star.x < 0 ) { star.x = static_cast< double >( m_maxDimension - 1 ); star.y = m_rndPos( m_rnd ); }
        if( _speed.v < 0 && star.y >= m_maxDimension ) { star.y = 0; star.x = m_rndPos( m_rnd ); }
        if( _speed.v > 0 && star.y < 0 ) { star.y = static_cast< double >( m_maxDimension - 1 ); star.x = m_rndPos( m_rnd ); }
    }
}