#include "NanoVGRenderer.h"
#include "core/Timer.h"
#include "Game.h"
#include "core/Packer.h"
#include "core/Macros.h"
#include "version.h"

#define _DISPLAY_FPS

int main( int, char * )
{
    const bool fullscreen{ true };
    Win32::SetDpiAware();

    // the game is designed at a fixed logical HEIGHT: the gameplay scale is identical
    // on every monitor, the logical width follows the screen aspect ratio so the frame
    // always fills it edge to edge (no bars); windowed mode covers the whole work area
    constexpr unsigned int logicalHeight{ 900 };

    Win32::Windows window{ L"Starship v" + std::wstring{ _ToWideString( __ToString( VERSION ) ) },
        Win32::GetMaximizedClientDimension(), fullscreen };
    if( !fullscreen )
        window.SetPos( Win32::GetWorkAreaOrigin() );
    const View view{ logicalHeight, window.GetDimension() };
    window.ShowCursor( false );
    auto & timer{ Timer::GetInstance() }; // init nano precision
    OpenGL ogl{ window };
    NanoVGRenderer nanoVG{ ogl };
    ogl.MakeCurrent().SetSwapInterval( 0 ); // frame pacing is owned by the Timer, don't let SwapBuffers block on vsync
    const auto resources{ Packer::UnPack( "./resource.dat" ) };
    if( !resources )
        return -1;
    nanoVG.CreateFont( "openSans", resources->find( "OpenSans-Light.ttf" )->second );
    nanoVG.CreateFont( "openSansBold", resources->find( "OpenSans-ExtraBold.ttf" )->second );
    nanoVG.CreateFont( "sourceCodePro", resources->find( "SourceCodePro-Regular.ttf" )->second );

    const unsigned long long frameRate{ 60 }; // 60 fps render target
    const unsigned long long tickRate{ 60 }; // fixed simulation rate
    Game game{ window, view, *resources, static_cast< int >( tickRate ) };

    // fixed-timestep simulation, decoupled from rendering: the accumulator collects
    // wall-clock time and the world ticks at exactly tickRate, so a late frame
    // triggers catch-up ticks instead of slowing the game down
    const unsigned long long tickDuration{ 1'000'000'000ull / tickRate };
    const unsigned long long maxCatchUp{ 5 * tickDuration }; // under sustained overload, slow down rather than spiral
    unsigned long long accumulator{ tickDuration }; // simulate the first tick immediately
    unsigned long long lastTime{ timer.Get() };

    Timer::FpsContext fpsContext{ frameRate };
    while( window.Dispatch() ) {
        const auto temper{ timer.Temper( fpsContext ) };

        // simulation:
        const auto currentTime{ timer.Get() };
        accumulator = std::min( accumulator + ( currentTime - lastTime ), maxCatchUp );
        lastTime = currentTime;
        while( accumulator >= tickDuration ) {
            accumulator -= tickDuration;
            game.Tick();
        }

        // rendering:
        const auto context{ ogl.MakeCurrent() };
        context.Viewport( view );
        context.Clear( { 0, 0.035, 0.075 } );
        const auto frame{ nanoVG.CreateFrame( view ) };
        game.Draw( frame );

        // frame rate information:
        #ifdef _DISPLAY_FPS
        temper.Update();
        const auto & fpsState{ fpsContext.Update() };
        const std::string fps{ std::format( "FPS {:.3}  |  {:.3}%{}", fpsState.avgFrameRate, fpsState.avgConsumption, fpsState.frameDropped ? "  |  FRAME DROPPED" : "" ) };
        frame.Text( { 6, view.logical.ToType< double >().height - 4 }, "sourceCodePro", 13, fps, { 1, 1, 1, fpsState.frameDropped ? 0.9 : 0.4 }, NanoVGRenderer::Frame::eTextAlign::bottomLeft );
        #endif
    }

    return 0;
}
