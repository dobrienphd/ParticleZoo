#pragma once

#include "particlezoo/ByteBuffer.h"

#include "particlezoo/utilities/pzimages.h"
#include "particlezoo/utilities/units.h"

#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>
#include <limits>
#include <algorithm>
#include <type_traits>
#include <numeric>
#include <cmath>
#include <utility>
#include <cstdint>

namespace ParticleZoo {

    // Utility functions for TIFF RATIONAL conversion
    inline std::pair<uint32_t, uint32_t> toURational(double value) {
        if (!std::isfinite(value) || value < 0.0) {
            return {0, 1};
        }
        // For simplicity, use denominator of 1000000 for reasonable precision
        constexpr uint32_t denominator = 1000000;
        uint32_t numerator = static_cast<uint32_t>(std::round(value * denominator));
        return {numerator, denominator};
    }

    inline std::pair<int32_t, int32_t> toSRational(double value) {
        if (!std::isfinite(value)) {
            return {0, 1};
        }
        // For simplicity, use denominator of 1000000 for reasonable precision
        constexpr int32_t denominator = 1000000;
        int32_t numerator = static_cast<int32_t>(std::round(value * denominator));
        return {numerator, denominator};
    }

    template<typename T>
    class TiffImage : public Image<T> {
        static_assert(std::is_arithmetic_v<T> && "TiffImage type must be arithmetic");

    public:
        TiffImage(int width, int height, T xPixelsPerUnitLength = T(1), T yPixelsPerUnitLength = T(1), T xOffset = T(0), T yOffset = T(0));

        int width() const override;
        int height() const override;
        void setGrayscaleValue(int x, int y, const T& value) override;
        void setPixel(int x, int y, const Pixel<T>& p) override;
        void setPixel(int x, int y, const T& R, const T& G, const T& B) override;
        Pixel<T> getPixel(int x, int y) const override;
        T getGrayscaleValue(int x, int y) const override;
        void normalize(T normalizationFactor) override;
        void save(const std::string& path) const override;

    private:
        int width_, height_;
        T minValue_, maxValue_;

        // calibration data
        T xPixelsPerUnitLength_;
        T yPixelsPerUnitLength_;
        T xOffset_;
        T yOffset_;

        bool isEmpty_;

        std::vector<T> data_;  // Store raw grayscale values instead of Pixel<T>
    };


    /* Inline implementation of TiffImage */

    template<typename T>
    inline TiffImage<T>::TiffImage(int w, int h, T xPixelsPerUnitLength, T yPixelsPerUnitLength, T xOffset, T yOffset)
        : width_(w), height_(h),
          minValue_(std::numeric_limits<T>::max()),
          maxValue_(std::numeric_limits<T>::lowest()),
          xPixelsPerUnitLength_(xPixelsPerUnitLength),
          yPixelsPerUnitLength_(yPixelsPerUnitLength),
          xOffset_(xOffset),
          yOffset_(yOffset),
          isEmpty_(true),
          data_(static_cast<size_t>(w) * static_cast<size_t>(h), T{})
    {
        if (w <= 0 || h <= 0) throw std::runtime_error("Invalid dimensions");
    }

    template<typename T>
    inline int TiffImage<T>::width() const { return width_; }

    template<typename T>
    inline int TiffImage<T>::height() const { return height_; }

    template<typename T>
    inline void TiffImage<T>::setGrayscaleValue(int x, int y, const T& value) {
        if (x < 0 || x >= width_ || y < 0 || y >= height_) throw std::runtime_error("Pixel out of range");
        size_t idx = static_cast<size_t>(y) * static_cast<size_t>(width_) + static_cast<size_t>(x);
        data_[idx] = value;
        if (value > maxValue_) maxValue_ = value;
        if (value < minValue_) minValue_ = value;
        isEmpty_ = false;
    }

    template<typename T>
    inline void TiffImage<T>::setPixel(int, int, const Pixel<T>&) {
        throw std::runtime_error("setPixel with Pixel<T> not supported for TiffImage");
    }

    template<typename T>
    inline void TiffImage<T>::setPixel(int, int, const T&, const T&, const T&) {
        throw std::runtime_error("setPixel with RGB values not supported for TiffImage");
    }

    template<typename T>
    inline Pixel<T> TiffImage<T>::getPixel(int x, int y) const {
        T grayscale = getGrayscaleValue(x, y);
        return Pixel<T>(grayscale, grayscale, grayscale);
    }

    template<typename T>
    inline T TiffImage<T>::getGrayscaleValue(int x, int y) const {
        if (x < 0 || x >= width_ || y < 0 || y >= height_) throw std::runtime_error("Pixel out of range");
        size_t idx = static_cast<size_t>(y) * static_cast<size_t>(width_) + static_cast<size_t>(x);
        return data_[idx];
    }

    template<typename T>
    inline void TiffImage<T>::normalize(T normalizationFactor) {
        if (normalizationFactor <= 0) throw std::runtime_error("Normalization factor must be greater than zero.");
        for (auto& value : data_) {
            value /= normalizationFactor;
        }
        minValue_ /= normalizationFactor;
        maxValue_ /= normalizationFactor;
    }

    template<typename T>
    inline void TiffImage<T>::save(const std::string& path) const {
        // Validate dynamic range
        if (!(maxValue_ >= minValue_) && !isEmpty_) {
            throw std::runtime_error("Invalid dynamic range for TIFF: maxValue < minValue");
        }

        // Convert calibration to centimeters because ResolutionUnit=3 (centimeter)
        const double xPixelsPerCM = static_cast<double>(xPixelsPerUnitLength_) / static_cast<double>(cm);
        const double yPixelsPerCM = static_cast<double>(yPixelsPerUnitLength_) / static_cast<double>(cm);
        const double xOffsetCM    = static_cast<double>(xOffset_) / static_cast<double>(cm);
        const double yOffsetCM    = static_cast<double>(yOffset_) / static_cast<double>(cm);

        // Basic validation for resolution/position values
        if (!(std::isfinite(xPixelsPerCM) && std::isfinite(yPixelsPerCM)) ||
            xPixelsPerCM <= 0.0 || yPixelsPerCM <= 0.0) {
            throw std::runtime_error("X/Y resolution must be finite and > 0.");
        }
        if (!(std::isfinite(xOffsetCM) && std::isfinite(yOffsetCM))) {
            throw std::runtime_error("X/Y position (offset) must be finite.");
        }

        // Determine the sample format and bits per sample based on the data type T
        uint16_t sampleFormat;
        uint16_t bitsPerSample;
        
        if constexpr (std::is_floating_point_v<T>) {
            sampleFormat = 3; // IEEE floating point
            if constexpr (sizeof(T) == 4) {
                bitsPerSample = 32;
            } else if constexpr (sizeof(T) == 8) {
                bitsPerSample = 64;
            } else {
                throw std::runtime_error("Unsupported floating point type size");
            }
        } else if constexpr (std::is_signed_v<T>) {
            sampleFormat = 2; // Two's complement signed integer
            bitsPerSample = sizeof(T) * 8;
        } else {
            sampleFormat = 1; // Unsigned integer
            bitsPerSample = sizeof(T) * 8;
        }

        // TIFF constants
        constexpr uint16_t TIFF_MAGIC_LE = 0x4949; // "II" for little endian
        constexpr uint16_t TIFF_VERSION = 42;
        constexpr uint16_t TAG_IMAGEWIDTH = 256;
        constexpr uint16_t TAG_IMAGELENGTH = 257;
        constexpr uint16_t TAG_BITSPERSAMPLE = 258;
        constexpr uint16_t TAG_COMPRESSION = 259;
        constexpr uint16_t TAG_PHOTOMETRIC = 262;
        constexpr uint16_t TAG_STRIPOFFSETS = 273;
        constexpr uint16_t TAG_SAMPLESPERPIXEL = 277;
        constexpr uint16_t TAG_ROWSPERSTRIP = 278;
        constexpr uint16_t TAG_STRIPBYTECOUNTS = 279;
        constexpr uint16_t TAG_XRESOLUTION = 282;
        constexpr uint16_t TAG_YRESOLUTION = 283;
        constexpr uint16_t TAG_RESOLUTIONUNIT = 296;
        constexpr uint16_t TAG_SAMPLEFORMAT = 339;
        constexpr uint16_t TAG_XPOSITION = 286;
        constexpr uint16_t TAG_YPOSITION = 287;

        // Data type constants for IFD entries
        constexpr uint16_t TYPE_SHORT = 3;
        constexpr uint16_t TYPE_LONG = 4;
        constexpr uint16_t TYPE_RATIONAL = 5;
        constexpr uint16_t TYPE_SRATIONAL = 10;

        // Calculate data size
        const size_t totalImageDataSize = data_.size() * sizeof(T);

        // Calculate exact buffer size by counting all write operations
        const size_t tiffHeaderSize = 8;                     // magic(2) + version(2) + ifd_offset(4)
        const size_t ifdHeaderSize = 2;                      // number of entries(2)
        const size_t ifdEntriesSize = 15 * 12;               // 15 entries × 12 bytes each
        const size_t nextIfdOffset = 4;                      // next IFD offset(4)
        const size_t rationalDataSize = 4 * 8;               // 4 rational values × 8 bytes each
        const size_t totalHeaderSize = tiffHeaderSize + ifdHeaderSize + ifdEntriesSize + nextIfdOffset + rationalDataSize;
        
        // Create ByteBuffer with exact capacity needed
        const size_t bufferSize = totalHeaderSize + totalImageDataSize;

        ByteBuffer buffer(bufferSize, ByteOrder::LittleEndian);

        // Write TIFF header
        buffer.write<uint16_t>(TIFF_MAGIC_LE);
        buffer.write<uint16_t>(TIFF_VERSION);
        const uint32_t ifdOffset = 8; // IFD starts right after header
        buffer.write<uint32_t>(ifdOffset);

        // Write IFD
        const uint16_t numEntries = 15;
        buffer.write<uint16_t>(numEntries);

        // Calculate offsets for data that doesn't fit in IFD entries
        uint32_t currentOffset = 8 + 2 + numEntries * 12 + 4; // After IFD
        const uint32_t xResolutionOffset = currentOffset; currentOffset += 8;
        const uint32_t yResolutionOffset = currentOffset; currentOffset += 8;
        const uint32_t xPositionOffset = currentOffset; currentOffset += 8;
        const uint32_t yPositionOffset = currentOffset; currentOffset += 8;
        const uint32_t stripDataOffset = currentOffset;

        // Helper lambda to write IFD entry
        auto writeIFDEntry = [&](uint16_t tag, uint16_t type, uint32_t count, uint32_t valueOrOffset) {
            buffer.write<uint16_t>(tag);
            buffer.write<uint16_t>(type);
            buffer.write<uint32_t>(count);
            buffer.write<uint32_t>(valueOrOffset);
        };

        // Write IFD entries
        writeIFDEntry(TAG_IMAGEWIDTH, TYPE_LONG, 1, static_cast<uint32_t>(width_));
        writeIFDEntry(TAG_IMAGELENGTH, TYPE_LONG, 1, static_cast<uint32_t>(height_));
        writeIFDEntry(TAG_BITSPERSAMPLE, TYPE_SHORT, 1, bitsPerSample);
        writeIFDEntry(TAG_COMPRESSION, TYPE_SHORT, 1, 1); // No compression
        writeIFDEntry(TAG_PHOTOMETRIC, TYPE_SHORT, 1, 1); // BlackIsZero for grayscale
        writeIFDEntry(TAG_STRIPOFFSETS, TYPE_LONG, 1, stripDataOffset);
        writeIFDEntry(TAG_SAMPLESPERPIXEL, TYPE_SHORT, 1, 1); // Grayscale
        writeIFDEntry(TAG_ROWSPERSTRIP, TYPE_LONG, 1, static_cast<uint32_t>(height_)); // Single strip
        writeIFDEntry(TAG_STRIPBYTECOUNTS, TYPE_LONG, 1, static_cast<uint32_t>(totalImageDataSize));
        writeIFDEntry(TAG_XRESOLUTION, TYPE_RATIONAL, 1, xResolutionOffset);
        writeIFDEntry(TAG_YRESOLUTION, TYPE_RATIONAL, 1, yResolutionOffset);
        writeIFDEntry(TAG_RESOLUTIONUNIT, TYPE_SHORT, 1, 3); // Centimeter
        writeIFDEntry(TAG_SAMPLEFORMAT, TYPE_SHORT, 1, sampleFormat);
        writeIFDEntry(TAG_XPOSITION, TYPE_SRATIONAL, 1, xPositionOffset);
        writeIFDEntry(TAG_YPOSITION, TYPE_SRATIONAL, 1, yPositionOffset);

        // Next IFD offset (0 means no more IFDs)
        buffer.write<uint32_t>(0);

        // Write rational values
        auto [xResNum, xResDen] = toURational(xPixelsPerCM);
        buffer.write<uint32_t>(xResNum);
        buffer.write<uint32_t>(xResDen);

        auto [yResNum, yResDen] = toURational(yPixelsPerCM);
        buffer.write<uint32_t>(yResNum);
        buffer.write<uint32_t>(yResDen);

        auto [xPosNum, xPosDen] = toSRational(xOffsetCM);
        buffer.write<int32_t>(xPosNum);
        buffer.write<int32_t>(xPosDen);

        auto [yPosNum, yPosDen] = toSRational(yOffsetCM);
        buffer.write<int32_t>(yPosNum);
        buffer.write<int32_t>(yPosDen);

        // Write pixel data
        for (const T& value : data_) {
            buffer.write<T>(value);
        }

        // Write to file
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file for writing: " + path);
        }
        
        file << buffer;
        if (file.fail()) {
            throw std::runtime_error("Failed to write TIFF data to file: " + path);
        }
        
    }
    

} // namespace ParticleZoo