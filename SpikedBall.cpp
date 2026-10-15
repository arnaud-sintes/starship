#include "SpikedBall.h"

#include <array>


namespace
{
    constexpr double lightShift{ -0.35 }; // the light comes from the upper left, whatever the ball does
    constexpr double perspective{ 6 };    // camera distance, in horn lengths

    struct Horn
    {
        Vector3 direction; // unit vector, already rotated
    };

    Color_d Shaded( const Color_d & _color, const double _depth ) // _depth: 0 (far) .. 1 (near)
    {
        const auto light{ 0.35 + 0.65 * _depth };
        return { _color.r * light, _color.g * light, _color.b * light, _color.a };
    }
}


void SpikedBall::Draw( const NanoVGRenderer::Frame & _frame, const Vector & _center, const Style & _style )
{
    const auto count{ std::min( _style.horns, maxHorns ) };
    std::array< Horn, maxHorns > horns;
    constexpr double goldenAngle{ 2.399963229728653 };
    for( int i{ 0 }; i < count; i++ ) { // fibonacci sphere: evenly spread, no crosshair symmetry
        const auto y{ 1 - 2 * ( i + 0.5 ) / count };
        const auto ring{ std::sqrt( 1 - y * y ) };
        horns[ static_cast< size_t >( i ) ].direction = Vector3{ std::cos( goldenAngle * i ) * ring, y, std::sin( goldenAngle * i ) * ring }
            .RotatedY( _style.yaw ).RotatedX( _style.pitch );
    }
    std::sort( horns.begin(), horns.begin() + count, []( const Horn & _a, const Horn & _b ) { return _a.direction.z < _b.direction.z; } );

    const auto project{ [ & ]( const Vector3 & _point ) {
        const auto distance{ _style.tipDistance * perspective };
        const auto scale{ distance / ( distance - _point.z ) };
        return std::pair{ Position_d{ _center.u + _point.x * scale, _center.v + _point.y * scale }, scale };
    } };

    const auto drawHorn{ [ & ]( const Horn & _horn ) {
        const auto depth{ ( _horn.direction.z + 1 ) * 0.5 };
        const auto [ base, baseScale ]{ project( _horn.direction * ( _style.bodyRadius - 2 ) ) };
        const auto [ tip, tipScale ]{ project( _horn.direction * _style.tipDistance ) };
        _frame.Line( base, tip, Shaded( _style.horn, depth ), _style.hornWidth * ( baseScale + tipScale ) * 0.5 );
        _frame.FillCircle( tip, _style.tipRadius * tipScale, Shaded( _style.tip, 0.4 + 0.6 * depth ) );
    } };

    // the far horns, behind the body:
    int next{ 0 };
    for( ; next < count && horns[ static_cast< size_t >( next ) ].direction.z < 0; next++ )
        drawHorn( horns[ static_cast< size_t >( next ) ] );

    // the body, with a fixed highlight for volume:
    _frame.FillCircle( _center, _style.bodyRadius, _style.body );
    _frame.GradientCircle( Position_d{ _center.u + _style.bodyRadius * lightShift, _center.v + _style.bodyRadius * lightShift }, _style.bodyRadius * 0.6,
        _style.highlight, Color_d{ _style.highlight.r, _style.highlight.g, _style.highlight.b, 0 } );

    // the near horns, in front of it:
    for( ; next < count; next++ )
        drawHorn( horns[ static_cast< size_t >( next ) ] );

    // the ember, only while on the visible face:
    if( _style.ember.a > 0 ) {
        const auto ember{ Vector3{ std::cos( _style.emberAngle ), 0, std::sin( _style.emberAngle ) }.RotatedX( 0.7 ) };
        if( ember.z > 0 )
            _frame.FillCircle( Position_d{ _center.u + ember.x * _style.bodyRadius * 0.85, _center.v + ember.y * _style.bodyRadius * 0.85 },
                _style.emberRadius * ( 0.6 + 0.4 * ember.z ), _style.ember );
    }
}
