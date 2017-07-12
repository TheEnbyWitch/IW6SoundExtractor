// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define NOMINMAX // breaks one std thing

#define PAUSE_AFTER_EXTRACT // pauses the app after extracting everything

#include "targetver.h"
#include <iostream>
#include <limits>
#include <stdlib.h>
#include <stdio.h>
#include <tchar.h>
#include <time.h>

#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

#include <Windows.h>

#include <zlib.h>

#define FF_CHUNK_SIZE 65535


// TODO: reference additional headers your program requires here
