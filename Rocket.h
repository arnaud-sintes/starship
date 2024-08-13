#pragma once

#include "Graphics.h"


// --------------
// TODO class
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

    void Reset();
    void Rotate( const int _direction );

    void StabilizeRotation();

    void ActivateThrust();

    // _rotationAdjustmentRate the bigger the faster (but inertia) the lower the slower (but less inertia)
    void _RotateTo( const double _targetOrientation, const double _rotationAdjustmentRate );

    void InvertMomentum( const double _rotationAdjustmentRate );

    void Acquire( const Rocket & _target, const double _rotationAdjustmentRate, const Vector & _positionCompensation = {} );

    void Update();

    void Draw( cairo_t & _cairo, const Vector & _translation );
};