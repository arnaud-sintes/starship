#include "StageDirector.h"


namespace
{
    constexpr int introSeconds{ 4 };
    constexpr int clearedSeconds{ 3 };

    using eObjective = MissionDef::eObjective;
    using eType = Enemy::eType;

    // the objectives alternate kill quotas (simple enemies) and survival timers (the
    // heavier hitters show up), while the pressure and the goody tiers ramp up:
    const std::vector< StageDef > & Stages()
    {
        static const std::vector< StageDef > stages{
            { "REXXUS-3B  -  THE MINING GUILD BLOCKADE", {
                "By jumping out of hyperdrive to reach the Rexxus-3b system, you encounter hostile resistance from the",
                "space mining guild who are illegally exploiting the strange attractors energy around the planet Stellis Secura.",
                "",
                "As a faithful member of the Universal Alliance for Peace, you cannot tolerate such a defiance to our glorious",
                "open-despocracy and the uncontestable authority of our galactic emperator Muhammad Silmane XIV!",
                "",
                "Let's teach this gang of small-time smugglers a lesson to remember..." }, {
                //  objective, target, roster, gravity mines, respawn delay (s), attractor density (share of the generated field)
                // one novelty at a time: chasers alone, then the wasps, then the sniper, then the gravity mines
                { eObjective::kill,    10, { { eType::chaser, 2 } },                                              0, 2.5, 0.09 },
                { eObjective::survive, 60, { { eType::chaser, 2 }, { eType::wasp, 1 } },                          0, 2.5, 0.19 },
                { eObjective::kill,    20, { { eType::chaser, 3 }, { eType::wasp, 2 }, { eType::sniper, 1 } },    0, 2.0, 0.31 },
                { eObjective::survive, 90, { { eType::chaser, 3 }, { eType::wasp, 2 }, { eType::sniper, 1 } },   12, 1.5, 0.47 },
                { eObjective::kill,    30, { { eType::chaser, 3 }, { eType::wasp, 2 }, { eType::sniper, 2 } },   32, 1.0, 0.625 },
            }, 1.0 }, // deep space: a dense sky
            { "STELLIS SECURA  -  THE ATTRACTORS BELT", {
                "The blockade is broken, and the planet Stellis Secura now shines ahead of you.",
                "But around it, the strange attractors belt thickens into a maze of gravity wells: the very energy the guild is stealing.",
                "",
                "Alerted, the guild throws its whole fleet into the defense of its precious belt, and its mines drift in the wells.",
                "These smugglers know every current of this place, and will gladly let it crush you.",
                "",
                "Use the wells as shields, keep moving, and bring the glory of the Universal Alliance for Peace to their doorstep!" }, {
                // closing in on the planet: the belt thickens, the guild sends its whole fleet
                { eObjective::kill,    25, { { eType::chaser, 4 }, { eType::wasp, 2 }, { eType::sniper, 1 } },   20, 1.5, 0.7 },
                { eObjective::survive, 90, { { eType::chaser, 3 }, { eType::wasp, 4 }, { eType::sniper, 2 } },   28, 1.2, 0.78 },
                { eObjective::kill,    40, { { eType::chaser, 4 }, { eType::wasp, 3 }, { eType::sniper, 2 } },   32, 1.0, 0.86 },
                { eObjective::survive, 120, { { eType::chaser, 4 }, { eType::wasp, 4 }, { eType::sniper, 2 } },  40, 0.8, 0.93 },
                { eObjective::kill,    60, { { eType::chaser, 5 }, { eType::wasp, 4 }, { eType::sniper, 2 } },   48, 0.6, 1.0 },
            }, 0.375 }, // the planet's glare drowns the stars
        };
        return stages;
    }

    int TotalMissions()
    {
        int total{ 0 };
        for( const auto & stage : Stages() )
            total += static_cast< int >( stage.missions.size() );
        return total;
    }

    struct Location
    {
        const StageDef * stage;
        int stageNumber;   // 1-based
        int missionNumber; // 1-based
        bool last;         // last mission of its stage
    };

    // a progress past the end is clamped on the very last mission:
    Location Locate( const int _progress )
    {
        auto remaining{ std::clamp( _progress, 0, TotalMissions() - 1 ) };
        int stageNumber{ 1 };
        for( const auto & stage : Stages() ) {
            const auto count{ static_cast< int >( stage.missions.size() ) };
            if( remaining < count )
                return { &stage, stageNumber, remaining + 1, remaining == count - 1 };
            remaining -= count;
            stageNumber++;
        }
        return { &Stages().back(), stageNumber - 1, static_cast< int >( Stages().back().missions.size() ), true };
    }

    std::string Clock( const int _seconds )
    {
        return std::format( "{}:{:02}", _seconds / 60, _seconds % 60 );
    }
}


StageDirector::StageDirector( const int _frameRate, const int _progress )
    : m_frameRate{ _frameRate }
    , m_progress{ std::clamp( _progress, 0, TotalMissions() ) }
    , m_phase{ m_progress >= TotalMissions() ? ePhase::freePlay : ePhase::intro }
{
}


StageDirector::eEvent StageDirector::Update()
{
    switch( m_phase ) {
        case ePhase::intro:
            if( ++m_phaseTicks >= m_frameRate * introSeconds ) {
                m_phase = ePhase::active;
                m_phaseTicks = 0;
                m_kills = 0;
            }
            break;
        case ePhase::active: {
            m_phaseTicks++;
            const auto & mission{ Mission() };
            const bool done{ mission.objective == eObjective::kill ? m_kills >= mission.target : m_phaseTicks >= m_frameRate * mission.target };
            if( done ) {
                m_phase = ePhase::cleared;
                m_phaseTicks = 0;
                return eEvent::cleared;
            }
            break;
        }
        case ePhase::cleared:
            if( ++m_phaseTicks >= m_frameRate * clearedSeconds ) {
                m_progress++;
                m_phase = m_progress >= TotalMissions() ? ePhase::freePlay : ePhase::intro;
                m_phaseTicks = 0;
                m_kills = 0;
            }
            break;
        case ePhase::freePlay:
            break;
    }
    return eEvent::none;
}


void StageDirector::DebugJumpAlmostDone( const int _progress )
{
    m_progress = std::clamp( _progress, 0, TotalMissions() - 1 );
    m_phase = ePhase::active;
    const auto & mission{ Mission() };
    m_kills = mission.objective == eObjective::kill ? mission.target - 1 : 0;
    m_phaseTicks = mission.objective == eObjective::kill ? 0 : std::max( ( mission.target - 5 ) * m_frameRate, 0 );
}


void StageDirector::OnKill()
{
    if( m_phase == ePhase::active || m_phase == ePhase::freePlay )
        m_kills++;
}


std::vector< Enemy::eType > StageDirector::Missing( const std::vector< Enemy > & _alive ) const
{
    std::vector< Enemy::eType > missing;
    if( m_phase != ePhase::active && m_phase != ePhase::freePlay )
        return missing;
    for( const auto & [ type, count ] : Mission().roster ) {
        const auto alive{ std::count_if( _alive.begin(), _alive.end(),
            [ & ]( const Enemy & _enemy ){ return !_enemy.dead && _enemy.type == type; } ) };
        for( auto i{ alive }; i < count; i++ )
            missing.emplace_back( type );
    }
    return missing;
}


int StageDirector::NextProgress() const
{
    return std::min( m_progress + 1, TotalMissions() );
}


const MissionDef & StageDirector::Mission() const
{
    const auto location{ Locate( m_progress ) };
    return location.stage->missions.at( static_cast< size_t >( location.missionNumber - 1 ) );
}


const char * StageDirector::StageTitle() const { return Locate( m_progress ).stage->title; }
const std::vector< const char * > & StageDirector::Story() const { return Locate( m_progress ).stage->story; }
double StageDirector::StarDensity() const { return Locate( m_progress ).stage->starDensity; }


const std::vector< const char * > & StageDirector::Epilogue()
{
    static const std::vector< const char * > lines{
        "The guild's fleet is scattered, its mines are silent, and the strange attractors belt of Stellis Secura is finally free.",
        "Its energy now serves the peoples of the Universal Alliance for Peace, as our emperator Muhammad Silmane XIV had foreseen.",
        "",
        "Smugglers will whisper your name for years, and wisely keep away from this system.",
        "",
        "Yet the belt is vast, and a few stragglers still roam its wells... Enjoy the silence, or hunt them down for the fun of it." };
    return lines;
}
int StageDirector::StageNumber() const { return Locate( m_progress ).stageNumber; }
int StageDirector::MissionNumber() const { return Locate( m_progress ).missionNumber; }
bool StageDirector::LastOfStage() const { return Locate( m_progress ).last; }


std::string StageDirector::Label( const int _progress )
{
    if( _progress >= TotalMissions() )
        return "FREE PLAY";
    const auto location{ Locate( _progress ) };
    return std::format( "STAGE {}-{}", location.stageNumber, location.missionNumber );
}


bool StageDirector::Reached( const int _stage, const int _mission ) const
{
    if( m_phase == ePhase::freePlay )
        return true;
    const auto location{ Locate( m_progress ) };
    return location.stageNumber > _stage || ( location.stageNumber == _stage && location.missionNumber >= _mission );
}


bool StageDirector::IsAt( const int _stage, const int _mission ) const
{
    const auto location{ Locate( m_progress ) };
    return m_phase != ePhase::freePlay && location.stageNumber == _stage && location.missionNumber == _mission;
}


std::string StageDirector::ObjectiveText() const
{
    const auto & mission{ Mission() };
    return mission.objective == eObjective::kill
        ? std::format( "ELIMINATE {} HOSTILES", mission.target )
        : std::format( "SURVIVE {} SECONDS", mission.target );
}


std::string StageDirector::TrackerLabel() const
{
    if( m_phase == ePhase::freePlay )
        return "FREE PLAY";
    return Mission().objective == eObjective::kill ? "ELIMINATE" : "SURVIVE";
}


std::string StageDirector::TrackerValue() const
{
    if( m_phase == ePhase::freePlay )
        return std::format( "{} KILLS", m_kills );
    const auto & mission{ Mission() };
    if( mission.objective == eObjective::kill )
        return std::format( "{} / {}", std::min( m_kills, mission.target ), mission.target );
    const auto remainingTicks{ std::max( mission.target * m_frameRate - m_phaseTicks, 0 ) };
    return Clock( ( remainingTicks + m_frameRate - 1 ) / m_frameRate );
}


double StageDirector::ObjectiveRatio() const
{
    if( m_phase == ePhase::cleared )
        return 1;
    if( m_phase != ePhase::active )
        return 0;
    const auto & mission{ Mission() };
    return std::clamp( mission.objective == eObjective::kill
        ? static_cast< double >( m_kills ) / mission.target
        : static_cast< double >( m_phaseTicks ) / ( mission.target * m_frameRate ), 0.0, 1.0 );
}


double StageDirector::PhaseRatio() const
{
    const auto duration{ m_frameRate * ( m_phase == ePhase::intro ? introSeconds : clearedSeconds ) };
    return std::clamp( static_cast< double >( m_phaseTicks ) / duration, 0.0, 1.0 );
}
