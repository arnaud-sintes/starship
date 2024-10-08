#include "NanoVGRenderer.h"

#include "external/glew/glew.h"
#include "external/nanovg/nanovg.h"
#include "external/nanovg/nanovg_gl.h"


// ----------------

NanoVGRenderer::NanoVGRenderer( const OpenGL & _openGL )
{
    auto context{ _openGL.MakeCurrent() };
    ::glewInit();
    m_context = ::nvgCreateGL3( NVG_ANTIALIAS /*| NVG_STENCIL_STROKES*/ );
}


NanoVGRenderer::~NanoVGRenderer()
{
    ::nvgDeleteGL3( static_cast< NVGcontext * >( m_context ) );
}


NanoVGRenderer::Frame NanoVGRenderer::CreateFrame( const Dimension_ui & _dimension ) const
{
    return { _dimension, m_context };
}


bool NanoVGRenderer::CreateFont( const std::string & _name, const std::string & _path ) const
{
    return ::nvgCreateFont( static_cast< NVGcontext * >( m_context ), _name.c_str(), _path.c_str() ) != -1;
}


// ----------------

NanoVGRenderer::Frame::Frame(const Dimension_ui & _dimension, void * _context )
    : m_context{ _context }
{   ::nvgBeginFrame( static_cast< NVGcontext * >( m_context ),
        static_cast< float >( _dimension.width ), static_cast< float >( _dimension.height ), 1 );
}


NanoVGRenderer::Frame::~Frame()
{
    ::nvgEndFrame( static_cast< NVGcontext * >( m_context ) );
}


void NanoVGRenderer::Frame::Line( const Position_d & _a, const Position_d & _b, const Color_d & _color, const double _strokeWidth ) const
{
    auto context{ static_cast< NVGcontext * >( m_context ) };
    ::nvgBeginPath( context );
    const auto a{ _a.ToType< float >() };
    ::nvgMoveTo( context, a.x, a.y );
    const auto b{ _b.ToType< float >() };
    ::nvgLineTo( context, b.x, b.y );
    const auto color{ ( _color * 255 ).ToType< unsigned char >() };
    ::nvgStrokeColor( context, ::nvgRGBA( color.r, color.g, color.b, 255 ) );
    ::nvgStrokeWidth( context, static_cast< float >( _strokeWidth ) );
    ::nvgStroke( context );
}


void NanoVGRenderer::Frame::FillCircle( const Position_d & _position, const double _radius, const Color_d & _color ) const
{
    auto context{ static_cast< NVGcontext * >( m_context ) };
    ::nvgBeginPath( context );
    const auto position{ _position.ToType< float >() };
    ::nvgCircle( context, position.x, position.y, static_cast< float >( _radius ) );
    const auto color{ ( _color * 255 ).ToType< unsigned char >() };
    ::nvgFillColor( context, ::nvgRGBA( color.r, color.g, color.b, 255 ) );
    ::nvgFill( context );
}


void NanoVGRenderer::Frame::StrokeCircle( const Position_d & _position, const double _radius, const Color_d & _color, const double _strokeWidth ) const
{
    auto context{ static_cast< NVGcontext * >( m_context ) };
    ::nvgBeginPath( context );
    const auto position{ _position.ToType< float >() };
    ::nvgCircle( context, position.x, position.y, static_cast< float >( _radius ) );
    const auto color{ ( _color * 255 ).ToType< unsigned char >() };
    ::nvgStrokeColor( context, ::nvgRGBA( color.r, color.g, color.b, 255 ) );
    ::nvgStrokeWidth( context, static_cast< float >( _strokeWidth ) );
    ::nvgStroke( context );
}


void NanoVGRenderer::Frame::FillArc( const Position_d & _position, const double _radius, const double _angleA, const double _angleB, const Color_d & _color, const bool _clockWise ) const
{
    auto context{ static_cast< NVGcontext * >( m_context ) };
    ::nvgBeginPath( context );
    const auto position{ _position.ToType< float >() };
    ::nvgArc( context, position.x, position.y, static_cast< float >( _radius ), static_cast< float >( _angleA ), static_cast< float >( _angleB ), _clockWise ? NVG_CW : NVG_CCW );
    const auto color{ ( _color * 255 ).ToType< unsigned char >() };
    ::nvgFillColor( context, ::nvgRGBA( color.r, color.g, color.b, 255 ) );
    ::nvgFill( context );
}


void NanoVGRenderer::Frame::StrokeArc( const Position_d & _position, const double _radius, const double _angleA, const double _angleB, const Color_d & _color, const double _strokeWidth, const bool _clockWise ) const
{
    auto context{ static_cast< NVGcontext * >( m_context ) };
    ::nvgBeginPath( context );
    const auto position{ _position.ToType< float >() };
    ::nvgArc( context, position.x, position.y, static_cast< float >( _radius ), static_cast< float >( _angleA ), static_cast< float >( _angleB ), _clockWise ? NVG_CW : NVG_CCW );
    const auto color{ ( _color * 255 ).ToType< unsigned char >() };
    ::nvgStrokeColor( context, ::nvgRGBA( color.r, color.g, color.b, 255 ) );
    ::nvgStrokeWidth( context, static_cast< float >( _strokeWidth ) );
    ::nvgStroke( context );
}


void NanoVGRenderer::Frame::FillRectangle( const Position_d & _a, const Position_d & _b, const Color_d & _color, const double _borderRadius ) const
{
    auto context{ static_cast< NVGcontext * >( m_context ) };
    ::nvgBeginPath( context );
    const auto a{ _a.ToType< float >() };
    const auto d{ ( Vector::From( _b ) - Vector::From( _a ) ).ToFloat() };
    if( _borderRadius != 0 )
        ::nvgRoundedRect( context, a.x, a.y, d.u, d.v, static_cast< float >( _borderRadius ) );
    else
        ::nvgRect( context, a.x, a.y, d.u, d.v );
    const auto color{ ( _color * 255 ).ToType< unsigned char >() };
    ::nvgFillColor( context, ::nvgRGBA( color.r, color.g, color.b, 255 ) );
    ::nvgFill( context );
}


void NanoVGRenderer::Frame::StrokeRectangle( const Position_d & _a, const Position_d & _b, const Color_d & _color, const double _strokeWidth, const double _borderRadius ) const
{
    auto context{ static_cast< NVGcontext * >( m_context ) };
    ::nvgBeginPath( context );
    const auto a{ _a.ToType< float >() };
    const auto d{ ( Vector::From( _b ) - Vector::From( _a ) ).ToFloat() };
    if( _borderRadius != 0 )
        ::nvgRoundedRect( context, a.x, a.y, d.u, d.v, static_cast< float >( _borderRadius ) );
    else
        ::nvgRect( context, a.x, a.y, d.u, d.v );
    const auto color{ ( _color * 255 ).ToType< unsigned char >() };
    ::nvgStrokeColor( context, ::nvgRGBA( color.r, color.g, color.b, 255 ) );
    ::nvgStrokeWidth( context, static_cast< float >( _strokeWidth ) );
    ::nvgStroke( context );
}


void NanoVGRenderer::Frame::Text( const Position_d & _position, const std::string & _fontName, const double _size, const std::string & _text, const Color_d & _color ) const
{
    auto context{ static_cast< NVGcontext * >( m_context ) };
    ::nvgFontSize( context, static_cast< float >( _size ) );
    ::nvgFontFace( context, _fontName.c_str() );
    ::nvgTextAlign( context, NVG_ALIGN_LEFT | NVG_ALIGN_TOP );
    const auto color{ ( _color * 255 ).ToType< unsigned char >() };
    ::nvgFillColor( context, ::nvgRGBA( color.r, color.g, color.b, 255 ) );
    const auto position{ _position.ToType< float >() };
    ::nvgText( context, position.x, position.y, _text.c_str(), 0 );
}


void NanoVGRenderer::Frame::Reflect( const Position_d & _position, const double _radius, const Color_d & _color, const double _reflectAngle, const double _animation ) const
{
    auto context{ static_cast< NVGcontext * >( m_context ) };
    ::nvgSave( context );
    ::nvgBeginPath( context );
    const double sinAnim{ ( std::sin( _animation + Maths::PiHalf ) + 1 ) * 0.5 };
    const auto yPos1{ static_cast< float >( std::sin( _animation - sinAnim ) * _radius * 1.33 ) };
    const auto yPos2{ static_cast< float >( std::sin( _animation + sinAnim ) * _radius * 1.33 ) };
    const auto position{ _position.ToType< float >() };
    const auto radius{ static_cast< float >( _radius ) };
    const auto iRadius{ radius * 0.75f };
    ::nvgTranslate( context, position.x, position.y );
    ::nvgRotate( context, static_cast< float >( _reflectAngle ) );
    ::nvgTranslate( context, -position.x, -position.y );
    ::nvgMoveTo( context,   position.x - radius, position.y );
    ::nvgBezierTo( context, position.x - iRadius, position.y + yPos1,
                            position.x + iRadius, position.y + yPos1,
                            position.x + radius, position.y );
    ::nvgBezierTo( context, position.x + iRadius, position.y + yPos2,
                            position.x - iRadius, position.y + yPos2,
                            position.x - radius, position.y );
    ::nvgClosePath( context );
    const auto sinAnim90{ sinAnim * 0.9 };
    const auto color{ ( Color_d{ _color.r + ( 1 - _color.r ) * sinAnim90, _color.g + ( 1 - _color.g ) * sinAnim90, _color.b + ( 1 - _color.b ) * sinAnim90 } * 255 ).ToType< unsigned char >() };
    ::nvgFillColor( context, ::nvgRGBA( color.r, color.g, color.b, static_cast< unsigned char >( 200 * ( sinAnim90 + 0.1 ) ) ) );
    ::nvgFill( context );
    ::nvgRestore( context );
}