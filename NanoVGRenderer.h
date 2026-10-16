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
    Frame CreateFrame( const View & _view ) const; // frame coordinates are logical, rendering is scaled to the view
    bool CreateFont( const std::string & _name, const std::string & _path ) const;

private:
    void * m_context;
};


// ----------------
class NanoVGRenderer::Frame
{
public:
    Frame( const View & _view, void * _context );
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
    void FillCircle( const Position_d & _position, const double _radius, const Color_d & _color, const bool _antialias = true ) const;
    void StrokeCircle( const Position_d & _position, const double _radius, const Color_d & _color, const double _strokeWidth ) const;
    void GradientCircle( const Position_d & _position, const double _radius, const Color_d & _colorInner, const Color_d & _colorOuter ) const;
    void FillArc( const Position_d & _position, const double _radius, const double _angleA, const double _angleB, const Color_d & _color, const bool _clockWise = true ) const;
    void StrokeArc( const Position_d & _position, const double _radius, const double _angleA, const double _angleB, const Color_d & _color, const double _strokeWidth, const bool _clockWise = true ) const;
    void FillPolygon( const std::vector< Position_d > & _points, const Color_d & _color ) const;
    void StrokePolygon( const std::vector< Position_d > & _points, const Color_d & _color, const double _strokeWidth ) const; // closed, rounded joins
    void StrokePolyline( const std::vector< Position_d > & _points, const Color_d & _color, const double _strokeWidth ) const; // open, rounded joins and caps
    void FillRectangle( const Position_d & _a, const Position_d & _b, const Color_d & _color, const double _borderRadius = 0 ) const;
    void GradientRectangle( const Position_d & _a, const Position_d & _b, const Color_d & _colorTop, const Color_d & _colorBottom, const double _borderRadius = 0 ) const; // vertical gradient
    void StrokeRectangle( const Position_d & _a, const Position_d & _b, const Color_d & _color, const double _strokeWidth, const double _borderRadius = 0 ) const;
    enum class eTextAlign {
        topLeft,
        topRight,
        bottomLeft,
        bottomRight,
        center
    };
    double TextWidth( const std::string & _fontName, const double _size, const std::string & _text, const double _letterSpacing = 0 ) const; // as Text() would draw it
    void Text( const Position_d & _position, const std::string & _fontName, const double _size, const std::string & _text, const Color_d & _color, const eTextAlign _textAlign = eTextAlign::topLeft, const double _letterSpacing = 0 ) const;
    void Reflect( const Position_d & _position, const double _radius, const Color_d & _color, const double _reflectAngle, const double _animation ) const;

private:
    void * m_context;
};
