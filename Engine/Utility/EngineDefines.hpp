#pragma once

#ifdef ENGINE_EXPORT
    #define ENGINE_API __declspec(dllexport)
    //#define FASTGLTF_EXPORT __declspec(dllexport)
#else
    #define ENGINE_API __declspec(dllimport)
  //  #define FASTGLTF_EXPORT __declspec(dllimport)
#endif

#ifdef _DEBUG
  constexpr bool debug = true;
#else
  constexpr bool debug = false;
#endif

#ifdef __clang__
    #define FORCE_INLINE [[clang::always_inline]]
#elifdef __GNUC__
    #define FORCE_INLINE [[gnu::always_inline]]
#endif