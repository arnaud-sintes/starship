#include "Rocket.h"


void Rocket::Reset()
{
    rotator.burst.at( Rocket::Rotator::right ) = false;
    rotator.burst.at( Rocket::Rotator::left ) = false;
    engine.burst = false;
}


void Rocket::Rotate( const int _direction )
{
    rotator.burst.at( _direction ) = true;
    Maths::Increase( rotator.thrust.at( _direction ), rotator.acceleration_rate, rotator.power );
}


void Rocket::StabilizeRotation()
{
    Rotate( rotationMomentum > 0 ? Rocket::Rotator::left : Rocket::Rotator::right );
}


void Rocket::ActivateThrust()
{
    engine.burst = true;
    Maths::Increase( engine.thrust, engine.acceleration_rate, engine.power );
}


// _rotationAdjustmentRate the bigger the faster (but inertia) the lower the slower (but less inertia)
void Rocket::_RotateTo( const double _targetOrientation, const double _rotationAdjustmentRate )
{
    // compute current normalized orientation:
    const auto normalizedOrientation{ Vector::From( orientation, 1 ).Orientation() };

    // distance must always be between [-Pi,+Pi]:
    auto orientationDistance{ Maths::NormalizeAngle( _targetOrientation - normalizedOrientation ) };

    // reduce spin-effect and rotate:
    const double targetMomentum{ _rotationAdjustmentRate * orientationDistance / Maths::Pi };
    if( static_cast< int >( std::abs( rotationMomentum - targetMomentum ) * 1000 ) != 0 )
        Rotate( rotationMomentum > targetMomentum ? Rocket::Rotator::left : Rocket::Rotator::right );
}


void Rocket::InvertMomentum( const double _rotationAdjustmentRate )
{
    _RotateTo( momentum.Orientation(), _rotationAdjustmentRate );
}


void Rocket::PointTo( const Vector & _target, const double _rotationAdjustmentRate, const Vector & _positionCompensation, const Vector & _targetMomentum )
{
     const auto targetPosition{ _target + _positionCompensation };
    // over-compensate positions with momentum:
    double compensationRate{ 1.0 };
    const double distance{ ( targetPosition - position ).Distance() };
    const double distanceCompensationTrigger{ 500 }; // 500 is "close"
    const double maxCompensationRate{ 10 };
    if( distance < distanceCompensationTrigger )
        compensationRate = maxCompensationRate * ( distanceCompensationTrigger - distance ) / distanceCompensationTrigger;
    const auto compensatedPosition{ position + ( momentum * compensationRate ) };
    const auto compensatedTargetPosition{ targetPosition + ( _targetMomentum * compensationRate ) };
    _RotateTo( ( compensatedPosition - compensatedTargetPosition ).Orientation(), _rotationAdjustmentRate );
}


void Rocket::Acquire( const Rocket & _target, const double _rotationAdjustmentRate, const Vector & _positionCompensation )
{
    PointTo( _target.position, _rotationAdjustmentRate, _positionCompensation, _target.momentum );
}


void Rocket::Update()
{
    // propellant consumption:
    const double propellantConsumption{ engine.thrust + rotator.thrust.at( Rotator::left ) + rotator.thrust.at( Rotator::right ) };
    const double consumptionFactor{ 0.05 };
    Maths::Decrease( propellant.value, propellantConsumption * consumptionFactor );
    if( propellant.value == 0 ) {
        engine.burst = false;
        rotator.burst.at( Rotator::left ) = false;
        rotator.burst.at( Rotator::right ) = false;
    }
        
    // propellant continuous production:
    Maths::Increase( propellant.value, propellant.production_rate, propellant.capacity );
        
    // shield repair:
    Maths::Increase( shield.value, shield.repair_rate, shield.capacity );
        
    // rotator:
    if( !rotator.burst.at( Rotator::left ) )
        Maths::Decrease( rotator.thrust.at( Rotator::left ), rotator.decceleration_rate );
    if( !rotator.burst.at( Rotator::right ) )
        Maths::Decrease( rotator.thrust.at( Rotator::right ), rotator.decceleration_rate );
    rotationMomentum += rotator.thrust.at( Rotator::right ) - rotator.thrust.at( Rotator::left );
    orientation += rotationMomentum;

    // engine:
    if( !engine.burst )
        Maths::Decrease( engine.thrust, engine.decceleration_rate );
    const auto thrust{ Vector::From( orientation + Maths::Pi, engine.thrust ) };
    thrustMotion += thrust;

    // solar density thrust retention:
    // note: this is fake, but helps to avoid excessive speeds and momentum...
    if( thrustMotion.Distance() > 0 )
        thrustMotion *= 0.999; // 99.9%

    // compute and substract drag force to motion (opposite to motion, depending of all drag force penalties):
    dynamic.totalMass =  shield.capacity * ( 1 - shield.quality ) + propellant.capacity * ( 1 - propellant.quality ) +
        engine.power * ( 1 - engine.quality ) + 2 * ( rotator.power * ( 1 - rotator.quality ) );
    const double dragForce{ 1.0 / dynamic.totalMass }; // totalMass == 0 -> no drag force;
        
    // update momentum and absolute position:
    momentum = thrustMotion * dragForce;
    position += momentum;
}


// TODO split class
void Rocket::Draw( const NanoVGRenderer::Frame & _frame, const Vector & _translation )
{
    const auto translated{ position + _translation };

    // general stroke:
    const double strokeWidth{ 3 };

    // draw tank:
    const double propellantRadius{ propellant.capacity * ( 1 - propellant.quality ) };
    const double containerStateMargin{ 4 };
    const double tankRadius{ propellantRadius + containerStateMargin };
    _frame.StrokeCircle( translated, tankRadius, color, strokeWidth );
        
    // draw shielded body:
    const double bodyStrokeWidth{ shield.capacity * ( 1 - shield.quality ) };
    const double bodyTankMargin{ 4 };
    const double bodyRadius{ tankRadius + bodyTankMargin + ( bodyStrokeWidth * 0.5 ) };
    const double shieldState{ shield.value / shield.capacity };
    const bool shieldUnder25Percent{ shield.value < ( shield.capacity * 0.25 ) };
    static int shieldAlert{ 0 };
    const Color_d shieldColor{ ( shieldUnder25Percent ? ( ( shieldAlert++ / 4 ) % 2 == 0 ? 1.0 : 0.0 ) : 1.0 ), shieldState, shieldState };
    _frame.StrokeArc( translated, bodyRadius, orientation + Maths::PiHalf - Maths::PiQuarter, orientation - Maths::PiHalf + Maths::PiQuarter, shieldColor, bodyStrokeWidth );

    // bounding box:
    dynamic.boundingBoxRadius = bodyRadius + bodyStrokeWidth * 0.5;
        
    // draw head:
    const double headRadius{ 5 };
    dynamic.headPosition = Vector::From( orientation + Maths::Pi, tankRadius + bodyTankMargin + bodyStrokeWidth );
    _frame.FillArc( translated + dynamic.headPosition, headRadius, orientation + Maths::PiHalf, orientation - Maths::PiHalf, color );
         
    // draw nozzle:
    const double powerFactor{ 100 * ( 1 - engine.quality ) };
    const double nozzleRadius{ powerFactor * engine.power };
    const auto nozzlePosition{ Vector::From( orientation, tankRadius + nozzleRadius ) };
    _frame.StrokeArc( translated + nozzlePosition, nozzleRadius, orientation + Maths::PiHalf, orientation - Maths::PiHalf, color, strokeWidth );
        
    // draw propellant state:
    const double propellantState{ propellant.value / propellant.capacity };
    const bool propellantUnder25Percent{ propellant.value < ( propellant.capacity * 0.25 ) };
    static int propellantAlert{ 0 };
    const Color_d propellantColor{ ( propellantUnder25Percent ? ( ( propellantAlert++ / 4 ) % 2 == 0 ? 1.0 : 0.0 ) : 1.0 ), propellantState, propellantState };
    _frame.FillCircle( translated, propellantRadius, propellantColor );

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
            _frame.FillCircle( translated + burstPosition, burstRadius, { 1, Maths::Random( 0, 1 ), 0 } );
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
            _frame.FillCircle( translated + burstLeftPosition, rotatorRadius, { 1, Maths::Random( 0, 0.5 ), 0 } );
            auto & burstRight{ dynamic.rotators.at( Rotator::left ).at( Rotator::right ) };
            burstRight.orientation = orientation - Maths::PiHalf;
            const auto burstRightPosition{ Vector::From( burstRight.orientation + displacement, rotatorDistance ) };
            burstRight.position = Vector::From( burstRight.orientation + displacement, rotatorInitialDistance + 1 );
            _frame.FillCircle( translated + burstRightPosition, rotatorRadius, { 1, Maths::Random( 0, 1 ), 0 } );
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
            _frame.FillCircle( translated + burstRightPosition, rotatorRadius, { 1, Maths::Random( 0, 0.5 ), 0 } );
            auto & burstLeft{ dynamic.rotators.at( Rotator::right ).at( Rotator::left ) };
            burstLeft.orientation = orientation + Maths::PiHalf;
            const auto burstLeftPosition{ Vector::From( burstLeft.orientation - displacement, rotatorDistance ) };
            burstLeft.position = Vector::From( burstLeft.orientation - displacement, rotatorInitialDistance + 1 );
            _frame.FillCircle( translated + burstLeftPosition, rotatorRadius, { 1, Maths::Random( 0, 0.5 ), 0 } );
        }
    }
}