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


void Rocket::ReceiveImpact( const Vector & _position, const Vector & _momentum, const double _impact )
{
    thrustMotion += _momentum;
    const auto normalizedOrientation{ Vector::From( orientation, 1 ).Orientation() };
    const auto impactOrientation{ ( _position - position ).Orientation() };
    rotationMomentum += _impact * Maths::NormalizeAngle( normalizedOrientation - impactOrientation );
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
    thrustMotion += dynamic.attraction;

    // solar density thrust retention:
    // note: this is fake, but helps to avoid excessive speeds and momentum...
    if( thrustMotion.DistanceSquared() > 0 )
        thrustMotion *= 0.999; // 99.9%

    // compute and substract drag force to motion (opposite to motion, depending of all drag force penalties):
    dynamic.totalMass = shield.capacity * ( 1 - shield.quality ) + propellant.capacity * ( 1 - propellant.quality ) +
        engine.power * ( 1 - engine.quality ) + 2 * ( rotator.power * ( 1 - rotator.quality ) );
    const double dragForce{ 1.0 / dynamic.totalMass }; // totalMass == 0 -> no drag force;

    // update momentum and absolute position:
    momentum = thrustMotion * dragForce;
    position += momentum;

    // derived geometry (bounding box, burst anchors...) used by collisions, particules and Draw:
    RefreshGeometry();
}


void Rocket::RefreshGeometry()
{
    const double containerStateMargin{ 4 };
    const double bodyTankMargin{ 4 };

    dynamic.propellantRadius = propellant.capacity * ( 1 - propellant.quality );
    dynamic.tankRadius = dynamic.propellantRadius + containerStateMargin;
    dynamic.bodyStrokeWidth = shield.capacity * ( 1 - shield.quality );
    dynamic.bodyRadius = dynamic.tankRadius + bodyTankMargin + ( dynamic.bodyStrokeWidth * 0.5 );
    dynamic.boundingBoxRadius = dynamic.bodyRadius + dynamic.bodyStrokeWidth * 0.5;
    dynamic.headPosition = Vector::From( orientation + Maths::Pi, dynamic.tankRadius + bodyTankMargin + dynamic.bodyStrokeWidth );
    dynamic.nozzleRadius = 100 * ( 1 - engine.quality ) * engine.power;

    // engine burst anchor (particules source):
    dynamic.engine.orientation = orientation;
    dynamic.engine.position = Vector::From( orientation, dynamic.tankRadius + bodyTankMargin );

    // rotators burst anchors:
    const double displacement{ Maths::Pi * 0.09 };
    const double rotatorInitialDistance{ dynamic.tankRadius + bodyTankMargin + dynamic.bodyStrokeWidth };
    auto & leftPair{ dynamic.rotators.at( Rotator::left ) };
    leftPair.at( Rotator::left ).orientation = orientation + Maths::PiHalf;
    leftPair.at( Rotator::left ).position = Vector::From( orientation + Maths::PiHalf + displacement, rotatorInitialDistance + 1 );
    leftPair.at( Rotator::right ).orientation = orientation - Maths::PiHalf;
    leftPair.at( Rotator::right ).position = Vector::From( orientation - Maths::PiHalf + displacement, rotatorInitialDistance + 1 );
    auto & rightPair{ dynamic.rotators.at( Rotator::right ) };
    rightPair.at( Rotator::right ).orientation = orientation - Maths::PiHalf;
    rightPair.at( Rotator::right ).position = Vector::From( orientation - Maths::PiHalf - displacement, rotatorInitialDistance + 1 );
    rightPair.at( Rotator::left ).orientation = orientation + Maths::PiHalf;
    rightPair.at( Rotator::left ).position = Vector::From( orientation + Maths::PiHalf - displacement, rotatorInitialDistance + 1 );
}


void Rocket::Draw( const NanoVGRenderer::Frame & _frame, const Vector & _translation ) const
{
    const auto translated{ position + _translation };

    // general stroke:
    const double strokeWidth{ 3 };

    // draw tank:
    _frame.StrokeCircle( translated, dynamic.tankRadius, color, strokeWidth );

    // draw shielded body:
    const double shieldState{ shield.value / shield.capacity };
    const bool shieldUnder25Percent{ shield.value < ( shield.capacity * 0.25 ) };
    const Color_d shieldColor{ ( shieldUnder25Percent ? ( ( shieldBlink++ / 4 ) % 2 == 0 ? 1.0 : 0.0 ) : 1.0 ), shieldState, shieldState };
    _frame.StrokeArc( translated, dynamic.bodyRadius, orientation + Maths::PiHalf - Maths::PiQuarter, orientation - Maths::PiHalf + Maths::PiQuarter, shieldColor, dynamic.bodyStrokeWidth );

    // draw head:
    const double headRadius{ 5 };
    _frame.FillArc( translated + dynamic.headPosition, headRadius, orientation + Maths::PiHalf, orientation - Maths::PiHalf, color );

    // draw nozzle:
    const auto nozzlePosition{ Vector::From( orientation, dynamic.tankRadius + dynamic.nozzleRadius ) };
    _frame.StrokeArc( translated + nozzlePosition, dynamic.nozzleRadius, orientation + Maths::PiHalf, orientation - Maths::PiHalf, color, strokeWidth );

    // draw propellant state:
    const double propellantState{ propellant.value / propellant.capacity };
    const bool propellantUnder25Percent{ propellant.value < ( propellant.capacity * 0.25 ) };
    const Color_d propellantColor{ ( propellantUnder25Percent ? ( ( propellantBlink++ / 4 ) % 2 == 0 ? 1.0 : 0.0 ) : 1.0 ), propellantState, propellantState };
    _frame.FillCircle( translated, dynamic.propellantRadius, propellantColor );

    // draw nozzle flame:
    if( engine.thrust != 0 ) { // only if there's some thrust
        if( engine.burster++ % ( engine.burst ? 2 : 4 ) == 0 ) { // flickers quickly when bursting, slowing when deccelerating
            const double nozzleBurstMargin{ -3 };
            const double burstMaxRadius{ dynamic.nozzleRadius + nozzleBurstMargin };
            const double thrustRatio{ engine.thrust / engine.power };
            const double burstRadius{ burstMaxRadius * thrustRatio };
            const auto burstPosition{ Vector::From( orientation, dynamic.tankRadius + dynamic.nozzleRadius * thrustRatio ) };
            _frame.FillCircle( translated + burstPosition, burstRadius, { 1, Maths::Random( 0, 1 ), 0 } );
        }
    }

    // draw rotators:
    const double rotatorPowerFactor{ 500 * ( 1 - rotator.quality ) };
    const double rotatorMaxRadius{ rotatorPowerFactor * rotator.power };
    const double displacement{ Maths::Pi * 0.09 };
    const double rotatorInitialDistance{ dynamic.tankRadius + 4 + dynamic.bodyStrokeWidth };
    if( rotator.thrust.at( Rotator::left ) ) {
        if( rotator.burster.at( Rotator::left )++ % ( rotator.burst.at( Rotator::left ) ? 2 : 4 ) == 0 ) { // flickers quickly when bursting, slowing when deccelerating
            const double thrustRatio{ rotator.thrust.at( Rotator::left ) / rotator.power };
            const double rotatorRadius{ rotatorMaxRadius * thrustRatio };
            const double rotatorDistance{ rotatorInitialDistance + rotatorRadius };
            _frame.FillCircle( translated + Vector::From( orientation + Maths::PiHalf + displacement, rotatorDistance ), rotatorRadius, { 1, Maths::Random( 0, 0.5 ), 0 } );
            _frame.FillCircle( translated + Vector::From( orientation - Maths::PiHalf + displacement, rotatorDistance ), rotatorRadius, { 1, Maths::Random( 0, 1 ), 0 } );
        }
    }
    if( rotator.thrust.at( Rotator::right ) ) {
        if( rotator.burster.at( Rotator::right )++ % ( rotator.burst.at( Rotator::right ) ? 2 : 4 ) == 0 ) { // flickers quickly when bursting, slowing when deccelerating
            const double thrustRatio{ rotator.thrust.at( Rotator::right ) / rotator.power };
            const double rotatorRadius{ rotatorMaxRadius * thrustRatio };
            const double rotatorDistance{ rotatorInitialDistance + rotatorRadius };
            _frame.FillCircle( translated + Vector::From( orientation - Maths::PiHalf - displacement, rotatorDistance ), rotatorRadius, { 1, Maths::Random( 0, 0.5 ), 0 } );
            _frame.FillCircle( translated + Vector::From( orientation + Maths::PiHalf - displacement, rotatorDistance ), rotatorRadius, { 1, Maths::Random( 0, 0.5 ), 0 } );
        }
    }
}
