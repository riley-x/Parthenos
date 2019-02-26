// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include <string>
#include <tuple>
#include <vector>
#include <deque>
#include <map>
#include <cmath>
#include <ctime>
#include <exception>
#include <algorithm>
#include <numeric>


// reference additional headers your program requires here
#include <d2d1.h>
#pragma comment(lib, "d2d1")
#include <d2d1_1.h>
#include <dwrite.h>
#pragma comment(lib, "dwrite")
#include <wincodec.h>