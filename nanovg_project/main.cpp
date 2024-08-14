// maqui, (the) Most Astonishingly Qualitative User Interface

#include "NanoVGSurface.h"


int main()
{
    Win32::ShowConsole( false );
    Win32::Windows window{ L"test", { 640, 480 } };
    NanoVGSurface surface{ window };
    surface.CreateFont( "fontA", "OpenSans-Light.ttf" );
    while( window.Dispatch() ) {
        auto frame{ surface.CreateFrame( window.GetDimension(), { 0, 0, 0 } ) };
        frame.FillArc( { 320, 0 }, 50, 0, 3.1415, { 1, 0, 0 } );
        frame.FillArc( { 0, 240 }, 50, 3.1415 / 2, 3.1415 + 3.1415 / 2, { 1, 0, 0 }, false );
        frame.StrokeCircle( { 320, 240 }, 50, { 1, 0, 0 }, 1 );
        frame.FillArc( { 640, 240 }, 50, 3.1415 / 2, 3.1415 + 3.1415 / 2, { 1, 0, 0 }, true );
        frame.FillArc( { 320, 480 }, 50, 0, 3.1415, { 1, 0, 0 }, false );
        frame.Line( { 100, 200 }, { 500, 400 }, { 0, 0.5, 1 }, 2 );
        frame.Text( { 0, 0 }, "fontA", 20, "hello there", { 1, 1, 1 } );
    }
}