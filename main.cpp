#include "NanoVGRenderer.h"
#include "core/Timer.h"
#include "Game.h"
#include "core/Packer.h"
#include "core/Macros.h"
#include "version.h"

#define _DISPLAY_FPS

int main( int, char * )
{
    const bool fullscreen{ false };
    Win32::ShowConsole( false );
    const Dimension_ui defaultWindowedDimension{ 1500, 900 };
    Win32::Windows window{ L"Starship v" + std::wstring{ _ToWideString( __ToString( VERSION ) ) }, defaultWindowedDimension, fullscreen };
    const Dimension_ui & windowDimension{ window.GetDimension() };
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

    const unsigned long long frameRate{ 60 }; // 60 fps target
    Game game{ window, *resources, static_cast< int >( frameRate ) };

    Timer::FpsContext fpsContext{ frameRate };
    while( window.Dispatch() ) {
        const auto temper{ timer.Temper( fpsContext ) };
        const auto context{ ogl.MakeCurrent() };
        context.Viewport( windowDimension );
        context.Clear( { 0, 0.035, 0.075 } );
        const auto frame{ nanoVG.CreateFrame( windowDimension ) };

        // main loop:
        game.RunFrame( frame );

        // frame rate information:
        #ifdef _DISPLAY_FPS
        temper.Update();
        const auto & fpsState{ fpsContext.Update() };
        const std::string fps{ std::format( "FPS: {:.3} ({:.3}%){}", fpsState.avgFrameRate, fpsState.avgConsumption, fpsState.frameDropped ? " [frame dropped]" : "" ) };
        frame.Text( { 2, windowDimension.ToType< double >().height - 2 }, "sourceCodePro", 14, fps, { 1, 1, 1 }, NanoVGRenderer::Frame::eTextAlign::bottomLeft );
        #endif
    }

    return 0;
}
