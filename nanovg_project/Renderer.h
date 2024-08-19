#pragma once

#include "Rocket.h"
#include "Laser.h"
#include "Enemy.h"
#include "Missile.h"
#include "StarField.h"
#include "Particule.h"


// --------------
// TODO way too big, move things at the right places (classes)
class Renderer
{
public:
    Renderer( const Win32::Windows & _windows );

private:
    void _AddEnemy();

public:
    void Loop( NanoVGRenderer::Frame & _frame );

private:
    inline static const int bigExplosion{ 200 };
    inline static const int mediumExplosion{ 200 };
    inline static const int smallExplosion{ 20 };

private:
    Rocket * _ClosestEnemy( const Vector & _position );
    void _Keys();
    void _Reset();
    void _AddParticule( const Vector & _position, const Vector & _direction, const double _orientation, const double _speed, const double _size );
    void _AddExplosion( const Vector & _position, const Vector & _direction, const int _count = mediumExplosion );
    void _AddEngineParticules( const Vector & _position, const Rocket::Dynamic::Burst & _burst, const double _rate, const int _maxPass, const double _limiter );
    void _AddEnginesParticules( const Rocket & _rocket );
    bool _RocketCollision( const Rocket & _a, const Rocket & _b );
    void _TransmitMomentum( const Vector & _position, const Vector & _momentum, const double _impact, Rocket & _b );
    void _TransmitMomentum( Rocket & _a, Rocket & _b );
    void _LaserImpact( Laser & _a, Rocket & _b );
    void _RocketImpact( Rocket & _a, Rocket & _b );
    bool _LaserRocketCollision( Laser & _laser, Rocket & _other );
    bool _MissileRocketCollision( Missile & _missile, Rocket & _other );
    void _Update();
    void _Draw( NanoVGRenderer::Frame & _frame );

private:
    const Win32::Windows & m_windows;
    StarField m_starField;
    Rocket m_ship;
    std::list< std::unique_ptr< Enemy > > m_enemies;
    Rocket * m_pTarget{ nullptr };
    std::list< std::unique_ptr< Laser > > m_lasers;
    std::list< std::unique_ptr< Missile > > m_missiles;
    std::list< std::unique_ptr< Particule > > m_particules;
};