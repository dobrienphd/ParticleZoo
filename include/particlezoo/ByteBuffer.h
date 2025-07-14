#pragma once

#ifdef BYTE_ORDER
#undef BYTE_ORDER
#endif

#include <cstring>
#include <stdexcept>
#include <vector>
#include <type_traits>
#include <span>
#include <bit>
#include <algorithm>
#include <array>
#include <istream>
#include <ostream>

namespace ParticleZoo {

    using byte = unsigned char;
    using signed_byte = char;

    enum class ByteOrder {
        LittleEndian = 1234,
        BigEndian = 4321,
        PDPEndian = 3412
    };
    
    constexpr unsigned int DEFAULT_BUFFER_SIZE = 4096;

    constexpr ByteOrder HOST_BYTE_ORDER = 
    (std::endian::native == std::endian::little) ? ByteOrder::LittleEndian :
    (std::endian::native == std::endian::big)    ? ByteOrder::BigEndian :
                                                  ByteOrder::PDPEndian;

    enum class FormatType { BINARY, ASCII, NONE };

    class ByteBuffer {
        public:
            // Create an empty ByteBuffer with a fixed capacity
            ByteBuffer(std::size_t bufferSize, ByteOrder byteOrder = HOST_BYTE_ORDER);

            // Create a ByteBuffer from a subspan of data
            ByteBuffer(const std::span<const byte> data, ByteOrder byteOrder = HOST_BYTE_ORDER);


            // Initialize from raw data via std::span
            std::size_t setData(std::span<const byte> data);

            // Initialize from an input stream
            std::size_t setData(std::istream & stream);

            // Append new data from the stream into the buffer (after current data)
            std::size_t appendData(std::istream & stream);

            // Append new data from another buffer into the buffer (after current data)
            std::size_t appendData(ByteBuffer & buffer, bool ignoreOffset = false);

            // Reset the buffer, clearing all data
            void clear();

            // Move the buffer offset to a specific position
            void moveTo(std::size_t offset);

            // Compact the buffer by shifting unread data to the front
            void compact();

            // Expand the buffer to its full capacity, filling the unused space with zeros
            void expand();

            // Write the buffer data to an output stream
            friend std::ostream& operator<<(std::ostream& os, const ByteBuffer &buffer);

            // Read a primitive type T (e.g., int, float)
            template<typename T>
            T read();

            // Read a null-terminated string from the buffer
            std::string readString();

            // Read a string of a specified length from the buffer
            std::string readString(std::size_t stringLength);
            
            // Read a line of ASCII text from the buffer
            std::string readLine();

            // Read a span of bytes from the buffer
            std::span<const byte> readBytes(std::size_t len);


            // Write a primitive type T to the buffer
            template<typename T>
            void write(const T &value);

            // Write a string to the buffer
            void writeString(const std::string & str);

            // Write a span of bytes to the buffer
            void writeBytes(std::span<const byte> data);


            // Get the length of the data in the buffer
            std::size_t length() const;

            // Get the remaining length of the data in the buffer for reading
            std::size_t remainingToRead() const;

            // Get the remaining length of the data in the buffer for writing
            std::size_t remainingToWrite() const;

            // Get the buffer capacity
            std::size_t capacity() const;

            // Get a pointer to the buffer data
            const byte* data() const;
            
            // Set the byte order of the data in the buffer
            void setByteOrder(ByteOrder byteOrder);

            // Get the byte order of the data in the buffer
            ByteOrder getByteOrder() const;

        private:
            std::vector<byte> buffer_;  // contiguous memory buffer
            std::size_t capacity_;      // buffer maximum capacity
            std::size_t offset_;        // current offset
            std::size_t length_;        // length of data written to the buffer
            ByteOrder byteOrder_;       // byte order of the data

            // Swap the byte order of a value
            // template<typename T>
            // static T swapByteOrder(T value);

            // Reorder bytes of a value to match the target byte order
            template<typename T>
            static T reorderBytes(T value, ByteOrder targetByteOrder);
    };


    /* Implementation of ByteBuffer - kept inline for performance */


    // Constructors

    inline ByteBuffer::ByteBuffer(std::size_t bufferSize, ByteOrder byteOrder) : buffer_(bufferSize, 0), capacity_(bufferSize), offset_(0), length_(0), byteOrder_(byteOrder) {
        if (bufferSize == 0) {
            throw std::runtime_error("Buffer size must be positive.");
        }
    }

    inline ByteBuffer::ByteBuffer(const std::span<const byte> data, ByteOrder byteOrder)
        : buffer_(data.begin(), data.end()),
            capacity_(data.size()),
            offset_(0),
            length_(data.size()),
            byteOrder_(byteOrder)
    { }


    // Accessors

    inline std::size_t ByteBuffer::length() const { return length_; }
    inline std::size_t ByteBuffer::remainingToRead() const { return length_ - offset_; }
    inline std::size_t ByteBuffer::remainingToWrite() const { return capacity_ - length_; }
    inline std::size_t ByteBuffer::capacity() const { return capacity_; }
    inline const byte* ByteBuffer::data() const { return buffer_.data(); }

    // Data Operations

    inline std::size_t ByteBuffer::setData(std::span<const byte> data) {
        if (data.size() > buffer_.size()) {
            throw std::runtime_error("Data length exceeds buffer size.");
        }
        offset_ = 0;
        length_ = data.size();
        std::memcpy(buffer_.data(), data.data(), data.size());
        return length_;
    }
    
    inline std::size_t ByteBuffer::setData(std::istream & stream) {
        stream.read(reinterpret_cast<char*>(buffer_.data()), capacity_);
        std::streamsize rawCount = stream.gcount();
        std::size_t bytesRead = static_cast<std::size_t>(rawCount);
        if (bytesRead == 0) {
            throw std::runtime_error("Failed to read any data from stream.");
        }
        offset_ = 0;
        length_ = bytesRead;
        return length_;
    }
    
    inline std::size_t ByteBuffer::appendData(std::istream & stream) {
        std::size_t spaceLeft = capacity_ - length_;
        if (spaceLeft == 0) {
            throw std::runtime_error("Buffer is already full, cannot append more data.");
        }
        stream.read(reinterpret_cast<char*>(buffer_.data() + length_), spaceLeft);
        std::streamsize rawCount = stream.gcount();
        std::size_t bytesRead = static_cast<std::size_t>(rawCount);
        if (bytesRead == 0) {
            throw std::runtime_error("Failed to read additional data from stream.");
        }
        length_ += bytesRead;
        return bytesRead;
    }

    inline std::size_t ByteBuffer::appendData(ByteBuffer & src, bool ignoreOffset) {
        std::size_t srcOffset = ignoreOffset ? 0 : src.offset_;
        std::size_t dataSize;
        if (ignoreOffset)
            dataSize = src.length_;
        else
            dataSize = src.length_ - src.offset_;
        if (length_ + dataSize > capacity_) {
            throw std::runtime_error("Data length exceeds buffer capacity.");
        }
        std::memcpy(buffer_.data() + length_, src.buffer_.data() + srcOffset, dataSize);
        length_ += dataSize;
        return dataSize;
    }

    inline void ByteBuffer::compact() {
        std::size_t remainingBytes = remainingToRead();
        if (remainingBytes > 0) {
            std::memmove(buffer_.data(), buffer_.data() + offset_, remainingBytes);
        }
        offset_ = 0;
        length_ = remainingBytes;
    }

    inline void ByteBuffer::expand() {
        // write zeros to the rest of the buffer
        std::size_t remainingBytes = capacity_ - length_;
        if (remainingBytes > 0) {
            std::memset(buffer_.data() + length_, 0, remainingBytes);
            length_ = capacity_;
        }
    }

    inline void ByteBuffer::clear() { offset_ = 0; length_ = 0; }

    inline void ByteBuffer::moveTo(std::size_t offset) {
        if (offset > length_) {
            throw std::runtime_error("Offset exceeds data length.");
        }
        offset_ = offset;
    }


    // Read functions
    
    template<typename T>
    inline T ByteBuffer::read() {
        static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable.");
        if (offset_ + sizeof(T) > length_) {
            throw std::runtime_error("Not enough data to read the requested type.");
        }
        T value;
        std::memcpy(&value, buffer_.data() + offset_, sizeof(T));
        offset_ += sizeof(T);
        value = reorderBytes(value, byteOrder_);
        return value;
    }

    inline std::string ByteBuffer::readString() {
        std::size_t start = offset_;
        while (offset_ < length_ && buffer_[offset_] != '\0') {
            offset_++;
        }
        if (offset_ >= length_) {
            offset_ = start; // Reset offset to start if null terminator not found
            throw std::runtime_error("Not enough data in buffer to read string.");
        }
        std::string result(reinterpret_cast<const char*>(buffer_.data() + start), offset_ - start);
        offset_++; // Skip the null terminator
        return result;
    }

    inline std::string ByteBuffer::readString(std::size_t stringLength) {
        if (offset_ + stringLength > length_) {
            throw std::runtime_error("Not enough data in buffer to read string.");
        }
        std::string result(reinterpret_cast<const char*>(buffer_.data() + offset_), stringLength);
        offset_ += stringLength;
        return result;
    }

    inline std::string ByteBuffer::readLine() {
        std::size_t unread = remainingToRead();
        if (unread == 0) {
            throw std::runtime_error("No data left in buffer to read line.");
        }
        // Search for '\n' using memchr
        auto startPtr = reinterpret_cast<const char*>(buffer_.data() + offset_);
        const void* newlinePtr = std::memchr(startPtr, '\n', unread);
        if (!newlinePtr) {
            throw std::runtime_error("Not enough data in buffer to read line.");
        }
        std::size_t lineLength = static_cast<const char*>(newlinePtr) - startPtr;
        // Exclude a trailing '\r' if present
        if (lineLength > 0 && startPtr[lineLength - 1] == '\r') {
            lineLength--;
        }
        std::string result(startPtr, lineLength);
        // Advance offset past the newline
        offset_ += lineLength + 1;
        return result;
    }
    

    inline std::span<const byte> ByteBuffer::readBytes(std::size_t len) {
        if (offset_ + len > length_) {
            throw std::runtime_error("Not enough data in buffer.");
        }
        std::span<const byte> data(buffer_.data() + offset_, len);
        offset_ += len;
        return data;
    }


    // Write functions
    template<typename T>
    inline void ByteBuffer::write(const T &value) {
        static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable.");
        if (offset_ + sizeof(T) > buffer_.size()) {
            throw std::runtime_error("Data length exceeds buffer capacity.");
        }
        T tmp = reorderBytes(value, byteOrder_);
        std::memcpy(buffer_.data() + offset_, &tmp, sizeof(T));
        offset_ += sizeof(T);
        if (offset_ > length_) {
            length_ = offset_;
        }
    }

    inline void ByteBuffer::writeString(const std::string & str) {
        std::size_t strSize = str.size();
        if (offset_ + strSize > buffer_.size()) {
            throw std::runtime_error("String length exceeds buffer capacity.");
        }
        std::memcpy(buffer_.data() + offset_, str.data(), strSize);
        offset_ += strSize;
        if (offset_ > length_) {
            length_ = offset_;
        }
    }

    inline void ByteBuffer::writeBytes(const std::span<const byte> data) {
        std::size_t dataSize = data.size();
        if (offset_ + dataSize > buffer_.size()) {
            throw std::runtime_error("Data length exceeds buffer capacity.");
        }
        std::memcpy(buffer_.data() + offset_, data.data(), dataSize);
        offset_ += dataSize;
        if (offset_ > length_) {
            length_ = offset_;
        }
    }


    // Byte Order Management

    inline void ByteBuffer::setByteOrder(ByteOrder byteOrder) { byteOrder_ = byteOrder; }
    inline ByteOrder ByteBuffer::getByteOrder() const { return byteOrder_; }

    // template<typename T>
    // T ByteBuffer::swapByteOrder(T value) {
    //     static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable.");
    //     std::array<byte, sizeof(T)> bytes = std::bit_cast<std::array<byte, sizeof(T)>>(value);
    //     std::reverse(bytes.begin(), bytes.end());
    //     return std::bit_cast<T>(bytes);
    // }

    template<typename T>
    inline T ByteBuffer::reorderBytes(T v, ByteOrder target) {
        if (target == HOST_BYTE_ORDER || sizeof(T) == 1) return v;

        auto bytes = std::bit_cast<std::array<byte, sizeof(T)>>(v);
        std::array<byte, sizeof(T)> out;

        if (target == ByteOrder::BigEndian
        || target == ByteOrder::LittleEndian) {
            // full reverse
            std::reverse_copy(bytes.begin(), bytes.end(), out.begin());
        }
        else { // PDP-endian: swap *inside* each 2-byte word, keep word order
            constexpr size_t W = 2;
            size_t words = sizeof(T) / W;
            for (size_t i = 0; i < words; ++i) {
                out[i*W + 0] = bytes[i*W + 1];
                out[i*W + 1] = bytes[i*W + 0];
            }
        }

        return std::bit_cast<T>(out);
    }

    inline std::ostream& operator<<(std::ostream &os, ByteOrder byteOrder) {
        switch (byteOrder) {
            case ByteOrder::LittleEndian: os << "Little Endian"; break;
            case ByteOrder::BigEndian:    os << "Big Endian"; break;
            case ByteOrder::PDPEndian:    os << "PDP Endian"; break;
            default:                      os << "Unknown Byte Order"; break;
        }
        return os;
    }

    inline std::ostream& operator<<(std::ostream& os, const ByteBuffer &buffer) {
        os.write(reinterpret_cast<const char*>(buffer.buffer_.data()), buffer.length_);
        return os;
    }

} // namespace ParticleZoo