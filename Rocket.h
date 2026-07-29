#pragma once

#include "NanoVGRenderer.h"


// --------------
struct Rocket
{
    Color_d color;
    Vector position;
    double orientation{ 0 };

    Vector thrustMotion; // cumulated thrust motion vector

    Vector momentum; // position momentum (thrust motion minus drag force)
    double rotationMomentum{ 0 }; // rotation momentum

    double damage{ 0 };

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
        mutable int burster; // visual flicker only, mutated during const Draw
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
        mutable std::array< int, 2 > burster; // visual flicker only, mutated during const Draw
    };
    Rotator rotator;

    unsigned long long id{ 0 }; // world-unique, used for safe cross-references (e.g. missile origin)
    double consumptionFactor{ 1 }; // propellant consumption modifier (0 during overdrive)

    struct Dynamic // derived state, refreshed by Update()
    {
        double boundingBoxRadius{ 0 };
        Vector headPosition;
        struct Burst
        {
            Vector position;
            double orientation{ 0 };
        };
        Burst engine;
        std::array< std::array< Burst, 2 >, 2 > rotators;
        double totalMass{ 0 };
        Vector attraction;
        // geometry shared between Update() and Draw():
        double propellantRadius{ 0 };
        double tankRadius{ 0 };
        double bodyStrokeWidth{ 0 };
        double bodyRadius{ 0 };
        double nozzleRadius{ 0 };
    };
    Dynamic dynamic;

    void Reset();
    void Rotate( const int _direction );
    void StabilizeRotation();
    void ActivateThrust();
    void InvertMomentum( const double _rotationAdjustmentRate );
    void PointTo( const Vector & _target, const double _rotationAdjustmentRate, const Vector & _positionCompensation = {}, const Vector & _targetMomentum = {} );
    void Acquire( const Rocket & _target, const double _rotationAdjustmentRate, const Vector & _positionCompensation = {} );
    void ReceiveImpact( const Vector & _position, const Vector & _momentum, const double _impact );
    void RefreshGeometry(); // called by Update; also call once at spawn so collisions never see a zero bounding box
    void Update();
    void Draw( const NanoVGRenderer::Frame & _frame, const Vector & _translation ) const;

    // visual-only blink state, mutated during const Draw
    // (public so Rocket stays an aggregate - private data would forbid braced init):
    mutable int shieldBlink{ 0 };
    mutable int propellantBlink{ 0 };

private:
    void _RotateTo( const double _targetOrientation, const double _rotationAdjustmentRate );
};
