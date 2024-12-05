#pragma once

#include "OpenGL.h"


// ----------------
class NanoVGRenderer
{
public:
    NanoVGRenderer( const OpenGL & _openGL );
    ~NanoVGRenderer();

public:
    class Frame;
    Frame CreateFrame( const Dimension_ui & _dimension ) const;
    bool CreateFont( const std::string & _name, const std::string & _path ) const;

private:
    void * m_context;
};


// ----------------
class NanoVGRenderer::Frame
{
public:
    Frame( const Dimension_ui & _dimension, void * _context );
    ~Frame();

public:
    struct Composition
    {
        enum class eType {
            add
        };

        void * m_context;

        Composition( void * _context, const eType _type );
        ~Composition();
    };

    Composition SetComposition( const Composition::eType _type ) const;

    void Line( const Position_d & _a, const Position_d & _b, const Color_d & _color, const double _strokeWidth ) const;
    void FillCircle( const Position_d & _position, const double _radius, const Color_d & _color ) const;
    void StrokeCircle( const Position_d & _position, const double _radius, const Color_d & _color, const double _strokeWidth ) const;
    void GradientCircle( const Position_d & _position, const double _radius, const Color_d & _colorInner, const Color_d & _colorOuter ) const;
    void FillArc( const Position_d & _position, const double _radius, const double _angleA, const double _angleB, const Color_d & _color, const bool _clockWise = true ) const;
    void StrokeArc( const Position_d & _position, const double _radius, const double _angleA, const double _angleB, const Color_d & _color, const double _strokeWidth, const bool _clockWise = true ) const;
    void FillRectangle( const Position_d & _a, const Position_d & _b, const Color_d & _color, const double _borderRadius = 0 ) const;
    void StrokeRectangle( const Position_d & _a, const Position_d & _b, const Color_d & _color, const double _strokeWidth, const double _borderRadius = 0 ) const;
    enum class eTextAlign {
        topLeft,
        topRight,
        bottomLeft,
        bottomRight,
        center
    };
    void Text( const Position_d & _position, const std::string & _fontName, const double _size, const std::string & _text, const Color_d & _color, const eTextAlign _textAlign = eTextAlign::topLeft ) const;
    void Reflect( const Position_d & _position, const double _radius, const Color_d & _color, const double _reflectAngle, const double _animation ) const;

private:
    void * m_context;
};
