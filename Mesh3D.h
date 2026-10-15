#pragma once

#include "NanoVGRenderer.h"


// --------------
// Fake 3D for the vector renderer: convex-ish solids (boxes, beams, lathed cones and cylinders...)
// described in model space (x forward, y across, z up), rotated by a heading, then projected with an
// oblique projection: the ground plane (z = 0) is drawn exactly as the top-down game sees it and
// the heights shift the points up the screen, so a hit box on the ground still matches the picture.
// Only the faces turned toward the viewer are drawn, lit from a fixed light, far to near.
struct Mesh3D
{
    struct Face
    {
        std::vector< int > corners;
        Color_d color;
        double glow{ 0 };           // 0: lit by the scene light .. 1: emissive
        Color_d edge{ 0, 0, 0, 0 }; // outline, alpha 0: none
        double edgeWidth{ 2 };
    };

    std::vector< Vector3 > vertices;
    std::vector< Face > faces;

    using Outline = std::vector< std::pair< double, double > >; // (x, y) points of a convex polygon

    static Mesh3D Extrusion( const Outline & _outline, const double _z0, const double _z1, const Color_d & _top, const Color_d & _side, const Color_d & _edge, const double _edgeWidth = 2 );
    static Mesh3D Box( const double _x0, const double _x1, const double _y0, const double _y1, const double _z0, const double _z1, const Color_d & _top, const Color_d & _side, const Color_d & _edge, const double _edgeWidth = 1.5 );
    static Mesh3D Beam( const Vector3 & _from, const Vector3 & _to, const double _halfWidth, const Color_d & _color, const Color_d & _edge, const double _edgeWidth = 1.5 );
    // a solid of revolution around the x axis (at y, z), from a profile of (x, radius): _color gets the (band, segment)
    // of every face. With _valley < 1 the odd segments are pushed in (flutes), and _twist (rad per x unit) and _spin turn them: a drill
    static Mesh3D Lathe( const std::vector< std::pair< double, double > > & _profile, const int _segments, const double _y, const double _z,
        const std::function< Color_d( int, int ) > & _color, const Color_d & _edge, const double _valley = 1, const double _twist = 0, const double _spin = 0 );

    // a torus around the z axis (the ring in the xy plane); _style gets the (segment, side) of every face
    static Mesh3D Torus( const double _radius, const double _tube, const int _segments, const int _sides, const std::function< void( int, int, Face & ) > & _style );
    // a faceted sphere (80 triangles); _style gets the index and the direction of every face
    static Mesh3D Icosphere( const double _radius, const std::function< void( int, const Vector3 &, Face & ) > & _style );

    Mesh3D Glowing( const double _glow ) const; // a copy, emissive
    Mesh3D Transformed( const std::function< Vector3( const Vector3 & ) > & _transform ) const; // a copy, its points moved

private:
    void _Orient(); // convex solids: outward is away from the centroid
};


// --------------
class Scene3D
{
public:
    // _origin: where the model origin stands on the screen, _heading: where its x axis points, _scale: pixels per model unit,
    // _roll: a turn around the x axis
    Scene3D( const Vector & _origin, const double _heading, const double _scale, const double _roll = 0 );

public:
    Vector3 World( const Vector3 & _model ) const; // pixels around the origin, z up toward the viewer
    Vector Project( const Vector3 & _world ) const;
    Vector ProjectModel( const Vector3 & _model ) const { return Project( World( _model ) ); }

    void Add( const Mesh3D & _mesh );
    void Add( const Vector3 & _modelPoint, std::function< void() > _draw ); // a glow, a decal: drawn with the faces, at the depth of the point
    void AddLast( std::function< void() > _draw ); // above everything
    void Draw( const NanoVGRenderer::Frame & _frame ); // far to near; empties the scene

private:
    struct Item
    {
        double depth{ 0 };
        std::vector< Position_d > polygon;
        Color_d fill, edge;
        double edgeWidth{ 0 };
        std::function< void() > custom;
    };

    const Vector m_origin;
    const double m_scale;
    const double m_roll;
    const Vector m_forward, m_across;
    std::vector< Item > m_items;
    std::vector< Item > m_lastItems;
};
