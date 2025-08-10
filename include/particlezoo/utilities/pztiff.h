#pragma once

#include "particlezoo/ByteBuffer.h"

#include "particlezoo/utilities/pzimages.h"

#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>
#include <limits>
#include <algorithm>
#include <type_traits>

namespace ParticleZoo {

    template<typename T>
    class TiffImage : public Image<T> {
        static_assert(std::is_arithmetic_v<T> && "TiffImage type must be arithmetic");

    public:
        TiffImage(int width, int height);
        int width() const override;
        int height() const override;
        void setPixel(int x, int y, const Pixel<T>& p) override;
        void setPixel(int x, int y, const T& R, const T& G, const T& B) override;
        Pixel<T> getPixel(int x, int y) const override;
        void normalize(T normalizationFactor) override;
        void save(const std::string& path) const override;

    private:
        int width_, height_;
        T minValue_, maxValue_;
        std::vector<Pixel<T>> data_;
    };


    /* Inline implementation of TiffImage */

    template<typename T>
    inline TiffImage<T>::TiffImage(int w, int h)
        : width_(w)
        , height_(h)
        , minValue_(std::numeric_limits<T>::max())
        , maxValue_(std::numeric_limits<T>::min())
        , data_(size_t(w) * h)
    {
        if (w <= 0 || h <= 0) throw std::runtime_error("Invalid dimensions");
    }

    template<typename T>
    inline int TiffImage<T>::width() const { return width_; }

    template<typename T>
    inline int TiffImage<T>::height() const { return height_; }

    template<typename T>
    inline void TiffImage<T>::setPixel(int x, int y, const Pixel<T>& p) {
        if (x < 0 || x >= width_ || y < 0 || y >= height_)
            throw std::runtime_error("Pixel out of range");
        size_t idx = size_t(y) * width_ + x;
        data_[idx] = p;
        T mx = std::max({p.r, p.g, p.b});
        T mn = std::min({p.r, p.g, p.b});
        if (mx > maxValue_) maxValue_ = mx;
        if (mn < minValue_) minValue_ = mn;
    }

    template<typename T>
    inline void TiffImage<T>::setPixel(int x, int y, const T& R, const T& G, const T& B) {
        setPixel(x, y, Pixel<T>{R, G, B});
    }

    template<typename T>
    inline Pixel<T> TiffImage<T>::getPixel(int x, int y) const {
        if (x < 0 || x >= width_ || y < 0 || y >= height_)
            throw std::runtime_error("Pixel out of range");
        return data_[size_t(y) * width_ + x];
    }

    template<typename T>
    inline void TiffImage<T>::normalize(T normalizationFactor) {
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
    inline void TiffImage<T>::save(const std::string& path) const {
        static_assert(std::is_arithmetic_v<T>, "TiffImage<T> requires arithmetic T");

        constexpr uint16_t samplesPerPixel = 3;
        constexpr uint16_t bitsPerSample   = sizeof(T) * 8; // e.g. 8,16,32
        constexpr uint16_t sampleFormat    = std::is_floating_point_v<T> ? 3 : 1; // 1=UnsignedInt, 3=IEEEFP

        size_t pixelCount = size_t(width_) * height_;
        size_t pdSize     = pixelCount * samplesPerPixel * sizeof(T);

        // IFD layout
        const uint16_t ENTRY_COUNT = 12;
        const size_t SIZE_IFD  = 2 + ENTRY_COUNT*12 + 4;
        const size_t OFF_IFD   = 8;
        const size_t OFF_AUX   = OFF_IFD + SIZE_IFD;
        // compute offsets into the AUX block
        const uint32_t offBits    = static_cast<uint32_t>(OFF_AUX);
        const uint32_t offSamFmt  = offBits + samplesPerPixel * sizeof(uint16_t); // 6
        const uint32_t offXRes    = offSamFmt + samplesPerPixel * sizeof(uint16_t); // 12
        const uint32_t offYRes    = offXRes   + 8; // 20
        const uint32_t offPixData = static_cast<uint32_t>(OFF_AUX + 28); // 6+6+8+8 = 28
        const size_t OFF_PIX      = OFF_AUX + 28;
        size_t totalSize          = OFF_PIX + pdSize;

        ByteBuffer buf(totalSize, ByteOrder::LittleEndian);

        // --- HEADER ---
        buf.write<uint8_t>('I'); buf.write<uint8_t>('I');
        buf.write<uint16_t>(42);
        buf.write<uint32_t>(static_cast<uint32_t>(OFF_IFD));

        // --- IFD ---
        buf.write<uint16_t>(ENTRY_COUNT);
        auto entry = [&](uint16_t tag, uint16_t type, uint32_t count, uint32_t valOff){
            buf.write<uint16_t>(tag);
            buf.write<uint16_t>(type);
            buf.write<uint32_t>(count);
            buf.write<uint32_t>(valOff);
        };

        entry(256, 4, 1, static_cast<uint32_t>(width_));              // ImageWidth
        entry(257, 4, 1, static_cast<uint32_t>(height_));             // ImageLength
        entry(258, 3, samplesPerPixel, offBits);                      // BitsPerSample
        entry(259, 3, 1,               1);                            // Compression = none
        entry(262, 3, 1,               2);                            // Photometric = RGB
        entry(273, 4, 1,               offPixData);                   // StripOffsets
        entry(277, 3, 1,               samplesPerPixel);              // SamplesPerPixel
        entry(278, 4, 1, static_cast<uint32_t>(height_));             // RowsPerStrip
        entry(279, 4, 1, static_cast<uint32_t>(pdSize));              // StripByteCounts
        entry(282, 5, 1,               offXRes);                      // XResolution
        entry(283, 5, 1,               offYRes);                      // YResolution
        entry(339, 3, samplesPerPixel, offSamFmt);                    // SampleFormat
        buf.write<uint32_t>(0);                                       // nextIFD = 0

        // --- AUX DATA ---
        for (int i = 0; i < samplesPerPixel; ++i)
            buf.write<uint16_t>(bitsPerSample);
        for (int i = 0; i < samplesPerPixel; ++i)
            buf.write<uint16_t>(sampleFormat);
        // XResolution = 72/1
        buf.write<uint32_t>(72); buf.write<uint32_t>(1);
        // YResolution = 72/1
        buf.write<uint32_t>(72); buf.write<uint32_t>(1);

        // --- PIXEL DATA ---
        for (const auto &P : data_) {
            buf.write<T>(P.r);
            buf.write<T>(P.g);
            buf.write<T>(P.b);
        }

        // --- WRITE TO DISK ---
        std::ofstream ofs(path, std::ios::binary);
        if (!ofs) throw std::runtime_error("Cannot open file for writing");
        ofs.write(reinterpret_cast<const char*>(buf.data()), buf.length());
    }
    

} // namespace ParticleZoo