#pragma once

#ifdef ENGINE_EXPORT
#define IMGUI_API __declspec(dllexport)
#else 
#define IMGUI_API __declspec(dllimport)
#endif 