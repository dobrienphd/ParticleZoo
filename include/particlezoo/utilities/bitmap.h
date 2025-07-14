#pragma once

#include "particlezoo/ByteBuffer.h"

#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>
#include <cassert>
#include <limits>

namespace ParticleZoo {

    // Simple 24-bit pixel
    template<typename T>
    struct Pixel {
        static_assert(std::is_arithmetic_v<T> && "Pixel type must be arithmetic");
        T r, g, b;
        Pixel() : r(0), g(0), b(0) {}
        Pixel(T rr, T gg, T bb) : r(rr), g(gg), b(bb) {}
    };

    template<typename T>
    class Bitmap {
        static_assert(std::is_arithmetic_v<T> && "Bitmap type must be arithmetic");

    public:
        Bitmap(int width, int height);
        int width() const;
        int height() const;
        void setPixel(int x, int y, const Pixel<T>& p);
        void setPixel(int x, int y, const T& R, const T& G, const T& B);
        Pixel<T> getPixel(int x, int y) const;


        void save(const std::string& path) const;
        void save(const std::string& path, T lowerLimit, T upperLimit) const;

    private:
        int width_, height_;
        T minValue_;
        T maxValue_;

        std::vector<Pixel<T>> data_;
    };

    /* Inline implementation of Bitmap */

    template<typename T>
    inline Bitmap<T>::Bitmap(int w, int h)
        : width_(w), height_(h), minValue_(std::numeric_limits<T>::max()), maxValue_(std::numeric_limits<T>::min()), data_(w * h)
    {
        if (w <= 0 || h <= 0) throw std::runtime_error("Invalid dimensions");
    }

    template<typename T>
    inline int Bitmap<T>::width() const { return width_; }

    template<typename T>
    inline int Bitmap<T>::height() const { return height_; }

    template<typename T>
    inline void Bitmap<T>::setPixel(int x, int y, const Pixel<T>& p) {
        if (x<0||x>=width_||y<0||y>=height_) throw std::runtime_error("Pixel out of range");
        size_t idx = (y * width_ + x);
        data_[idx] = p;
        T maxInPixel = std::max({p.r, p.g, p.b});
        T minInPixel = std::min({p.r, p.g, p.b});
        if (maxInPixel > maxValue_) {
            maxValue_ = maxInPixel;
        }
        if (minInPixel < minValue_) {
            minValue_ = minInPixel;
        }
    }

    template<typename T>
    inline void Bitmap<T>::setPixel(int x, int y, const T& R, const T& G, const T& B) {
        if (x<0||x>=width_||y<0||y>=height_) throw std::runtime_error("Pixel out of range");
        size_t idx = (y * width_ + x);
        data_[idx] = Pixel<T>{R, G, B};
        T maxInPixel = std::max({R, G, B});
        T minInPixel = std::min({R, G, B});
        if (maxInPixel > maxValue_) {   
            maxValue_ = maxInPixel;
        }
        if (minInPixel < minValue_) {
            minValue_ = minInPixel;
        }
    }

    template<typename T>
    inline Pixel<T> Bitmap<T>::getPixel(int x, int y) const {
        if (x<0||x>=width_||y<0||y>=height_) throw std::runtime_error("Pixel out of range");
        size_t idx = (y * width_ + x);
        return data_[idx];
    }

    template<typename T>
    inline void Bitmap<T>::save(const std::string& path) const {
        save(path, minValue_, maxValue_);
    }

    template<typename T>
    inline void Bitmap<T>::save(const std::string& path, T lowerLimit, T upperLimit) const {
        // BMP row padding to 4-byte boundary
        int rowBytes = width_ * 3;
        int pad = (4 - (rowBytes % 4)) % 4;
        int pixelDataSize = (rowBytes + pad) * height_;
        int fileSize = 54 + pixelDataSize;

        // Scale the pixel values to 0-255 range, keeping in mind that min values may be negative
        T range = upperLimit - lowerLimit;
        if (range <= 0) {
            throw std::runtime_error("Invalid range for pixel values");
        }
        double scale = 255.0 / (upperLimit - lowerLimit);

        ByteBuffer buf(54 + pixelDataSize, ByteOrder::LittleEndian);
        // file header
        buf.write<uint16_t>(0x4D42);
        buf.write<uint32_t>(fileSize);
        buf.write<uint32_t>(0);
        buf.write<uint32_t>(54);
        // info header
        buf.write<uint32_t>(40);
        buf.write<int32_t>(width_);
        buf.write<int32_t>(height_);
        buf.write<uint16_t>(1);
        buf.write<uint16_t>(24);
        buf.write<uint32_t>(0);
        buf.write<uint32_t>(pixelDataSize);
        buf.write<uint32_t>(2835);
        buf.write<uint32_t>(2835);
        buf.write<uint32_t>(0);
        buf.write<uint32_t>(0);
        // pixel data (bottom-up)
        std::vector<byte> padding(pad, 0);
        for (int y = height_ - 1; y >= 0; --y) {
            for (int x = 0; x < width_; ++x) {
                const Pixel<T>& p = getPixel(x, y);
                T scaledR = (p.r - lowerLimit) * scale;
                T scaledG = (p.g - lowerLimit) * scale;
                T scaledB = (p.b - lowerLimit) * scale;
                if (scaledR < 0) scaledR = 0;
                if (scaledG < 0) scaledG = 0;
                if (scaledB < 0) scaledB = 0;
                if (scaledR > 255) scaledR = 255;
                if (scaledG > 255) scaledG = 255;
                if (scaledB > 255) scaledB = 255;
                byte r = static_cast<byte>(scaledR);
                byte g = static_cast<byte>(scaledG);
                byte b = static_cast<byte>(scaledB);
                buf.write<byte>(b);
                buf.write<byte>(g);
                buf.write<byte>(r);
            }
            if (pad) buf.writeBytes(padding);
        }
        std::ofstream ofs(path, std::ios::binary);
        if (!ofs) throw std::runtime_error("Cannot open file for writing");
        ofs.write(reinterpret_cast<const char*>(buf.data()), buf.length());
    }

} // namespace ParticleZoo