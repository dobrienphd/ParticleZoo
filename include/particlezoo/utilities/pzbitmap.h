#pragma once

#include "particlezoo/ByteBuffer.h"
#include "particlezoo/utilities/pzimages.h"

#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>
#include <cassert>
#include <limits>

namespace ParticleZoo {

    template<typename T>
    class BitmapImage : public Image<T> {
        static_assert(std::is_arithmetic_v<T> && "Bitmap type must be arithmetic");

    public:
        BitmapImage(int width, int height);
        int width() const override;
        int height() const override;
        void setPixel(int x, int y, const Pixel<T>& p) override;
        void setPixel(int x, int y, const T& R, const T& G, const T& B) override;
        Pixel<T> getPixel(int x, int y) const override;
        void normalize(T normalizationFactor) override;

        void save(const std::string& path) const override;
        void save(const std::string& path, T lowerLimit, T upperLimit) const;

    private:
        int width_, height_;
        T minValue_;
        T maxValue_;

        std::vector<Pixel<T>> data_;
    };

    /* Inline implementation of Bitmap */

    template<typename T>
    inline BitmapImage<T>::BitmapImage(int w, int h)
        : width_(w), height_(h), minValue_(std::numeric_limits<T>::max()), maxValue_(std::numeric_limits<T>::min()), data_(w * h)
    {
        if (w <= 0 || h <= 0) throw std::runtime_error("Invalid dimensions");
    }

    template<typename T>
    inline int BitmapImage<T>::width() const { return width_; }

    template<typename T>
    inline int BitmapImage<T>::height() const { return height_; }

    template<typename T>
    inline void BitmapImage<T>::setPixel(int x, int y, const Pixel<T>& p) {
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
    inline void BitmapImage<T>::setPixel(int x, int y, const T& R, const T& G, const T& B) {
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
    inline Pixel<T> BitmapImage<T>::getPixel(int x, int y) const {
        if (x<0||x>=width_||y<0||y>=height_) throw std::runtime_error("Pixel out of range");
        size_t idx = (y * width_ + x);
        return data_[idx];
    }

    template<typename T>
    inline void BitmapImage<T>::normalize(T normalizationFactor) {
        if (normalizationFactor <= 0) {
            throw std::runtime_error("Normalization factor must be greater than zero.");
        }
        for (auto& pixel : data_) {
            pixel.r /= normalizationFactor;
            pixel.g /= normalizationFactor;
            pixel.b /= normalizationFactor;
        }
        minValue_ /= normalizationFactor;
        maxValue_ /= normalizationFactor;
    }

    template<typename T>
    inline void BitmapImage<T>::save(const std::string& path) const {
        save(path, minValue_, maxValue_);
    }

    template<typename T>
    inline void BitmapImage<T>::save(const std::string& path, T lowerLimit, T upperLimit) const {
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
        T scale = static_cast<T>(255.0) / (upperLimit - lowerLimit);

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
                if (scaledR < 0) scaledR = static_cast<T>(0);
                if (scaledG < 0) scaledG = static_cast<T>(0);
                if (scaledB < 0) scaledB = static_cast<T>(0);
                if (scaledR > 255) scaledR = static_cast<T>(255);
                if (scaledG > 255) scaledG = static_cast<T>(255);
                if (scaledB > 255) scaledB = static_cast<T>(255);
                byte red = static_cast<byte>(scaledR);
                byte green = static_cast<byte>(scaledG);
                byte blue = static_cast<byte>(scaledB);
                buf.write<byte>(blue);
                buf.write<byte>(green);
                buf.write<byte>(red);
            }
            if (pad) buf.writeBytes(padding);
        }
        std::ofstream ofs(path, std::ios::binary);
        if (!ofs) throw std::runtime_error("Cannot open file for writing");
        ofs.write(reinterpret_cast<const char*>(buf.data()), buf.length());
    }

} // namespace ParticleZoo