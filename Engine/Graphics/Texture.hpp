#pragma once

#include <memory>

//The texture at index 0 in the texture array is guaranteed to be a placeholder
constexpr uint32_t ImageUndefined = 0;
using ImageHandle = uint32_t;

class Image;
class Sampler;

struct Texture {
    std::shared_ptr<Image> image = nullptr;
    Sampler* sampler = nullptr;
    ImageHandle handle = ImageUndefined;
};