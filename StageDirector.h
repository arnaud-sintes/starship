#pragma once

#include "Enemy.h"


// --------------
// A mission (displayed "Stage 1-2") is pure data: an objective plus what the
// simulation keeps alive around the ship while it lasts.
struct MissionDef
{
    enum class eObjective
    {
        kill,    // destroy `target` hostiles
        survive, // stay alive `target` seconds
    };
    eObjective objective;
    int target;

    struct Spawn
    {
        Enemy::eType type;
        int count; // concurrent enemies of this type
    };
    std::vector< Spawn > roster;

    size_t gravityMines; // gravity mines population cap
    double respawnSeconds; // delay before a destroyed enemy is replaced
    double attractorDensity; // [ 0, 1 ] share of the strange attractors field that exists
};


// A stage is the context (title) of a sequence of missions.
struct StageDef
{
    const char * title;
    std::vector< const char * > story; // scenario shown on the full-screen briefing, one line each ("" = blank line)
    std::vector< MissionDef > missions;
    double starDensity; // [ 0, 1 ] share of the starfield that is drawn (1 = the densest sky)
};


// --------------
// Owns the stage table and the mission flow (briefing -> objective -> debriefing ->
// next mission); the World asks it what is missing from the field instead of
// hard-coding a roster, and reports kills to it. Simulation-only, no rendering.
//
// `progress` is the flat index of the current mission over all the stages; past the
// last one, the game goes on in free play (last mission setup, no objective).
class StageDirector
{
public:
    enum class ePhase
    {
        intro,    // briefing banner, nothing spawns yet
        active,   // objective running
        cleared,  // debriefing banner, the field is cleaned up
        freePlay, // all stages done, endless
    };

    enum class eEvent
    {
        none,
        cleared, // the objective was just fulfilled
    };

    explicit StageDirector( const int _frameRate, const int _progress = 0 );

public:
    eEvent Update(); // one tick
    void OnKill();
    void DebugJumpAlmostDone( const int _progress ); // TEMPORARY test helper: mission _progress, active, one kill (or 5 seconds) from clearing

    // enemy types to (re)spawn so that the field matches the current mission roster:
    std::vector< Enemy::eType > Missing( const std::vector< Enemy > & _alive ) const;

    ePhase Phase() const { return m_phase; }
    int Progress() const { return m_progress; }
    int NextProgress() const; // where a retry/checkpoint taken at the end of this mission restarts
    const MissionDef & Mission() const;
    const char * StageTitle() const;
    const std::vector< const char * > & Story() const;
    static const std::vector< const char * > & Epilogue(); // shown once every mission is cleared
    double StarDensity() const;
    int StageNumber() const;
    int MissionNumber() const;
    bool LastOfStage() const;
    // goody unlock schedule (see Goody::FirstMission); free play unlocks everything:
    bool Reached( const int _stage, const int _mission ) const;
    bool IsAt( const int _stage, const int _mission ) const;
    std::string Label() const { return Label( m_progress ); } // "STAGE 1-2" or "FREE PLAY"
    static std::string Label( const int _progress );

    std::string ObjectiveText() const;  // "ELIMINATE 12 HOSTILES"
    std::string TrackerLabel() const;   // "ELIMINATE" / "SURVIVE" / "FREE PLAY"
    std::string TrackerValue() const;   // "7 / 12" / "0:42" / "37 KILLS"
    double ObjectiveRatio() const;      // [ 0, 1 ]
    double PhaseRatio() const;          // [ 0, 1 ] over the intro / cleared banners

private:
    int m_frameRate;
    int m_progress;
    ePhase m_phase;
    int m_phaseTicks{ 0 };
    int m_kills{ 0 };
};
