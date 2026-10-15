#include "Mesh3D.h"


namespace
{
    constexpr double shiftX{ 0.1 }, shiftY{ -0.55 };                 // oblique projection: where a unit of height goes on the screen
    const Vector3 viewer{ -shiftX, -shiftY, 1 };                     // toward the viewer: what is seen along the projection
    const Vector3 light{ Vector3{ -0.5, -0.35, 0.8 }.Normalized() }; // from the upper left

    // right-hand normal of a polygon (not normalized): outward when the corners turn counter-clockwise seen from outside
    Vector3 FaceNormal( const std::vector< Vector3 > & _vertices, const std::vector< int > & _corners )
    {
        Vector3 normal;
        for( size_t i{ 0 }; i < _corners.size(); i++ ) {
            const auto & p{ _vertices.at( static_cast< size_t >( _corners.at( i ) ) ) };
            const auto & q{ _vertices.at( static_cast< size_t >( _corners.at( ( i + 1 ) % _corners.size() ) ) ) };
            normal.x += ( p.y - q.y ) * ( p.z + q.z );
            normal.y += ( p.z - q.z ) * ( p.x + q.x );
            normal.z += ( p.x - q.x ) * ( p.y + q.y );
        }
        return normal;
    }

    Vector3 FaceCenter( const std::vector< Vector3 > & _vertices, const std::vector< int > & _corners )
    {
        Vector3 sum;
        for( const auto corner : _corners )
            sum = sum + _vertices.at( static_cast< size_t >( corner ) );
        return sum * ( 1.0 / static_cast< double >( _corners.size() ) );
    }

    // turns the face around when it looks toward _inside
    void OrientFace( const std::vector< Vector3 > & _vertices, Mesh3D::Face & _face, const Vector3 & _inside )
    {
        if( FaceNormal( _vertices, _face.corners ).DotProd( FaceCenter( _vertices, _face.corners ) - _inside ) < 0 )
            std::ranges::reverse( _face.corners );
    }
}


Mesh3D Mesh3D::Extrusion( const Outline & _outline, const double _z0, const double _z1, const Color_d & _top, const Color_d & _side, const Color_d & _edge, const double _edgeWidth )
{
    Mesh3D mesh;
    const auto count{ static_cast< int >( _outline.size() ) };
    for( const auto & [ x, y ] : _outline )
        mesh.vertices.emplace_back( Vector3{ x, y, _z1 } );
    for( const auto & [ x, y ] : _outline )
        mesh.vertices.emplace_back( Vector3{ x, y, _z0 } );
    Face top{ {}, _top, 0, _edge, _edgeWidth };
    Face bottom{ {}, _top, 0, _edge, _edgeWidth };
    for( int i{ 0 }; i < count; i++ ) {
        top.corners.emplace_back( i );
        bottom.corners.emplace_back( count + i );
        mesh.faces.emplace_back( Face{ { count + i, count + ( i + 1 ) % count, ( i + 1 ) % count, i }, _side, 0, _edge, _edgeWidth * 0.6 } );
    }
    mesh.faces.emplace_back( std::move( top ) );
    mesh.faces.emplace_back( std::move( bottom ) );
    mesh._Orient();
    return mesh;
}


Mesh3D Mesh3D::Box( const double _x0, const double _x1, const double _y0, const double _y1, const double _z0, const double _z1, const Color_d & _top, const Color_d & _side, const Color_d & _edge, const double _edgeWidth )
{
    return Extrusion( { { _x0, _y0 }, { _x1, _y0 }, { _x1, _y1 }, { _x0, _y1 } }, _z0, _z1, _top, _side, _edge, _edgeWidth );
}


Mesh3D Mesh3D::Beam( const Vector3 & _from, const Vector3 & _to, const double _halfWidth, const Color_d & _color, const Color_d & _edge, const double _edgeWidth )
{
    const auto axis{ ( _to - _from ).Normalized() };
    const auto reference{ std::abs( axis.z ) < 0.9 ? Vector3{ 0, 0, 1 } : Vector3{ 1, 0, 0 } };
    const auto e1{ axis.CrossProd( reference ).Normalized() * _halfWidth };
    const auto e2{ axis.CrossProd( e1 ).Normalized() * _halfWidth };
    Mesh3D mesh;
    for( const auto & end : { _from, _to } )
        for( const auto & [ a, b ] : std::array< std::pair< double, double >, 4 >{ { { 1, 1 }, { -1, 1 }, { -1, -1 }, { 1, -1 } } } )
            mesh.vertices.emplace_back( end + e1 * a + e2 * b );
    for( int i{ 0 }; i < 4; i++ )
        mesh.faces.emplace_back( Face{ { i, ( i + 1 ) % 4, 4 + ( i + 1 ) % 4, 4 + i }, _color, 0, _edge, _edgeWidth } );
    mesh.faces.emplace_back( Face{ { 0, 1, 2, 3 }, _color, 0, _edge, _edgeWidth } );
    mesh.faces.emplace_back( Face{ { 4, 5, 6, 7 }, _color, 0, _edge, _edgeWidth } );
    mesh._Orient();
    return mesh;
}


Mesh3D Mesh3D::Lathe( const std::vector< std::pair< double, double > > & _profile, const int _segments, const double _y, const double _z,
    const std::function< Color_d( int, int ) > & _color, const Color_d & _edge, const double _valley, const double _twist, const double _spin )
{
    Mesh3D mesh;
    const auto rings{ static_cast< int >( _profile.size() ) };
    for( const auto & [ x, radius ] : _profile )
        for( int k{ 0 }; k < _segments; k++ ) {
            const auto angle{ _spin + _twist * ( x - _profile.front().first ) + Maths::Pi2 * k / _segments };
            const auto r{ radius * ( k % 2 == 1 ? _valley : 1.0 ) };
            mesh.vertices.emplace_back( Vector3{ x, _y + r * std::cos( angle ), _z + r * std::sin( angle ) } );
        }
    const auto index{ [ & ]( const int _ring, const int _segment ) { return _ring * _segments + _segment % _segments; } };
    const auto middle{ ( _profile.front().first + _profile.back().first ) * 0.5 };
    for( int j{ 0 }; j + 1 < rings; j++ ) {
        const auto x{ ( _profile.at( static_cast< size_t >( j ) ).first + _profile.at( static_cast< size_t >( j ) + 1 ).first ) * 0.5 };
        for( int k{ 0 }; k < _segments; k++ ) {
            Face face{ { index( j, k ), index( j, k + 1 ), index( j + 1, k + 1 ), index( j + 1, k ) }, _color( j, k ), 0, _edge, 1 };
            OrientFace( mesh.vertices, face, Vector3{ x, _y, _z } ); // outward: away from the axis
            mesh.faces.emplace_back( std::move( face ) );
        }
    }
    for( const auto ring : { 0, rings - 1 } ) { // the end caps, unless the solid closes to a point
        if( _profile.at( static_cast< size_t >( ring ) ).second <= 0 )
            continue;
        Face cap{ {}, _color( std::min( ring, rings - 2 ), 0 ), 0, _edge, 1 };
        for( int k{ 0 }; k < _segments; k++ )
            cap.corners.emplace_back( index( ring, k ) );
        OrientFace( mesh.vertices, cap, Vector3{ middle, _y, _z } );
        mesh.faces.emplace_back( std::move( cap ) );
    }
    return mesh;
}


Mesh3D Mesh3D::Glowing( const double _glow ) const
{
    auto mesh{ *this };
    for( auto & face : mesh.faces )
        face.glow = _glow;
    return mesh;
}


void Mesh3D::_Orient()
{
    Vector3 sum;
    for( const auto & vertex : vertices )
        sum = sum + vertex;
    const auto inside{ sum * ( 1.0 / static_cast< double >( vertices.size() ) ) };
    for( auto & face : faces )
        OrientFace( vertices, face, inside );
}


// --------------
Scene3D::Scene3D( const Vector & _origin, const double _heading, const double _scale, const double _roll )
    : m_origin{ _origin }
    , m_scale{ _scale }
    , m_roll{ _roll }
    , m_forward{ Vector::From( _heading, 1 ) }
    , m_across{ Vector::From( _heading + Maths::PiHalf, 1 ) }
{}


Vector3 Scene3D::World( const Vector3 & _model ) const
{
    const auto rolled{ _model.RotatedX( m_roll ) };
    return { ( m_forward.u * rolled.x + m_across.u * rolled.y ) * m_scale, ( m_forward.v * rolled.x + m_across.v * rolled.y ) * m_scale, rolled.z * m_scale };
}


Vector Scene3D::Project( const Vector3 & _world ) const
{
    return { m_origin.u + _world.x + shiftX * _world.z, m_origin.v + _world.y + shiftY * _world.z };
}


void Scene3D::Add( const Mesh3D & _mesh )
{
    std::vector< Vector3 > world;
    world.reserve( _mesh.vertices.size() );
    for( const auto & vertex : _mesh.vertices )
        world.emplace_back( World( vertex ) );
    for( const auto & face : _mesh.faces ) {
        auto normal{ FaceNormal( world, face.corners ) };
        const auto length{ std::sqrt( normal.DotProd( normal ) ) };
        if( length < 1e-9 )
            continue;
        normal = normal * ( 1 / length );
        if( normal.DotProd( viewer ) <= 0.001 ) // turned away
            continue;
        Item item;
        item.depth = FaceCenter( world, face.corners ).DotProd( viewer );
        for( const auto corner : face.corners )
            item.polygon.emplace_back( Project( world.at( static_cast< size_t >( corner ) ) ) );
        const auto k{ ( 0.42 + 0.95 * std::max( 0.0, normal.DotProd( light ) ) ) * ( 1 - face.glow ) + face.glow };
        item.fill = { std::min( face.color.r * k, 1.0 ), std::min( face.color.g * k, 1.0 ), std::min( face.color.b * k, 1.0 ), face.color.a };
        item.edge = face.edge;
        item.edgeWidth = face.edgeWidth;
        m_items.emplace_back( std::move( item ) );
    }
}


void Scene3D::Add( const Vector3 & _modelPoint, std::function< void() > _draw )
{
    Item item;
    item.depth = World( _modelPoint ).DotProd( viewer ) + 0.5; // a hair above the face it sits on
    item.custom = std::move( _draw );
    m_items.emplace_back( std::move( item ) );
}


void Scene3D::AddLast( std::function< void() > _draw )
{
    Item item;
    item.custom = std::move( _draw );
    m_lastItems.emplace_back( std::move( item ) );
}


void Scene3D::Draw( const NanoVGRenderer::Frame & _frame )
{
    std::ranges::stable_sort( m_items, []( const Item & _a, const Item & _b ) { return _a.depth < _b.depth; } );
    const auto draw{ [ & ]( const Item & _item ) {
            if( _item.custom ) {
                _item.custom();
                return;
            }
            _frame.FillPolygon( _item.polygon, _item.fill );
            if( _item.edge.a > 0 )
                _frame.StrokePolygon( _item.polygon, _item.edge, _item.edgeWidth );
        } };
    for( const auto & item : m_items )
        draw( item );
    for( const auto & item : m_lastItems )
        draw( item );
    m_items.clear();
    m_lastItems.clear();
}
