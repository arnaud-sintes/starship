#pragma once

#define __Stringify( x ) #x
#define __ToString( x ) __Stringify( x )
#define __Widen( x ) L##x
#define _ToWideString( x ) __Widen( x )