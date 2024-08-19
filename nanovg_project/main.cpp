// maqui, (the) Most Astonishingly Qualitative User Interface

#include "NanoVGRenderer.h"
#include "Timer.h"
#include "Renderer.h"


int main()
{
    Win32::ShowConsole( true );
    Win32::Windows window{ L"Starship", { 1920, 1000 } };
    auto & timer{ Timer::GetInstance() }; // init nano precision
    Win32::SetThreadRealtimePriority();
    OpenGL ogl{ window };
    NanoVGRenderer nanoVG{ ogl };
    nanoVG.CreateFont( "fontA", "OpenSans-Light.ttf" );

    Renderer renderer{ window };

    Timer::FpsContext fpsContext{ 60 };
    while( window.Dispatch() ) {
        const auto temper{ timer.Temper( fpsContext ) };
        {
            auto context{ ogl.MakeCurrent() };
            context.Viewport( window.GetDimension() );
            context.Clear( { 0, 0.05, 0.1 } );
            auto frame{ nanoVG.CreateFrame( window.GetDimension() ) };

            // main loop:
            renderer.Loop( frame );
            
            // frame rate information:
            fpsContext.Update();
            std::stringstream ssFps;            
            ssFps << "fps: " << std::setprecision( 3 ) << fpsContext.Fps();
            std::stringstream ssConsumption;            
            ssConsumption << " (" << std::setprecision( 3 ) << fpsContext.Consumption() << "%)";
            const std::string fps{ ssFps.str() + ssConsumption.str() + ( fpsContext.FrameDropped() ? " [frame dropped]" : "" ) };
            frame.Text( { 2, 2 }, "fontA", 14, fps, { 1, 1, 1 } );
        }
    }
}