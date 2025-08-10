#pragma once

#include <string>
#include <type_traits>

namespace ParticleZoo {

template<typename T>
struct Pixel {
    static_assert(std::is_arithmetic_v<T>, "Pixel<T> must be arithmetic");
    T r, g, b;
    Pixel() : r(0), g(0), b(0) {}
    Pixel(T rr, T gg, T bb) : r(rr), g(gg), b(bb) {}
};

/// Pure‐virtual interface for any RGB image
template<typename T>
class Image {
    static_assert(std::is_arithmetic_v<T>, "Image<T> must be arithmetic");
public:
    virtual ~Image() = default;

    virtual int width() const = 0;
    virtual int height() const = 0;

    virtual void setPixel(int x, int y, const Pixel<T>& p) = 0;
    virtual void setPixel(int x, int y, const T& R, const T& G, const T& B) = 0;
    virtual Pixel<T> getPixel(int x, int y) const = 0;
    virtual void normalize(T normalizationFactor) = 0;

    /// write to disk (format‐specific behavior in derived)
    virtual void save(const std::string& path) const = 0;
};

} // namespace ParticleZoo