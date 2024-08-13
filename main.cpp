#pragma warning( push )
#pragma warning( disable: 5054 )
#include <cairo.h>
#include <cairo-win32.h>
#pragma warning( pop )

#include <windows.h>

#include <cmath>
#include <thread>
#include <mutex>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <array>
#include <random>
#include <unordered_set>
#include <functional>


class PrecisionTime
{
private:
    typedef NTSTATUS( CALLBACK* LPFN_NtQueryTimerResolution )	( PULONG, PULONG, PULONG );
    typedef NTSTATUS( CALLBACK* LPFN_NtSetTimerResolution )	( ULONG, BOOLEAN, PULONG );
    typedef NTSTATUS( CALLBACK* LPFN_NtWaitForSingleObject )	( HANDLE, BOOLEAN, PLARGE_INTEGER );

public:
    PrecisionTime()
        : m_waitEvent{ ::CreateEventA( nullptr, TRUE, FALSE, "" ) }
    {
        // map undocumented Nt dll timer functions:
        m_hNtDll = ::GetModuleHandle( L"Ntdll" );
		m_pQueryResolution = ( LPFN_NtQueryTimerResolution ) ::GetProcAddress( m_hNtDll, "NtQueryTimerResolution" );
		m_pSetResolution = ( LPFN_NtSetTimerResolution ) ::GetProcAddress( m_hNtDll, "NtSetTimerResolution" );
		m_NtWaitForSingleObject = ( LPFN_NtWaitForSingleObject ) ::GetProcAddress( ( HMODULE ) m_hNtDll, "NtWaitForSingleObject" );

        // switch to maximum precision:
        ULONG nMinRes = 0, nMaxRes = 0, nCurRes = 0;
		m_pQueryResolution( &nMinRes, &nMaxRes, &nCurRes );
        ULONG dummy = 0;
        m_pSetResolution( nMaxRes, TRUE, &dummy );

        // increase priorities:
        ::SetPriorityClass( ::GetCurrentProcess(), REALTIME_PRIORITY_CLASS );
    }

    ~PrecisionTime()
    {
        ::FreeLibrary( m_hNtDll );
        ::CloseHandle( m_waitEvent );
    }

public:
    // nano precision
    unsigned long long Get()
    {
        LARGE_INTEGER counts{ 0 };
        static LARGE_INTEGER countsPerSec{ 0 };
        static BOOL b{ ::QueryPerformanceFrequency( &countsPerSec ) };
        ::QueryPerformanceCounter( &counts );
        return static_cast< unsigned long long >( counts.QuadPart ) * 1'000'000'000 / countsPerSec.QuadPart;
    }

    // nano precision
    void Sleep( unsigned long long _ns )
    {
        LARGE_INTEGER dueTime;
	    dueTime.QuadPart = -( static_cast< long long >( _ns / 100 ) );
	    m_NtWaitForSingleObject( m_waitEvent, TRUE, &dueTime );
    }

public:
    struct TemperObj
    {
        TemperObj( PrecisionTime & _precisionTime, const unsigned long long _frameRate, std::function< void( const int _consumptionPercent ) > && _fnConsumption, std::function< void() > && _fnReport )
            : m_precisionTime{ _precisionTime }
            , m_loopStartTime{ m_precisionTime.Get() }
            , m_frameRate{ _frameRate }
            , m_fnConsumption{ std::move( _fnConsumption ) }
            , m_fnReport{ std::move( _fnReport ) }
        {}

        ~TemperObj()
        {
            const auto loopElapsedTime( m_precisionTime.Get() - m_loopStartTime );
            static const auto maxLoopDuration{ 1'000'000'000 / m_frameRate };
            if( loopElapsedTime < maxLoopDuration ) {
                m_fnConsumption( static_cast< int >( loopElapsedTime * 100 / maxLoopDuration ) );
                m_precisionTime.Sleep( maxLoopDuration - loopElapsedTime );
            }
            else
                m_fnReport();
        }

        const auto GetStartTime() const { return m_loopStartTime; }

    private:
        PrecisionTime & m_precisionTime;
        const unsigned long long m_loopStartTime;
        const unsigned long long m_frameRate;
        const std::function< void( const int _consumptionPercent ) > m_fnConsumption;
        const std::function< void() > m_fnReport;
    };

    TemperObj Temper( const unsigned long long _frameRate, std::function< void( const int _consumptionPercent ) > && _fnConsumption, std::function< void() > && _fnReport )
    {
        return { *this, _frameRate, std::move( _fnConsumption ), std::move( _fnReport ) };
    }

private:
    HMODULE m_hNtDll{ nullptr };
	LPFN_NtQueryTimerResolution m_pQueryResolution{ nullptr };
	LPFN_NtSetTimerResolution m_pSetResolution{ nullptr };
	LPFN_NtWaitForSingleObject m_NtWaitForSingleObject{ nullptr };
    HANDLE m_waitEvent{ nullptr };
};


struct Vector
{
    double u{ 0 }, v{ 0 };

    static Vector From( const double _orientation, const double _distance )
    {
        return { std::cos( _orientation ) * _distance, std::sin( _orientation ) * _distance };
    }

    double DotProd( const Vector & _other ) const
    {
        return u * _other.u + v * _other.v;
    }

    double CrossProd( const Vector & _other ) const
    {
        return u * _other.v + v * _other.u;
    }

    double Distance() const
    {
        return std::sqrt( u * u + v * v );
    }

    Vector & Normalize()
    {
        const auto distance{ Distance() };
        u /= distance;
        v /= distance;
        return *this;
    }

    Vector Normalized() const
    {
        const auto distance{ Distance() };
        return { u / distance, v / distance };
    }

    double Orientation() const
    {
        const auto distance{ Distance() };
        return std::atan2( v / distance, u / distance );
    }

    Vector & operator += ( const Vector & _other )
    {
        u += _other.u;
        v += _other.v;
        return *this;
    }

    Vector operator + ( const Vector & _other ) const
    {
        return { u + _other.u, v + _other.v };
    }

    Vector & operator -= ( const Vector & _other )
    {
        u -= _other.u;
        v -= _other.v;
        return *this;
    }

    Vector operator - ( const Vector & _other ) const
    {
        return { u - _other.u, v - _other.v };
    }

    Vector & operator *= ( const double _value )
    {
        u *= _value;
        v *= _value;
        return *this;
    }

    Vector operator * ( const double _value ) const
    {
        return { u * _value, v * _value };
    }
};


struct Maths
{
    inline static const double Pi{ 3.1415927 };
    inline static const double Pi2{ Pi * 2 };
    inline static const double PiHalf{ Pi / 2 };
    inline static const double PiQuarter{ Pi / 4 };

    static bool Collision( const Vector & _circleA, const double _radiusA, const Vector & _circleB, const double _radiusB )
    {
        const double u{ _circleA.u - _circleB.u };
        const double v{ _circleA.v - _circleB.v };
        return std::sqrt( u * u + v * v ) <= _radiusA + _radiusB;
    }

    static bool Collision( const Vector & _circle, const double _radius, const Vector & _segmentA, const Vector & _segmentB )
    {
        const Vector d{ _segmentB - _segmentA };
        const Vector f{ _segmentA - _circle };
        double a{ d.DotProd( d ) };
        double b{ 2 * f.DotProd( d ) };
        double c{ f.DotProd( f ) - _radius * _radius };
        double discriminant{ b * b - 4 * a * c };
        if( discriminant < 0 )
            return false;
        discriminant = std::sqrt( discriminant );
        double t1{ ( -b - discriminant ) / ( 2 * a ) };
        double t2{ ( -b + discriminant ) / ( 2 * a ) };
        if( t1 >= 0 && t1 <= 1 )
            return true;
        if( t2 >= 0 && t2 <= 1 )
            return true;
        return false;
    }

    static double NormalizeAngle( const double _angle )
    {
        if( _angle > Maths::Pi )
            return _angle - Maths::Pi2;
        else
        if( _angle < -Maths::Pi )
            return _angle + Maths::Pi2;
        return _angle;
    }
};


static double Random( const double _min, const double _max )
{
    static std::random_device rndDev;
    static std::mt19937 rnd{ rndDev() };
    constexpr unsigned int maxRandom{ 10000 };
    static std::uniform_int_distribution< std::mt19937::result_type > rndDist{ 0, maxRandom };
    return static_cast< double >( rndDist( rnd ) ) * ( _max - _min ) / maxRandom + _min;
}

static void Increase( double & _value, const double _increaser, const double _max )
{
    if( _value < ( _max - _increaser ) )
        _value += _increaser;
}

static void Decrease( double & _value, const double _reducer )
{
    if( _value == 0 )
        return;
    if( ( _value > 0 && _value <= _reducer ) || ( _value < 0 && _value >= _reducer ) )
        _value = 0;
    else
        _value += _reducer * ( _value > 0 ? -1 : 1 );
}


struct Graphics
{
    struct Color
    {
        double r, g, b;
    };

    inline static const Color colorWhite{ 1, 1, 1 };

    struct Arc
    {
        double a, b;
    };

    inline static const Arc arcFull{ 0, Maths::Pi2 };

    static Color FireColor( double _ratio )
    {
        _ratio = ( 1 - ( _ratio * _ratio ) ) * 3;
        return Color{ _ratio > 1 ? 1 : _ratio,
            _ratio > 2 ? 1 : ( _ratio > 1 ? _ratio - 1 : 0 ),
            _ratio > 2 ? _ratio - 2 : 0 };
    }

    static void Stroke( cairo_t & _cairo, const Vector & _position, const double _radius, const Arc & _arc, const Color & _color, const double _strokeWidth )
    {
        cairo_new_path( &_cairo );
        cairo_set_source_rgb( &_cairo, _color.r, _color.g, _color.b );
        cairo_arc( &_cairo, _position.u, _position.v, _radius, _arc.a, _arc.b );
        cairo_set_line_width( &_cairo, _strokeWidth );
        cairo_stroke( &_cairo );
    }

    static void Fill( cairo_t & _cairo, const Vector & _position, const double _radius, const Arc & _arc, const Color & _color )
    {
        cairo_new_path( &_cairo );
        cairo_set_source_rgb( &_cairo, _color.r, _color.g, _color.b );
        cairo_arc( &_cairo, _position.u, _position.v, _radius, _arc.a, _arc.b );
        cairo_fill( &_cairo );
    }

    static void Line( cairo_t & _cairo, const Vector & _a, const Vector & _b, const Color & _color, const double _strokeWidth, const bool _dash )
    {
        cairo_new_path( &_cairo );
        cairo_set_source_rgb( &_cairo, _color.r, _color.g, _color.b );
        cairo_set_line_width( &_cairo, _strokeWidth );
        cairo_move_to( &_cairo, _a.u, _a.v );
        cairo_line_to( &_cairo, _b.u, _b.v );
        if( _dash ) {
            cairo_save( &_cairo );
            static double dashes[]{ 6, 3 };
            cairo_set_dash( &_cairo, dashes, 2, 0 );
        }
        cairo_stroke( &_cairo );
        if( _dash )
            cairo_restore( &_cairo );
    }

    static void Text( cairo_t & _cairo, const Vector & _position, const double _size, const Color & _color, const std::string & _text )
    {
        cairo_new_path( &_cairo );
        cairo_set_source_rgb( &_cairo, _color.r, _color.g, _color.b );
        cairo_set_font_size( &_cairo, _size );
        cairo_move_to( &_cairo, _position.u, _position.v );
        cairo_show_text( &_cairo, _text.c_str() );
    }
};


struct Rocket
{
    Graphics::Color color;
    Vector position;
    double orientation;

    Vector thrustMotion; // cumulated thrust motion vector

    Vector momentum; // position momentum (thrust motion minus drag force)
    double rotationMomentum; // rotation momentum

    double damage;
    
    struct Shield
    {
        double value; // capacity by default
        double capacity;
        double repair_rate;
        double quality; // better quality -> thiner shield, with incidence on total mass (drag force penality)
    };
    Shield shield;

    struct Propellant
    {
        double value; // capacity by default
        double capacity;
        double production_rate;
        double quality; // better quality -> less tank size, with incidence on total mass (drag force penality)
    };
    Propellant propellant;

    struct Engine
    {
        double thrust; // 0 by default
        double power;
        bool burst; // false by default
        double acceleration_rate;
        double decceleration_rate;
        double quality; // better quality -> less nozzle size, with incidence on total mass (drag force penality)
        int burster; // for animation purpose
    };
    Engine engine;

    struct Rotator
    {
        inline static const int left{ 0 };
        inline static const int right{ 1 };

        std::array< double, 2 > thrust; // 0 by default
        double power;
        std::array< bool, 2 > burst; // false by default
        double acceleration_rate;
        double decceleration_rate;
        double quality; // better quality -> less nozzle size, with incidence on total mass (drag force penality)
        std::array< int, 2 > burster; // for animation purpose
    };
    Rotator rotator;

    struct Dynamic // dynamic feeding
    {
        double boundingBoxRadius{ 0 };
        Vector headPosition;
        struct Burst
        {
            Vector position;
            double orientation;
        };
        Burst engine;
        std::array< std::array< Burst, 2 >, 2 > rotators;
        double totalMass{ 0 };
    };
    Dynamic dynamic;

    void Reset()
    {
        rotator.burst.at( Rocket::Rotator::right ) = false;
        rotator.burst.at( Rocket::Rotator::left ) = false;
        engine.burst = false;
    }

    void Rotate( const int _direction )
    {
        rotator.burst.at( _direction ) = true;
        Increase( rotator.thrust.at( _direction ), rotator.acceleration_rate, rotator.power );
    }

    void StabilizeRotation()
    {
        Rotate( rotationMomentum > 0 ? Rocket::Rotator::left : Rocket::Rotator::right );
    }

    void ActivateThrust()
    {
        engine.burst = true;
        Increase( engine.thrust, engine.acceleration_rate, engine.power );
    }

    // _rotationAdjustmentRate the bigger the faster (but inertia) the lower the slower (but less inertia)
    void _RotateTo( const double _targetOrientation, const double _rotationAdjustmentRate )
    {
        // compute current normalized orientation:
        const auto normalizedOrientation{ Vector::From( orientation, 1 ).Orientation() };

        // distance must always be between [-Pi,+Pi]:
        auto orientationDistance{ Maths::NormalizeAngle( _targetOrientation - normalizedOrientation ) };

        // reduce spin-effect and rotate:
        const double targetMomentum{ _rotationAdjustmentRate * orientationDistance / Maths::Pi };
        Rotate( rotationMomentum > targetMomentum ? Rocket::Rotator::left : Rocket::Rotator::right );
    }

    void InvertMomentum( const double _rotationAdjustmentRate )
    {
        _RotateTo( momentum.Orientation(), _rotationAdjustmentRate );
    }

    void Acquire( const Rocket & _target, const double _rotationAdjustmentRate, const Vector & _positionCompensation = {} )
    {
        const auto targetPosition{ _target.position + _positionCompensation };
        // over-compensate positions with momentum:
        double compensationRate{ 1.0 };
        const double distance{ ( targetPosition - position ).Distance() };
        const double distanceCompensationTrigger{ 500 }; // 500 is "close"
        const double maxCompensationRate{ 10 };
        if( distance < distanceCompensationTrigger )
            compensationRate = maxCompensationRate * ( distanceCompensationTrigger - distance ) / distanceCompensationTrigger;
        const auto compensatedPosition{ position + ( momentum * compensationRate ) };
        const auto compensatedTargetPosition{ targetPosition + ( _target.momentum * compensationRate ) };
        _RotateTo( ( compensatedPosition - compensatedTargetPosition ).Orientation(), _rotationAdjustmentRate );
    }

    void Update()
    {
        // propellant consumption:
        const double propellantConsumption{ engine.thrust + rotator.thrust.at( Rotator::left ) + rotator.thrust.at( Rotator::right ) };
        const double consumptionFactor{ 0.05 };
        Decrease( propellant.value, propellantConsumption * consumptionFactor );
        if( propellant.value == 0 ) {
            engine.burst = false;
            rotator.burst.at( Rotator::left ) = false;
            rotator.burst.at( Rotator::right ) = false;
        }
        
        // propellant continuous production:
        Increase( propellant.value, propellant.production_rate, propellant.capacity );
        
        // shield repair:
        Increase( shield.value, shield.repair_rate, shield.capacity );
        
        // rotator:
        if( !rotator.burst.at( Rotator::left ) )
            Decrease( rotator.thrust.at( Rotator::left ), rotator.decceleration_rate );
        if( !rotator.burst.at( Rotator::right ) )
            Decrease( rotator.thrust.at( Rotator::right ), rotator.decceleration_rate );
        rotationMomentum += rotator.thrust.at( Rotator::right ) - rotator.thrust.at( Rotator::left );
        orientation += rotationMomentum;

        // engine:
        if( !engine.burst )
            Decrease( engine.thrust, engine.decceleration_rate );
        const auto thrust{ Vector::From( orientation + Maths::Pi, engine.thrust ) };
        thrustMotion += thrust;

        // compute and substract drag force to motion (opposite to motion, depending of all drag force penalties):
        dynamic.totalMass =  shield.capacity * ( 1 - shield.quality ) + propellant.capacity * ( 1 - propellant.quality ) +
            engine.power * ( 1 - engine.quality ) + 2 * ( rotator.power * ( 1 - rotator.quality ) );
        const double dragForce{ 1.0 / dynamic.totalMass }; // totalMass == 0 -> no drag force;
        
        // update momentum and absolute position:
        momentum = thrustMotion * dragForce;
        position += momentum;
    }

    void Draw( cairo_t & _cairo, const Vector & _translation )
    {
        const auto translated{ position + _translation };

        // general stroke:
        const double strokeWidth{ 3 };

        // draw tank:
        const double propellantRadius{ propellant.capacity * ( 1 - propellant.quality ) };
        const double containerStateMargin{ 4 };
        const double tankRadius{ propellantRadius + containerStateMargin };
        Graphics::Stroke( _cairo, translated, tankRadius, Graphics::arcFull, color, strokeWidth );
        
        // draw shielded body:
        const double bodyStrokeWidth{ shield.capacity * ( 1 - shield.quality ) };
        const double bodyTankMargin{ 4 };
        const double bodyRadius{ tankRadius + bodyTankMargin + ( bodyStrokeWidth * 0.5 ) };
        const double shieldState{ shield.value / shield.capacity };
        const bool shieldUnder25Percent{ shield.value < ( shield.capacity * 0.25 ) };
        static int shieldAlert{ 0 };
        const Graphics::Color shieldColor{ ( shieldUnder25Percent ? ( ( shieldAlert++ / 4 ) % 2 == 0 ? 1.0 : 0.0 ) : 1.0 ), shieldState, shieldState };
        Graphics::Stroke( _cairo, translated, bodyRadius, { orientation + Maths::PiHalf - Maths::PiQuarter, orientation - Maths::PiHalf + Maths::PiQuarter }, shieldColor, bodyStrokeWidth );

        // bounding box:
        dynamic.boundingBoxRadius = bodyRadius + bodyStrokeWidth * 0.5;
        
        // draw head:
        const double headRadius{ 5 };
        dynamic.headPosition = Vector::From( orientation + Maths::Pi, tankRadius + bodyTankMargin + bodyStrokeWidth );
        Graphics::Fill( _cairo, translated + dynamic.headPosition, headRadius, { orientation + Maths::PiHalf, orientation - Maths::PiHalf }, color );
         
        // draw nozzle:
        const double powerFactor{ 100 * ( 1 - engine.quality ) };
        const double nozzleRadius{ powerFactor * engine.power };
        const auto nozzlePosition{ Vector::From( orientation, tankRadius + nozzleRadius ) };
        Graphics::Stroke( _cairo, translated + nozzlePosition, nozzleRadius, { orientation + Maths::PiHalf, orientation - Maths::PiHalf }, color, strokeWidth );
        
        // draw propellant state:
        const double propellantState{ propellant.value / propellant.capacity };
        const bool propellantUnder25Percent{ propellant.value < ( propellant.capacity * 0.25 ) };
        static int propellantAlert{ 0 };
        const Graphics::Color propellantColor{ ( propellantUnder25Percent ? ( ( propellantAlert++ / 4 ) % 2 == 0 ? 1.0 : 0.0 ) : 1.0 ), propellantState, propellantState };
        Graphics::Fill( _cairo, translated, propellantRadius, Graphics::arcFull, propellantColor );

        // draw nozzle flame:
        if( engine.thrust != 0 ) { // only if there's some thrust
            if( engine.burster++ % ( engine.burst ? 2 : 4 ) == 0 ) { // flickers quickly when bursting, slowing when deccelerating
                const double nozzleBurstMargin{ -3 };
                const double burstMaxRadius{ nozzleRadius + nozzleBurstMargin };
                const double thrustRatio{ engine.thrust / engine.power };
                const double burstRadius{ burstMaxRadius * thrustRatio };
                dynamic.engine.orientation = orientation;
                const auto burstPosition{ Vector::From( orientation, tankRadius + nozzleRadius * thrustRatio ) };
                dynamic.engine.position = Vector::From( orientation, tankRadius + 4 );
                Graphics::Fill( _cairo, translated + burstPosition, burstRadius, Graphics::arcFull, { 1, Random( 0, 1 ), 0 } );
            }
        }

        // draw rotators:
        const double rotatorPowerFactor{ 500 * ( 1 - rotator.quality ) };
        const double rotatorMaxRadius{ rotatorPowerFactor * rotator.power };
        const double displacement{ Maths::Pi * 0.09 };
        if( rotator.thrust.at( Rotator::left ) ) {
            if( rotator.burster.at( Rotator::left )++ % ( rotator.burst.at( Rotator::left ) ? 2 : 4 ) == 0 ) { // flickers quickly when bursting, slowing when deccelerating
                const double thrustRatio{ rotator.thrust.at( Rotator::left ) / rotator.power };
                const double rotatorRadius{ rotatorMaxRadius * thrustRatio };
                const double rotatorInitialDistance{ tankRadius + bodyTankMargin + bodyStrokeWidth };
                const double rotatorDistance{ rotatorInitialDistance + rotatorRadius };
                auto & burstLeft{ dynamic.rotators.at( Rotator::left ).at( Rotator::left ) };
                burstLeft.orientation = orientation + Maths::PiHalf;
                const auto burstLeftPosition{ Vector::From( burstLeft.orientation + displacement, rotatorDistance ) };
                burstLeft.position = Vector::From( burstLeft.orientation + displacement, rotatorInitialDistance + 1 );
                Graphics::Fill( _cairo, translated + burstLeftPosition, rotatorRadius, Graphics::arcFull, { 1, Random( 0, 0.5 ), 0 } );
                auto & burstRight{ dynamic.rotators.at( Rotator::left ).at( Rotator::right ) };
                burstRight.orientation = orientation - Maths::PiHalf;
                const auto burstRightPosition{ Vector::From( burstRight.orientation + displacement, rotatorDistance ) };
                burstRight.position = Vector::From( burstRight.orientation + displacement, rotatorInitialDistance + 1 );
                Graphics::Fill( _cairo, translated + burstRightPosition, rotatorRadius, Graphics::arcFull, { 1, Random( 0, 1 ), 0 } );
            }
        }
        if( rotator.thrust.at( Rotator::right ) ) {
            static int burster{ 0 };
            if( rotator.burster.at( Rotator::right )++ % ( rotator.burst.at( Rotator::right ) ? 2 : 4 ) == 0 ) { // flickers quickly when bursting, slowing when deccelerating
                const double thrustRatio{ rotator.thrust.at( Rotator::right ) / rotator.power };
                const double rotatorRadius{ rotatorMaxRadius * thrustRatio };
                const double rotatorInitialDistance{ tankRadius + bodyTankMargin + bodyStrokeWidth };
                const double rotatorDistance{ rotatorInitialDistance + rotatorRadius };
                auto & burstRight{ dynamic.rotators.at( Rotator::right ).at( Rotator::right ) };
                burstRight.orientation = orientation - Maths::PiHalf;
                const auto burstRightPosition{ Vector::From( burstRight.orientation - displacement, rotatorDistance ) };
                burstRight.position = Vector::From( burstRight.orientation - displacement, rotatorInitialDistance + 1 );
                Graphics::Fill( _cairo, translated + burstRightPosition, rotatorRadius, Graphics::arcFull, { 1, Random( 0, 0.5 ), 0 } );
                auto & burstLeft{ dynamic.rotators.at( Rotator::right ).at( Rotator::left ) };
                burstLeft.orientation = orientation + Maths::PiHalf;
                const auto burstLeftPosition{ Vector::From( burstLeft.orientation - displacement, rotatorDistance ) };
                burstLeft.position = Vector::From( burstLeft.orientation - displacement, rotatorInitialDistance + 1 );
                Graphics::Fill( _cairo, translated + burstLeftPosition, rotatorRadius, Graphics::arcFull, { 1, Random( 0, 0.5 ), 0 } );
            }
        }
    }
};


class StarField
{
private:
    struct Layer
    {
        int density;
        double speedMin, speedMax;
        double colorMin, colorMax;
        double radiusMin, radiusMax;
    };

    struct Star
    {
        double x, y;
        double speed;
        double c;
        double radius;
    };

public:
    StarField( const size_t _windowWidth, const size_t _windowHeight )
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

public:
    void Draw( cairo_t & _cairo, const Vector & _speed )
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

private:
    const size_t m_windowWidth;
    const size_t m_windowHeight;
    std::random_device m_rndDev;
    std::mt19937 m_rnd;
    const size_t m_dimension;
    std::uniform_int_distribution< std::mt19937::result_type > m_rndPos;
    std::vector< Star > m_field;
};


struct Enemy
{
    Rocket rocket;
    int shotRate;
};


struct Laser
{
    Vector position;
    Vector momentum;
    double damage;
    int lifeSpan{ 0 };
    int maxLifeSpan{ 20 };

    struct Dynamic
    {
        Vector positionA;
        Vector positionB;
    };
    Dynamic dynamic;

    void Update()
    {
        position += momentum;
    }

    void Draw( cairo_t & _cairo, const Vector & _translation )
    {
        dynamic.positionA = position;
        dynamic.positionB = dynamic.positionA + momentum;
        const auto lifeRatio{ static_cast< double >( lifeSpan ) / static_cast< double >( maxLifeSpan ) };
        Graphics::Line( _cairo, dynamic.positionA + _translation, dynamic.positionB + _translation, Graphics::FireColor( lifeRatio ), 3, false );
    }
};


struct Missile
{
    Rocket rocket;
    bool targetShip;
    Rocket * pOrigin;
    bool bypassCollision{ false };
    int lifeSpan{ 0 };
};


struct Particule
{
    Vector position;
    Vector momentum;
    double lifeSpan;
    double width;
};


class Renderer
{
public:
    Renderer( cairo_surface_t & _cairoSurface, cairo_t & _cairo, const size_t _windowWidth, const size_t _windowHeight )
        : m_cairoSurface{ _cairoSurface }
        , m_cairo{ _cairo }
        , m_windowWidth{ _windowWidth }
        , m_windowHeight{ _windowHeight }
        , m_starField{ _windowWidth, _windowHeight }
        , m_ship{ { 0.5, 0.75, 1 },  {}, Maths::PiHalf, {}, {}, 0,
            5, // damage
            { 20, 20, 0.01, 0.9 }, // shield
            { 20, 20, 0.01, 0.75 }, // propellant
            { 0, 0.5, false, 0.005, 0.01, 0.75, 0 }, // engine
            { { 0, 0 }, 0.01, { false, false }, 0.001, 0.005, 0, { 0, 0 } } } // rotators
    {
        for( int i{ 0 }; i < 5; i++ )
            _AddEnemy();
    }

private:
    void _AddEnemy()
    {
        const auto minDistance{ Random( 0.25, 0.75 ) * static_cast< double >( max( m_windowWidth, m_windowHeight ) ) };
        m_enemies.emplace_back( Rocket{ { 1, 0.5, 0.75 }, m_ship.position + Vector::From( Random( 0, Maths::Pi2 ), minDistance ), Random( 0, Maths::Pi2 ), {}, {}, 0,
                5, // damage
                { 5, 5, 0.001, 0.2 }, // shield
                { 10, 10, 0.05, Random( 0.1, 0.75 ) }, // propellant
                { 0, Random( 0.1, 0.5 ), false, 0.005, 0.01, Random( 0.2, 0.75 ), 0 }, // engine
                { { 0, 0 }, 0.01, { false, false }, 0.001, 0.005, 0, { 0, 0 } } }, // rotators
                static_cast< int >( Random( 0, 60 * 5 ) ) );
    }

public:
    void Loop( std::stop_token _token )
    {
        // main loop:
        unsigned long long fpsLastSnapshot{ 0 };
        unsigned long long fpsFrameCount{ 0 };
        while( !_token.stop_requested() ) {
            const auto temper{ m_precisionTime.Temper( 60, [ this ]( const int _consumptionPercent ){
                    m_frameConsumption = _consumptionPercent;
                }, [ this ]{
                    static bool firstFrame{ true };
                    m_frameDropAlert = firstFrame ? 0 : 60; // 1 seconds alert
                    firstFrame = false;
                } ) }; // 60fps

            // fps management:
            const auto currentTime{ temper.GetStartTime() };
            const auto elpasedTime{ currentTime - fpsLastSnapshot };
            // update every second:
            if( elpasedTime >= 1'000'000'000 ) {
                m_fps = static_cast< double >( fpsFrameCount * 1'000'000'000 ) / elpasedTime;
                fpsLastSnapshot = currentTime;
                fpsFrameCount = 0;
            }
            fpsFrameCount++;

            // reset:
            _Reset();

            // key mapping:
            _Keys();

            // update:
            _Update();

            // paint background:
            cairo_set_source_rgb( &m_cairo, 0, 0.05, 0.1 );
            cairo_paint( &m_cairo );

            // draw:
            _Draw();

            // flush:
            cairo_surface_flush( &m_cairoSurface );
        }
    }

    void KeyPress( const unsigned long _key, const bool _pressed )
    {
        m_keyPressed.at( static_cast< unsigned char >( _key ) ) = _pressed;
    }

    bool _KeyPressed( const unsigned long _key )
    {
        return m_keyPressed.at( static_cast< unsigned char >( _key ) );
    }

private:
    Rocket * _ClosestEnemy( const Vector & _position )
    {
        Rocket * pTarget{ nullptr };
        double minDistance{ std::numeric_limits< double >::infinity() };
        for( auto & enemy : m_enemies ) {
            const auto distance{ ( enemy.rocket.position - _position ).Distance() };
            if( distance >= minDistance )
                continue;
            minDistance = distance;
            pTarget = &enemy.rocket;
        }
        return pTarget;
    }

    void _Keys()
    {
        // stabilize rotation:
        if( _KeyPressed( VK_DOWN ) )
            m_ship.StabilizeRotation();
        else
        // invert momentum:
        if( _KeyPressed( VK_LCONTROL ) || _KeyPressed( VK_RCONTROL ) )
            m_ship.InvertMomentum( 0.2 );
        else
        // change ship orientation:
        if( _KeyPressed( VK_RIGHT ) )
            m_ship.Rotate( Rocket::Rotator::right );
        else
        if( _KeyPressed( VK_LEFT ) )
            m_ship.Rotate( Rocket::Rotator::left );

        // activate ship burst:
        if( _KeyPressed( VK_UP ) )
            m_ship.ActivateThrust();

        // acquire closest target:
        m_pTarget = nullptr;
        if( _KeyPressed( VK_LSHIFT ) || _KeyPressed( VK_RSHIFT ) ) {
            m_pTarget = _ClosestEnemy( m_ship.position );
            if( m_pTarget != nullptr )
                m_ship.Acquire( *m_pTarget, 0.5 );
        }

        // laser:
        static bool alternateLazerPosition{ true };
        static int laserShotRate{ 0 };
        if( laserShotRate++ > 5 && _KeyPressed( VK_SPACE ) )
        {
            alternateLazerPosition = !alternateLazerPosition;
            const auto momentum{ Vector::From( m_ship.orientation + Maths::Pi, 50 ) }; // 50px length
            const auto position{ Vector::From( m_ship.orientation + Maths::PiHalf * ( alternateLazerPosition ? 1 : -1 ), m_ship.dynamic.boundingBoxRadius ) };
            m_lasers.emplace_back( Laser{ m_ship.position - momentum + m_ship.dynamic.headPosition + position, momentum,
                0.2 } ); // damage
            laserShotRate = 0;
        }
        
        // TODO non-moving celestial object (mines)

        // shoot missile:
        static int missileShotRate{ 0 };
        if( missileShotRate++ > 35 && _KeyPressed( VK_SPACE ) ) {
            missileShotRate = 0;
            m_missiles.emplace_back( Rocket{ { 0.5, 0.75, 1 }, m_ship.position, m_ship.orientation, {}, m_ship.momentum, 0,
                3, // damage
                { 1, 1, 0.01, 0.5 }, // shield
                { 10, 10, 0.005, 0.9 }, // propellant
                { 0, 0.5, false, 0.01, 0.05, 0.8, 0 }, // engine
                //{ { 0, 0 }, 0, { false, false }, 0, 0, 0, { 0, 0 } } } ); // TODO non-guided missiles rotator?
                { { 0, 0 }, 0.01, { false, false }, 0.001, 0.005, 0.5, { 0, 0 } } }, false, &m_ship ); // guided missiles rotator
        }
    }

    void _Reset()
    {
        // reset enemies data:
        for( auto & enemy : m_enemies )
            enemy.rocket.Reset();

        // reset missiles data:
        for( auto & missile : m_missiles )
            missile.rocket.Reset();

        // reset ship data:
        m_ship.Reset();
    }

    void _AddParticule( const Vector & _position, const Vector & _direction, const double _orientation, const double _speed, const double _size )
    {
        const auto momentum{ Vector::From( _orientation, _speed ) + _direction };
        m_particules.emplace_back( Particule{ _position, momentum, Random( 1.5, 3 ), _size } );
    }

    inline static const int bigExplosion{ 200 };
    inline static const int mediumExplosion{ 200 };
    inline static const int smallExplosion{ 20 };
    void _AddExplosion( const Vector & _position, const Vector & _direction, const int _count = mediumExplosion )
    {
        for( int i{ 0 }; i < _count; i++ )
            _AddParticule( _position, _direction, Random( 0, Maths::Pi2 ), Random( 0, 4 ), Random( 0.5, 3 ) );
    }

    void _AddEngineParticules( const Vector & _position, const Rocket::Dynamic::Burst & _burst, const double _rate, const int _maxPass, const double _limiter )
    {
        const int passCount{ _rate < 0.5 ? 1 : _maxPass };
        const double maxSize{ 0.5 + _rate * _limiter }; // [0.5, 0.5 to 1.5]
        const double maxSpeed{ 1 + _rate }; // [0.5, 1 to 2]
        const double maxWideness{ 0.25 + _rate * 0.75 * _limiter }; // [-0.25, +0.25] to [-1, +1]
        for( int i{ 0 }; i < passCount; i++ )
            _AddParticule( _position + _burst.position, {}, _burst.orientation + Random( -maxWideness, maxWideness ), Random( 0.5, maxSpeed ), Random( 0.5, maxSize ) );
    }
    
    void _AddEnginesParticules( const Rocket & _rocket )
    {
        if( _rocket.engine.thrust != 0 )
            _AddEngineParticules( _rocket.position, _rocket.dynamic.engine, _rocket.engine.thrust / _rocket.engine.power, 2, 1 );
        const auto & leftThrust{ _rocket.rotator.thrust.at( Rocket::Rotator::left ) };
        if( leftThrust != 0 ) {
            const auto & rotator{ _rocket.dynamic.rotators.at( Rocket::Rotator::left ) };
            _AddEngineParticules( _rocket.position, rotator.at( Rocket::Rotator::left ), leftThrust / _rocket.rotator.power, 1, 0.5 );
            _AddEngineParticules( _rocket.position, rotator.at( Rocket::Rotator::right ), leftThrust / _rocket.rotator.power, 1, 0.5 );
        }
        const auto & rightThrust{ _rocket.rotator.thrust.at( Rocket::Rotator::right ) };
        if( rightThrust != 0 ) {
            const auto & rotator{ _rocket.dynamic.rotators.at( Rocket::Rotator::right ) };
            _AddEngineParticules( _rocket.position, rotator.at( Rocket::Rotator::left ), rightThrust / _rocket.rotator.power, 1, 0.5 );
            _AddEngineParticules( _rocket.position, rotator.at( Rocket::Rotator::right ), rightThrust / _rocket.rotator.power, 1, 0.5 );
        }
    }

    bool _RocketCollision( const Rocket & _a, const Rocket & _b )
    {
        return Maths::Collision( _a.position, _a.dynamic.boundingBoxRadius, _b.position, _b.dynamic.boundingBoxRadius );
    }

    void _TransmitMomentum( const Vector & _position, const Vector & _momentum, const double _impact, Rocket & _b )
    {
        _b.thrustMotion += _momentum;
        const auto normalizedEnemyOrientation{ Vector::From( _b.orientation, 1 ).Orientation() };
        const auto impactOrientation{ ( _position - _b.position ).Orientation() };
        _b.rotationMomentum += _impact * Maths::NormalizeAngle( normalizedEnemyOrientation - impactOrientation );
    }

    void _TransmitMomentum( Rocket & _a, Rocket & _b )
    {
        _TransmitMomentum( _a.position, _a.momentum, 0.05 * ( _a.dynamic.totalMass / 15 ), _b );
    }

    void _LaserImpact( Laser & _a, Rocket & _b )
    {   
        const auto momentum{ _a.momentum * 0.01 };
        // transmit momentum:
        _TransmitMomentum( _a.position, momentum, 0.05, _b );
        // add small impact explosion on target:
        _AddExplosion( _b.position, Vector{} - momentum, smallExplosion );
        // shield impact:
        _b.shield.value -= _a.damage;
    }

    void _RocketImpact( Rocket & _a, Rocket & _b )
    {   
        // transmit momentum:
        _TransmitMomentum( _a, _b );
        // add small impact explosion on target:
        _AddExplosion( _b.position, Vector{} - _a.momentum, smallExplosion );
        // shield impact:
        _b.shield.value -= _a.damage;
    }

    bool _LaserRocketCollision( Laser & _laser, Rocket & _other )
    {
        if( !Maths::Collision( _other.position, _other.dynamic.boundingBoxRadius, _laser.dynamic.positionA, _laser.dynamic.positionB ) )
            return false;
        _LaserImpact( _laser, _other );
        return true;
    }

    bool _MissileRocketCollision( Missile & _missile, Rocket & _other )
    {
        // prevent collision when launched:
        if( _missile.pOrigin == &_other && !_missile.bypassCollision ) {
            if( !Maths::Collision( _missile.rocket.position, _missile.rocket.dynamic.boundingBoxRadius, _other.position, _other.dynamic.boundingBoxRadius ) )
                _missile.bypassCollision = true;
            return false;
        }
        if( !_RocketCollision( _missile.rocket, _other ) )
            return false;
        _RocketImpact( _missile.rocket, _other );
        return true;
    }

    void _Update()
    {        
        // TODO use mouse to move?
        
        // TODO solar wind (waves)
        // TODO planets and gravity attraction

        // enemise collision:
        for( auto & enemy : m_enemies ) {
            // enemies-enemies collisions:
            for( auto & enemyOther : m_enemies )
                if( &enemy != &enemyOther )
                    if( _RocketCollision( enemy.rocket, enemyOther.rocket ) )
                        _RocketImpact( enemy.rocket, enemyOther.rocket );
            // enemy-ship collision:
            if( _RocketCollision( enemy.rocket, m_ship ) ) {
                _RocketImpact( enemy.rocket, m_ship );
                _RocketImpact( m_ship, enemy.rocket );
            }
        }

        // laser collision:
        for( auto & laser : m_lasers ) {
            bool collision{ false };
            // laser-ships collisions:
            for( auto itEnemy{ m_enemies.begin() }, itEnemyEnd{ m_enemies.end() }; !collision && itEnemy != itEnemyEnd; ++itEnemy )
                collision = _LaserRocketCollision( laser, itEnemy->rocket );
            // laser-missiles collisions:
            for( auto itMissile{ m_missiles.begin() }; !collision && itMissile != m_missiles.end(); ) {
                if( itMissile->pOrigin != &m_ship ) // laser don't destroy ship's missiles
                    if( _LaserRocketCollision( laser, itMissile->rocket ) ) {
                        collision = true;
                        _AddExplosion( laser.position, itMissile->rocket.momentum );
                        itMissile = m_missiles.erase( itMissile );
                        continue;
                    }
                ++itMissile;
            }
            if( collision )
                laser.lifeSpan = laser.maxLifeSpan;
        }
        
        // missiles collision:
        for( auto it{ m_missiles.begin() }; it != m_missiles.end(); ) {
            bool collision{ false };
            // missiles-missiles collisions:
            for( auto itMissile{ m_missiles.begin() }; !collision && itMissile != m_missiles.end(); ) {
                if( it != itMissile )
                    if( _RocketCollision( it->rocket, itMissile->rocket ) ) {
                        collision = true;
                        _AddExplosion( itMissile->rocket.position, itMissile->rocket.momentum );
                        itMissile = m_missiles.erase( itMissile );
                        continue;
                    }
                ++itMissile;
            }
            // missiles-enemies collisions:
            for( auto itEnemy{ m_enemies.begin() }, itEnemyEnd{ m_enemies.end() }; !collision && itEnemy != itEnemyEnd; ++itEnemy )
                collision = _MissileRocketCollision( *it, itEnemy->rocket );
            // missiles-ship collision:
            if( !collision )
                collision = _MissileRocketCollision( *it, m_ship );
            // explode and remove:
            if( collision ) {
                _AddExplosion( it->rocket.position, it->rocket.momentum );
                it = m_missiles.erase( it );
                continue;
            }
            ++it;
        }        
        
        // update enemies data:
        int newEnemiesToGenerate{ 0 };
        for( auto it{ m_enemies.begin() }; it != m_enemies.end(); ) {
            // better NOT aim directly the ship to avoid collisions:
            it->rocket.Acquire( m_ship, 0.5, Vector::From( m_ship.orientation + Maths::Pi, 100 ) );
            it->rocket.ActivateThrust();
            it->rocket.Update();

            // shield:
            if( it->rocket.shield.value <= 0 ) {
                _AddExplosion( it->rocket.position, it->rocket.momentum, bigExplosion );
                newEnemiesToGenerate++;
                it = m_enemies.erase( it );
                continue;
            }

            // enemies launch rockets aiming the ship:
            if( it->shotRate++ > 60 * 5 ) { // every 5 seconds
                it->shotRate = 0;
                m_missiles.emplace_back( Rocket{ { 1, 0.5, 0.75 }, it->rocket.position, it->rocket.orientation, {}, it->rocket.momentum, 0,
                    3, // damage
                    { 1, 1, 0.01, 0.5 }, // shield
                    { 10, 10, 0.01, 0.9 }, // propellant
                    { 0, 0.5, false, 0.01, 0.05, 0.8, 0 }, // engine
                    { { 0, 0 }, 0.01, { false, false }, 0.001, 0.005, 0.5, { 0, 0 } } }, true, &it->rocket ); // guided missiles rotator
            }
            ++it;
        }
        for( int i{ 0 }; i < newEnemiesToGenerate; i++ )
            _AddEnemy();

        // update laser data:
        for( auto it{ m_lasers.begin() }; it != m_lasers.end(); ) {
            if( it->lifeSpan++ > it->maxLifeSpan ) {
                it = m_lasers.erase( it );
                continue;
            }
            // regular update:
            it->Update();
            ++it;
        }

        // update missiles data:
        for( auto it{ m_missiles.begin() }; it != m_missiles.end(); ) {
            // running:
            if( it->lifeSpan == 0 ) {
                // acquiring proper target:
                const auto pTarget{ it->targetShip ? &m_ship : _ClosestEnemy( it->rocket.position ) };
                if( pTarget != nullptr ) {
                    it->rocket.Acquire( *pTarget, 0.1 );
                    it->rocket.ActivateThrust();
                }
                // stopping when out of propellant:
                if( it->rocket.propellant.value <= it->rocket.propellant.production_rate )
                    it->lifeSpan = 1;
            }
            // stopped:
            else {
                it->lifeSpan++;
                // explode and remove after one second of drift:
                if( it->lifeSpan == 60 ) {
                    _AddExplosion( it->rocket.position, it->rocket.momentum );
                    it = m_missiles.erase( it );
                    continue;
                }
            }
            // regular update:
            it->rocket.Update();
            ++it;
        }

        // TODO deal with shield / potential explosion/removal (game-over)

        // update ship data:
        m_ship.Update();

        // add particules:
        for( auto & enemy : m_enemies )
            _AddEnginesParticules( enemy.rocket );
        for( auto & missile : m_missiles )
            _AddEnginesParticules( missile.rocket );
        _AddEnginesParticules( m_ship );

        // update particules data:
        for( auto it{ m_particules.begin() }; it != m_particules.end(); ) {
            it->lifeSpan -= 0.09; // quickly reduce particules lifespan
            if( it->lifeSpan <= 0 ) {
                it = m_particules.erase( it );
                continue;
            }
            it->position += it->momentum;
            ++it;
        }
    }

    void _Draw()
    {
        // display frame rate information:
        if( m_frameCounter++ % 30 == 0 ) // refresh every 500ms
            m_freezedFrameConsumption = m_frameConsumption;
        std::stringstream sFps;
        sFps << "fps: " << std::setprecision( 3 ) << m_fps;
        Graphics::Text( m_cairo, { 2, 2 + 14 }, 14, { 1, 1, 1 }, sFps.str() );
        if( m_frameDropAlert == 0 )
            Graphics::Text( m_cairo, { 64, 2 + 14 }, 14, { 0.5, 0.75, 1 }, "(" + std::to_string( m_freezedFrameConsumption ) + "%)" );
        else {
            m_frameDropAlert--;
            Graphics::Text( m_cairo, { 64, 2 + 14 }, 14, { 1, 0.5, 0.5 }, "[frame drop]" );
        }

        // TODO information display (what?)
        // - current ship information (shield, propeller & engines classes)
        // - current target information
        // - fuel & shield alerts
        
        // draw starfield, related to ship motion:
        m_starField.Draw( m_cairo, m_ship.momentum * 2 );

        const Vector screenCenter{ static_cast< double >( m_windowWidth ) / 2, static_cast< double >( m_windowHeight ) / 2 };
        const Vector shipPosition{ screenCenter - m_ship.position };

        // draw particules:
        for( auto & particule : m_particules )
            Graphics::Fill( m_cairo, particule.position + shipPosition, particule.width, Graphics::arcFull, Graphics::FireColor( ( 3 - particule.lifeSpan ) / 3 ) );

        // draw enemies:
        for( auto & enemy : m_enemies ) {
            if( m_pTarget == &enemy.rocket ) {
                Graphics::Line( m_cairo, screenCenter, enemy.rocket.position + shipPosition, { 1, 0.5, 0.1 }, 0.5, true );
                const auto vector{ enemy.rocket.position - m_ship.position };
                const auto distance{ static_cast< int >( vector.Distance() ) };
                if( distance > 500 ) { // 500 is "close"
                    const auto position{ Vector::From( vector.Orientation(), m_ship.dynamic.boundingBoxRadius + 50 ) };
                    Graphics::Text( m_cairo, position + screenCenter, 12, { 1, 0.75, 0.5 }, std::to_string( distance ) );
                }
            }
            enemy.rocket.Draw( m_cairo, shipPosition );
        }

        // draw lasers:
        for( auto & laser : m_lasers )
            laser.Draw( m_cairo, shipPosition );

        // draw missiles:
        for( auto & missile : m_missiles )
            missile.rocket.Draw( m_cairo, shipPosition );

        // draw ship:
        m_ship.Draw( m_cairo, shipPosition );
    }

private:
    PrecisionTime m_precisionTime;
    cairo_surface_t & m_cairoSurface;
    cairo_t & m_cairo;
    const size_t m_windowWidth;
    const size_t m_windowHeight;

private:
    std::array< std::atomic< bool >, 255 > m_keyPressed;
    int m_frameCounter{ 0 };
    double m_fps{ 0 };
    int m_frameConsumption{ 0 };
    int m_freezedFrameConsumption{ 0 };
    int m_frameDropAlert{ 0 };

private:
    StarField m_starField;
    Rocket m_ship;
    std::vector< Enemy > m_enemies;
    Rocket * m_pTarget{ nullptr };
    std::vector< Laser > m_lasers;
    std::vector< Missile > m_missiles;
    std::vector< Particule > m_particules;
};


int main()
{
    const std::wstring windowName{ L"Starship" };
    const size_t windowWidth{ 1920 };
    const size_t windowHeight{ 1000 };

    // windows init:
    ::HINSTANCE hInstance{ ::GetModuleHandleW( nullptr ) };
    WNDCLASS wndClass{ 0 };
    wndClass.lpfnWndProc = []( HWND _hWnd, UINT _msg, WPARAM _wParam, LPARAM _lParam )
    {
        if( _msg != WM_DESTROY )
            return ::DefWindowProc( _hWnd, _msg, _wParam, _lParam );
        ::PostQuitMessage( 0 );
        return LRESULT{ 0 };
    };
    wndClass.hInstance = hInstance;
    wndClass.lpszClassName = windowName.c_str();
    ::RegisterClassW( &wndClass );
    RECT windowRect{ 0, 0, windowWidth, windowHeight };
    DWORD windowStyle{ WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX };
    DWORD windowExStyle{ WS_EX_APPWINDOW | WS_EX_WINDOWEDGE };
    ::AdjustWindowRectEx( &windowRect, windowStyle, FALSE, windowExStyle );
    HWND hWnd{ ::CreateWindowExW( windowExStyle, windowName.c_str(), windowName.c_str(), windowStyle, 200, 200, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, nullptr, nullptr, hInstance, nullptr ) };
    ::ShowWindow( ::GetConsoleWindow(), SW_HIDE );
    MONITORINFO monitorInfo{ sizeof( MONITORINFO ) };
    ::GetMonitorInfo( MonitorFromWindow( hWnd, MONITOR_DEFAULTTOPRIMARY ), &monitorInfo );
    ::SetWindowPos( hWnd, nullptr, ( monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left - windowWidth ) >> 1,
        ( monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top - windowHeight ) >> 1, 0, 0, SWP_NOSIZE | SWP_NOZORDER );
    ::ShowWindow( hWnd, SW_SHOW );

    // cairo init:
    HDC hDc{ ::GetDC( hWnd ) };
    cairo_surface_t * pCairoSurface{ cairo_win32_surface_create( hDc ) };
    cairo_t * pCairo{ cairo_create( pCairoSurface ) };
    cairo_set_antialias( pCairo, CAIRO_ANTIALIAS_BEST );
        
    // rendering thread:
    Renderer renderer{ *pCairoSurface, *pCairo, windowWidth, windowHeight };
    std::jthread renderThread{ [ & ]( std::stop_token _token ){
            ::SetThreadPriority( ::GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL );
            renderer.Loop( _token );
        } };

    // keyboard hook:
    static const auto fnKeyPress{ [ & ]( const unsigned long _key, const bool _pressed ){
            if( ::GetForegroundWindow() == hWnd )
                renderer.KeyPress( _key, _pressed );
        } };
    static HHOOK keyboardHook{ ::SetWindowsHookEx( WH_KEYBOARD_LL, []( const int _nCode, const WPARAM _wParam, const LPARAM _lParam ) {
            auto press{ ( PKBDLLHOOKSTRUCT ) _lParam };
            if( press->vkCode == 0x0d && ( press->flags & LLKHF_EXTENDED ) != 0 )
		        press->vkCode = 0x0e;
            const bool down{ _wParam == WM_KEYDOWN || _wParam == WM_SYSKEYDOWN };
            if( ( _wParam == WM_KEYDOWN || _wParam == WM_SYSKEYDOWN ) || ( _wParam == WM_KEYUP || _wParam == WM_SYSKEYUP ) )
                fnKeyPress( press->vkCode, down );
            return CallNextHookEx( keyboardHook, _nCode, _wParam, _lParam );
        }, nullptr, 0 ) };

    // dispatch thread:
    ::SetThreadPriority( ::GetCurrentThread(), THREAD_PRIORITY_NORMAL );
    MSG msg;
    while( ::GetMessageW( &msg, nullptr, 0, 0 ) ) {
        ::TranslateMessage( &msg );
        ::DispatchMessageW( &msg );
    }

    // stop keyboard hook:
    ::UnhookWindowsHookEx( keyboardHook );

    // rendering thread stop:
    renderThread.request_stop();
    renderThread.join();

    // cairo release:
    cairo_destroy( pCairo );
    cairo_surface_destroy( pCairoSurface );
    ::ReleaseDC( hWnd, hDc );

    // windows release:
    ::DestroyWindow( hWnd );
    ::UnregisterClassW( windowName.c_str(), hInstance  );

    return 0;
}