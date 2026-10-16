#include "SplashScreen.h"

#include "Mesh3D.h"


namespace
{
    constexpr double frameSeconds{ 1.0 / 60 };
    constexpr Color_d blue{ 0.35, 0.65, 1 };
    constexpr Color_d amber{ 1, 0.62, 0.2 };
    constexpr double Pi2{ 6.283185307179586 };

    double Hash( const double _a, const double _b )
    {
        const auto v{ std::sin( _a * 12.9898 + _b * 78.233 ) * 43758.5453 };
        return v - std::floor( v );
    }

    double EaseOut( const double _t ) // 0..1, fast then slow
    {
        const auto t{ std::clamp( _t, 0.0, 1.0 ) };
        return 1 - ( 1 - t ) * ( 1 - t ) * ( 1 - t );
    }
}


SplashScreen::SplashScreen( const Dimension_ui & _logical )
    : m_logical{ _logical }
{}


void SplashScreen::Draw( const NanoVGRenderer::Frame & _frame, const std::string & _version )
{
    m_time += frameSeconds;
    const auto t{ m_time };
    const Vector screen{ static_cast< double >( m_logical.width ), static_cast< double >( m_logical.height ) };
    const Vector center{ screen * 0.5 + Vector{ 0, -75 } };
    const auto settle{ EaseOut( t / 2.2 ) }; // 0 at the start, 1 once everything is in place

    // the hyperspace warp, slowing down to a drift: streaks flying out of the center
    const auto speed{ 0.05 + 1.1 * std::exp( -t * 1.1 ) };
    m_warp += speed * frameSeconds;
    {
        const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
        const auto reach{ std::sqrt( screen.DistanceSquared() ) * 0.6 };
        for( int i{ 0 }; i < 240; i++ ) {
            const auto phase{ std::fmod( Hash( i, 1 ) + m_warp * ( 0.6 + 0.8 * Hash( i, 2 ) ), 1.0 ) };
            const auto angle{ Hash( i, 3 ) * Pi2 };
            const auto length{ 0.01 + 0.16 * std::min( speed, 1.0 ) };
            const auto head{ reach * phase * phase }, tail{ reach * std::pow( std::max( phase - length, 0.0 ), 2 ) };
            const auto alpha{ std::pow( phase, 1.3 ) * ( 0.25 + 0.6 * Hash( i, 4 ) ) };
            const auto tint{ Hash( i, 5 ) };
            _frame.Line( center + Vector::From( angle, tail ), center + Vector::From( angle, head ), { 0.6 + 0.4 * tint, 0.8 + 0.2 * tint, 1, alpha }, 0.8 + 2 * phase );
        }
    }

    // the glow behind the title
    {
        const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
        _frame.GradientCircle( center, 520 * ( 0.6 + 0.4 * settle ), { blue.r, blue.g, blue.b, 0.22 * settle }, { blue.r, blue.g, blue.b, 0 } );
    }

    // the thin technical circles: a fine one, a dashed one and a ticked one, turning against each other
    {
        const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
        const auto reveal{ EaseOut( ( t - 0.3 ) / 1.6 ) };
        const auto scale{ 0.7 + 0.3 * reveal };
        const auto spin{ t * 0.5 };
        _frame.StrokeCircle( center, 235 * scale, { blue.r, blue.g, blue.b, 0.35 * reveal }, 1.2 );
        for( int i{ 0 }; i < 4; i++ )
            _frame.StrokeArc( center, 235 * scale, spin * 2 + Pi2 * i / 4, spin * 2 + Pi2 * i / 4 + 0.8, { 0.8, 0.92, 1, 0.8 * reveal }, 3 );
        for( int i{ 0 }; i < 48; i++ )
            _frame.StrokeArc( center, 270 * scale, -spin * 1.4 + Pi2 * i / 48, -spin * 1.4 + Pi2 * i / 48 + Pi2 / 96, { blue.r, blue.g, blue.b, 0.7 * reveal }, 2 );
        _frame.StrokeCircle( center, 310 * scale, { amber.r, amber.g, amber.b, 0.3 * reveal }, 1.2 );
        for( int i{ 0 }; i < 60; i++ ) {
            const auto angle{ spin * 0.6 + Pi2 * i / 60 };
            const auto length{ i % 5 == 0 ? 16 : 7 };
            _frame.Line( center + Vector::From( angle, 310 * scale ), center + Vector::From( angle, 310 * scale + length ), { amber.r, amber.g, amber.b, ( i % 5 == 0 ? 0.85 : 0.4 ) * reveal }, 1.4 );
        }
        _frame.FillCircle( center + Vector::From( -spin * 2.6, 310 * scale ), 3.5, { 1, 0.9, 0.7, reveal } );
    }

    // three tumbling rings in fake 3D, in the colors of the two bosses
    {
        Scene3D scene{ center, 0, 1 };
        const auto grow{ 0.4 + 0.5 * EaseOut( t / 2.4 ) };
        const auto tumble{ t * 0.35 };
        const auto ring{ [ & ]( const double _radius, const double _tube, const double _tiltX, const double _tiltY, const double _spin, const Color_d & _lit, const Color_d & _dark ) {
                scene.Add( Mesh3D::Torus( _radius * grow, _tube, 40, 5, [ & ]( const int _segment, const int, Mesh3D::Face & _out ) {
                        const bool lit{ _segment % 8 < 4 };
                        _out.color = lit ? _lit : _dark;
                        _out.glow = lit ? 0.35 : 0.1;
                        _out.edge = { _lit.r, _lit.g, _lit.b, 0.55 };
                        _out.edgeWidth = 1;
                    } ).Transformed( [ & ]( const Vector3 & _v ) { return _v.RotatedZ( _spin ).RotatedX( _tiltX ).RotatedY( _tiltY ); } ) );
            } };
        ring( 360, 5, 0.9 + 0.3 * std::sin( tumble * 0.8 ), tumble * 0.9, -t * 1.0, { 0.45, 0.7, 0.95 }, { 0.14, 0.2, 0.4 } );
        ring( 430, 4, -0.7, 0.6 + tumble * 1.1, t * 1.4, { 0.95, 0.62, 0.2 }, { 0.3, 0.2, 0.14 } );
        ring( 500, 3.5, 1.35 + 0.2 * std::sin( tumble * 0.6 ), -tumble * 0.7, t * 0.8, { 0.45, 0.7, 0.95 }, { 0.14, 0.2, 0.4 } );
        scene.Draw( _frame );
    }

    // the title, settling out of a chromatic glitch: the red and blue copies slide back onto the white one
    const auto titleSize{ std::min( 150.0, screen.u * 0.1 ) };
    const auto offset{ ( 1 - settle ) * 46 };
    const auto fade{ std::clamp( t / 0.9, 0.0, 1.0 ) };
    const auto glitch{ t < 1.6 && std::fmod( t * 11, 1.0 ) < 0.12 ? 1.0 : 0.0 }; // a few jolts while it settles
    const Vector place{ center + Vector{ glitch * 9, 0 } };
    const std::string title{ "STARSHIP" };
    const auto spacing{ titleSize * 0.18 };
    {
        const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
        _frame.Text( place + Vector{ -offset, 0 }, "openSansBold", titleSize, title, { 1, 0.2, 0.25, 0.7 * fade }, NanoVGRenderer::Frame::eTextAlign::center, spacing );
        _frame.Text( place + Vector{ offset, 0 }, "openSansBold", titleSize, title, { 0.2, 0.5, 1, 0.7 * fade }, NanoVGRenderer::Frame::eTextAlign::center, spacing );
        _frame.Text( place + Vector{ 0, 4 }, "openSansBold", titleSize, title, { blue.r, blue.g, blue.b, 0.35 * fade }, NanoVGRenderer::Frame::eTextAlign::center, spacing );
    }
    _frame.Text( place, "openSansBold", titleSize, title, { 1, 1, 1, fade }, NanoVGRenderer::Frame::eTextAlign::center, spacing );

    // a line drawing itself under the title, a light at each end
    const auto lineReveal{ EaseOut( ( t - 0.9 ) / 1.0 ) };
    const auto lineHalf{ titleSize * 3.2 * lineReveal };
    const Vector lineCenter{ center + Vector{ 0, titleSize * 0.78 } };
    if( lineReveal > 0 ) {
        _frame.Line( lineCenter - Vector{ lineHalf, 0 }, lineCenter + Vector{ lineHalf, 0 }, { blue.r, blue.g, blue.b, 0.55 }, 1.2 );
        const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
        for( const auto side : { -1.0, 1.0 } ) {
            _frame.GradientCircle( lineCenter + Vector{ side * lineHalf, 0 }, 16, { 0.8, 0.92, 1, 0.8 }, { blue.r, blue.g, blue.b, 0 } );
            _frame.FillCircle( lineCenter + Vector{ side * lineHalf, 0 }, 2.8, { 1, 1, 1, 1 } );
        }
    }
    _frame.Text( lineCenter + Vector{ 0, 42 }, "openSansBold", 20, "A  S P A C E  S H O O T E R", { 0.9, 0.95, 1, 1.0 * EaseOut( ( t - 1.3 ) / 0.8 ) }, NanoVGRenderer::Frame::eTextAlign::center, 7 );

    // the version
    _frame.Text( { screen.u - 24, screen.v - 16 }, "sourceCodePro", 13, std::format( "BETA {}", _version ), { 1, 1, 1, 0.4 * fade }, NanoVGRenderer::Frame::eTextAlign::bottomRight );

    // up from black, with a flash of the jump out of hyperspace
    if( t < 0.7 )
        _frame.FillRectangle( {}, screen, { 0, 0.02, 0.05, 1 - t / 0.7 } );
    const auto flash{ std::exp( -t * 7 ) * 0.5 };
    if( flash > 0.01 ) {
        const auto composition{ _frame.SetComposition( NanoVGRenderer::Frame::Composition::eType::add ) };
        _frame.GradientCircle( center, 600, { 0.7, 0.85, 1, flash }, { 0.3, 0.5, 1, 0 } );
    }
}
