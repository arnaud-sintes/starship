#include "NanoVGRenderer.h"
#include "core/Timer.h"
#include "Renderer.h"
#include "core/Packer.h"
#include "core/Macros.h"
#include "version.h"


#define _DISPLAY_FPS


int main( int, char * )
{
    Win32::ShowConsole( false );
    const Dimension_ui windowDimension{ 1500, 900 };
    Win32::Windows window{ L"Starship v" + std::wstring{ _ToWideString( __ToString( VERSION ) ) }, windowDimension };
    window.ShowCursor( false );
    auto & timer{ Timer::GetInstance() }; // init nano precision
    //Win32::SetThreadRealtimePriority();
    OpenGL ogl{ window };
    NanoVGRenderer nanoVG{ ogl };
    const auto resources{ Packer::UnPack( "./resource.dat" ) };
    if( !resources )
        return -1;
    nanoVG.CreateFont( "openSans", resources->find( "OpenSans-Light.ttf" )->second );
    nanoVG.CreateFont( "openSansBold", resources->find( "OpenSans-ExtraBold.ttf" )->second );
    nanoVG.CreateFont( "sourceCodePro", resources->find( "SourceCodePro-Regular.ttf" )->second );

    Renderer renderer{ window, *resources };

    Timer::FpsContext fpsContext{ 60 }; // 60 fps loop
    while( window.Dispatch() ) {
        const auto temper{ timer.Temper( fpsContext ) };
        const auto context{ ogl.MakeCurrent() };
        context.Viewport( windowDimension );
        context.Clear( { 0, 0.05, 0.1 } );
        const auto frame{ nanoVG.CreateFrame( windowDimension ) };

        // main loop:
        renderer.Loop( frame );
            
        // frame rate information:
        #ifdef _DISPLAY_FPS
        const auto & fpsState{ fpsContext.Update() };
        std::stringstream ssFps;
        ssFps << "FPS: " << std::setprecision( 3 ) << fpsState.avgFrameRate;
        std::stringstream ssConsumption;            
        ssConsumption << " (" << std::setprecision( 3 ) << fpsState.avgConsumption << "%)";
        const std::string fps{ ssFps.str() + ssConsumption.str() + ( fpsState.frameDropped ? " [frame dropped]" : "" ) };
        frame.Text( { 2, windowDimension.ToType< double >().height - 2 }, "sourceCodePro", 14, fps, { 1, 1, 1 }, NanoVGRenderer::Frame::eTextAlign::bottomLeft );
        #endif
    }

    return 0;
}