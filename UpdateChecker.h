#pragma once

#include "core/std.h"


// --------------
// At startup, asks GitHub (in the background, the game never waits for it) for the latest release of the game, and tells whether
// it is newer than the running version. No network, no answer, a malformed one: nothing is ever reported, the game just plays.
class UpdateChecker
{
public:
    UpdateChecker( const std::string & _currentVersion ); // starts the check

public:
    bool UpdateAvailable() const { return m_shared->done && m_shared->newer; } // once the answer is in
    const std::string & CurrentVersion() const { return m_currentVersion; }
    const std::string & LatestVersion() const { return m_shared->latest; } // valid when UpdateAvailable()
    std::string Url() const; // the page of the latest release, where the zip is to be downloaded

    static bool IsNewer( const std::string & _version, const std::string & _than ); // dotted numbers, an optional leading 'v'

private:
    struct Shared // written by the background thread, then published by `done`
    {
        std::atomic< bool > done{ false };
        bool newer{ false };
        std::string latest;
    };

    const std::string m_currentVersion;
    std::shared_ptr< Shared > m_shared; // the thread keeps it alive on its own, should the game quit first
};
