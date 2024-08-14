#include "NanoVGSurface.h"

#include "external/glew/glew.h"
#include "external/nanovg/nanovg.h"
#include "external/nanovg/nanovg_gl.h"


// ----------------

NanoVGSurface::NanoVGSurface( const Win32::Windows & _windows )
    : m_ogl{ _windows }
{
    auto context{ m_ogl.MakeCurrent() };
    ::glewInit();
    m_context = ::nvgCreateGL3( NVG_ANTIALIAS | NVG_STENCIL_STROKES );
}


NanoVGSurface::~NanoVGSurface()
{
    ::nvgDeleteGL3( static_cast< NVGcontext * >( m_context ) );
}


NanoVGSurface::Frame NanoVGSurface::CreateFrame( const Dimension_ui & _dimension, const Color_d & _color ) const
{
    return { _dimension, _color, m_ogl, m_context };
}


bool NanoVGSurface::CreateFont( const std::string & _name, const std::string & _path ) const
{
    return ::nvgCreateFont( static_cast< NVGcontext * >( m_context ), _name.c_str(), _path.c_str() ) != -1;
}


// ----------------

NanoVGSurface::Frame::Frame( const Dimension_ui & _dimension, const Color_d & _color, const OpenGLSurface & _ogl, void * _context )
    : m_glContext{ _ogl.MakeCurrent() }
    , m_context{ _context }
{
    m_glContext.Viewport( _dimension );
    m_glContext.Clear( _color );
    ::nvgBeginFrame( static_cast< NVGcontext * >( m_context ),
        static_cast< float >( _dimension.width ), static_cast< float >( _dimension.height ), 1 );
}


NanoVGSurface::Frame::~Frame()
{
    ::nvgEndFrame( static_cast< NVGcontext * >( m_context ) );
}


void NanoVGSurface::Frame::Line( const Position_d & _a, const Position_d & _b, const Color_d & _color, const double _strokeWidth ) const
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


void NanoVGSurface::Frame::FillCircle( const Position_d & _position, const double _radius, const Color_d & _color ) const
{
    auto context{ static_cast< NVGcontext * >( m_context ) };
    ::nvgBeginPath( context );
    const auto position{ _position.ToType< float >() };
    ::nvgCircle( context, position.x, position.y, static_cast< float >( _radius ) );
    const auto color{ ( _color * 255 ).ToType< unsigned char >() };
    ::nvgFillColor( context, ::nvgRGBA( color.r, color.g, color.b, 255 ) );
    ::nvgFill( context );
}


void NanoVGSurface::Frame::StrokeCircle( const Position_d & _position, const double _radius, const Color_d & _color, const double _strokeWidth ) const
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


void NanoVGSurface::Frame::FillArc( const Position_d & _position, const double _radius, const double _angleA, const double _angleB, const Color_d & _color, const bool _clockWise ) const
{
    auto context{ static_cast< NVGcontext * >( m_context ) };
    ::nvgBeginPath( context );
    const auto position{ _position.ToType< float >() };
    ::nvgArc( context, position.x, position.y, static_cast< float >( _radius ), static_cast< float >( _angleA ), static_cast< float >( _angleB ), _clockWise ? NVG_CW : NVG_CCW );
    const auto color{ ( _color * 255 ).ToType< unsigned char >() };
    ::nvgFillColor( context, ::nvgRGBA( color.r, color.g, color.b, 255 ) );
    ::nvgFill( context );
}


void NanoVGSurface::Frame::StrokeArc( const Position_d & _position, const double _radius, const double _angleA, const double _angleB, const Color_d & _color, const double _strokeWidth, const bool _clockWise ) const
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


void NanoVGSurface::Frame::Text( const Position_d & _position, const std::string & _fontName, const double _size, const std::string & _text, const Color_d & _color )
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