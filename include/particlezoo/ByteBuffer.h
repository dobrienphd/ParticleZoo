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

    using byte = unsigned char;         ///< Type alias for unsigned byte (8 bits)
    using signed_byte = char;           ///< Type alias for signed byte (8 bits)

    /**
     * @brief Enumeration of byte ordering schemes for multi-byte data types.
     * 
     * Defines the different ways multi-byte values can be stored in memory,
     * for cross-platform compatibility when reading/writing binary data files.
     */
    enum class ByteOrder {
        LittleEndian = 1234,    ///< Least significant byte first
        BigEndian = 4321,       ///< Most significant byte first
        PDPEndian = 3412        ///< Mixed endian
    };
 
    constexpr std::size_t DEFAULT_BUFFER_SIZE = 1048576;   ///< Default buffer size (1MiB)

    /**
     * @brief The byte order of the host system.
     * 
     * Automatically determined at compile time based on the system's native byte order.
     */
    constexpr ByteOrder HOST_BYTE_ORDER = 
    (std::endian::native == std::endian::little) ? ByteOrder::LittleEndian :
    (std::endian::native == std::endian::big)    ? ByteOrder::BigEndian :
                                                  ByteOrder::PDPEndian;

    /**
     * @brief Enumeration of file format types.
     */
    enum class FormatType { 
        BINARY,     ///< Binary format
        ASCII,      ///< ASCII text format
        NONE        ///< Used for when ParticleZoo will not be responsible for reading/writing (e.g. ROOT)
    };

    /**
     * @brief Byte buffer used to improve I/O performance for reading and writing binary and text data.
     * 
     * ByteBuffer provides efficient buffered I/O operations with automatic byte order conversion
     * for cross-platform compatibility. It supports both reading from and writing to the buffer,
     * with automatic capacity management and various data type read/write operations.
     * 
     * The buffer maintains both a current offset (read/write position) and a length
     * (amount of valid data), allowing for flexible positioning and partial reads/writes.
     */
    class ByteBuffer {
        public:
            /**
             * @brief Create an empty ByteBuffer with a fixed capacity.
             * 
             * @note The capacity must be greater than zero.
             * @param bufferSize The maximum capacity of the buffer in bytes (default: DEFAULT_BUFFER_SIZE)
             * @param byteOrder The byte order for multi-byte data types (default: HOST_BYTE_ORDER)
             * @throws std::runtime_error if bufferSize is zero
             */
            ByteBuffer(std::size_t bufferSize = DEFAULT_BUFFER_SIZE, ByteOrder byteOrder = HOST_BYTE_ORDER);

            /**
             * @brief Create a ByteBuffer from a span of existing data.
             * 
             * The buffer is initialized with a copy of the provided data.
             * 
             * @param data A span containing the initial data to copy into the buffer
             * @param byteOrder The byte order for multi-byte data types (default: HOST_BYTE_ORDER)
             */
            ByteBuffer(const std::span<const byte> data, ByteOrder byteOrder = HOST_BYTE_ORDER);


            /**
             * @brief Initialize the buffer with data from a span.
             * 
             * Replaces any existing data in the buffer. Resets the offset to 0.
             * 
             * @param data A span containing the data to copy into the buffer
             * @return std::size_t The number of bytes copied
             * @throws std::runtime_error if data size exceeds buffer capacity
             */
            std::size_t setData(std::span<const byte> data);

            /**
             * @brief Initialize the buffer with data from an input stream.
             * 
             * Reads up to the buffer's capacity from the stream. Replaces any existing data.
             * Resets the offset to 0.
             * 
             * @param stream The input stream to read from
             * @return std::size_t The number of bytes read from the stream
             * @throws std::runtime_error if no data could be read from the stream
             */
            std::size_t setData(std::istream & stream);

            /**
             * @brief Append data from an input stream to the existing buffer content.
             * 
             * Reads additional data from the stream and appends it after the current data.
             * Does not modify the current offset.
             * 
             * @param stream The input stream to read from
             * @return std::size_t The number of bytes appended from the stream
             * @throws std::runtime_error if buffer is full or no data could be read
             */
            std::size_t appendData(std::istream & stream);

            /**
             * @brief Append data from another ByteBuffer to this buffer.
             * 
             * Copies data from the source buffer and appends it after the current data.
             * 
             * @param buffer The source ByteBuffer to copy data from
             * @param ignoreOffset If true, copies all data from source; if false, only unread data
             * @return std::size_t The number of bytes appended
             * @throws std::runtime_error if the combined data exceeds buffer capacity
             */
            std::size_t appendData(ByteBuffer & buffer, bool ignoreOffset = false);

            /**
             * @brief Reset the buffer, resetting the offset and length to 0.
             */
            void clear();

            /**
             * @brief Move the read/write offset to a specific position in the buffer.
             * 
             * @param offset The new offset position (must be <= current data length)
             * @throws std::runtime_error if offset exceeds the current data length
             */
            void moveTo(std::size_t offset);

            /**
             * @brief Compact the buffer by moving unread data to the beginning.
             * 
             * Shifts any unread data (from current offset to end) to the start of the buffer
             * and updates the length and offset accordingly. Useful for reclaiming space
             * after partial reads.
             */
            void compact();

            /**
             * @brief Expand the buffer to its full capacity, filling unused space with zeros.
             * 
             * Extends the data length to the full buffer capacity by filling the remaining
             * space with zero-ed bytes.
             */
            void expand();

            /**
             * @brief Write the buffer data to an output stream.
             * 
             * @param os The output stream to write to
             * @param buffer The ByteBuffer to write from
             * @return std::ostream& Reference to the output stream for chaining
             */
            friend std::ostream& operator<<(std::ostream& os, const ByteBuffer &buffer);

            /**
             * @brief Read a primitive type T from the buffer with automatic byte order conversion.
             * 
             * Reads sizeof(T) bytes from the current offset and converts the byte order
             * if necessary. Advances the offset by sizeof(T).
             * 
             * @tparam T The primitive type to read (must be trivially copyable)
             * @return T The value read from the buffer
             * @throws std::runtime_error if insufficient data is available
             */
            template<typename T>
            T read();

            /**
             * @brief Read a null-terminated string from the buffer.
             * 
             * Reads characters until a null terminator ('\0') is found. Advances the offset
             * past the null terminator.
             * 
             * @return std::string The string read from the buffer (without null terminator)
             * @throws std::runtime_error if null terminator is not found or insufficient data
             */
            std::string readString();

            /**
             * @brief Read a string of specified length from the buffer.
             * 
             * Reads exactly the specified number of characters. Advances the offset
             * by the string length.
             * 
             * @param stringLength The number of characters to read
             * @return std::string The string read from the buffer
             * @throws std::runtime_error if insufficient data is available
             */
            std::string readString(std::size_t stringLength);
            
            /**
             * @brief Read a line of ASCII text from the buffer.
             * 
             * Reads characters until a newline ('\n') is found. Automatically handles
             * Windows-style line endings by removing trailing '\r'. Advances the offset
             * past the newline.
             * 
             * @return std::string The line read from the buffer (without newline characters)
             * @throws std::runtime_error if newline is not found or no data is available
             */
            std::string readLine();

            /**
             * @brief Read a span of bytes from the buffer.
             * 
             * Returns a view of the requested bytes without copying. Advances the offset
             * by the requested length.
             * 
             * @param len The number of bytes to read
             * @return std::span<const byte> A span view of the requested bytes
             * @throws std::runtime_error if insufficient data is available
             */
            std::span<const byte> readBytes(std::size_t len);


            /**
             * @brief Write a primitive type T to the buffer with automatic byte order conversion.
             * 
             * Converts the value to the buffer's byte order if necessary and writes sizeof(T)
             * bytes. Advances the offset by sizeof(T) and updates the length if necessary.
             * 
             * @tparam T The primitive type to write (must be trivially copyable)
             * @param value The value to write to the buffer
             * @throws std::runtime_error if insufficient space is available
             */
            template<typename T>
            void write(const T &value);

            /**
             * @brief Write a string to the buffer.
             * 
             * Writes the string's characters with or without (default) a null terminator.
             * Advances the offset by the string length and updates the length if necessary.
             * 
             * @param str The string to write to the buffer
             * @param includeNullTerminator If true, appends a null terminator after the string
             * @throws std::runtime_error if insufficient space is available
             */
            void writeString(const std::string & str, bool includeNullTerminator = false);

            /**
             * @brief Write a span of bytes to the buffer.
             * 
             * Copies the bytes from the span to the buffer. Advances the offset by the
             * data size and updates the length if necessary.
             * 
             * @param data A span containing the bytes to write
             * @throws std::runtime_error if insufficient space is available
             */
            void writeBytes(std::span<const byte> data);


            /**
             * @brief Get the length of valid data in the buffer.
             * 
             * @return std::size_t The number of bytes of valid data
             */
            std::size_t length() const;

            /**
             * @brief Get the number of bytes remaining to be read from current offset.
             * 
             * @return std::size_t The number of unread bytes (length - offset)
             */
            std::size_t remainingToRead() const;

            /**
             * @brief Get the number of bytes available for writing.
             * 
             * @return std::size_t The remaining capacity (capacity - length)
             */
            std::size_t remainingToWrite() const;

            /**
             * @brief Get the total capacity of the buffer.
             * 
             * @return std::size_t The maximum number of bytes the buffer can hold
             */
            std::size_t capacity() const;

            /**
             * @brief Get a pointer to the raw buffer data.
             * 
             * @return const byte* Pointer to the beginning of the buffer data
             */
            const byte* data() const;
            
            /**
             * @brief Set the byte order for interpreting multi-byte data types.
             * 
             * @param byteOrder The byte order to use for subsequent read/write operations
             */
            void setByteOrder(ByteOrder byteOrder);

            /**
             * @brief Get the current byte order setting.
             * 
             * @return ByteOrder The byte order used for multi-byte data types
             */
            ByteOrder getByteOrder() const;

        private:
            std::vector<byte> buffer_;  // contiguous memory buffer
            std::size_t capacity_;      // buffer maximum capacity
            std::size_t offset_;        // current offset
            std::size_t length_;        // length of data written to the buffer
            ByteOrder byteOrder_;       // byte order of the data

            /**
             * @brief Reorder bytes of a value to match the target byte order.
             * 
             * Converts multi-byte values between different byte orders. Single-byte values
             * are returned unchanged. Handles little-endian, big-endian, and PDP-endian formats.
             * 
             * @tparam T The type of value to reorder (must be trivially copyable)
             * @param value The value to reorder
             * @param targetByteOrder The target byte order
             * @return T The value with bytes reordered for the target byte order
             */
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

    inline void ByteBuffer::writeString(const std::string & str, bool includeNullTerminator) {
        std::size_t strSize = str.size();
        if (offset_ + strSize + (includeNullTerminator ? 1 : 0) > buffer_.size()) {
            throw std::runtime_error("String length exceeds buffer capacity.");
        }
        std::memcpy(buffer_.data() + offset_, str.data(), strSize);
        offset_ += strSize;
        if (includeNullTerminator) {
            buffer_[offset_] = 0;
            offset_++;
        }
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