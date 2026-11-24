#include "particlezoo/peneasy/penEasyphspFile.h"

#include <format>
#include <stdexcept>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>
#include <array>
#include <charconv>
#include <cstring>
#include <string_view>

#include "particlezoo/Particle.h"
#include "particlezoo/ByteBuffer.h"
#include "particlezoo/penelope/ILBArray.h"

namespace ParticleZoo::penEasyphspFile
{

    Writer::Writer(const std::string & fileName, const UserOptions & options)
    : PhaseSpaceFileWriter("penEasy", fileName, options, FormatType::ASCII)
    {}

    Writer::~Writer()
    {
        try {
            close();
        } catch (...) {
            // destructors must not throw; call close() explicitly to handle write errors
        }
    }

    void Writer::writeHeaderData(ByteBuffer & buffer)
    {
        buffer.writeString(std::string(FILE_HEADER));
    }
    
    const std::string Writer::writeASCIIParticle(Particle & particle)
    {
        int kpar;
        switch (particle.getType()) {
            case ParticleType::Photon:
                kpar = 2;
                break;
            case ParticleType::Electron:
                kpar = 1;
                break;
            case ParticleType::Positron:
                kpar = 3;
                break;
            case ParticleType::Proton:
                kpar = 4;
                break;
            default:
                throw std::runtime_error("Unsupported particle type.");
        }

        float e = particle.getKineticEnergy() * 1.e6f; // convert to eV
        float x = particle.getX();
        float y = particle.getY();
        float z = particle.getZ();
        float u = particle.getDirectionalCosineX();
        float v = particle.getDirectionalCosineY();
        float w = particle.getDirectionalCosineZ();
        float weight = particle.getWeight();

        int dn = 0;
        if (particle.hasIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER)) {
            dn = (int) particle.getIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER);
        }
        
        if (particle.isNewHistory() && dn < 1) {
            dn = 1;
        }

        std::array<int, 5> ilb = Penelope::ExtractILBArrayFromParticle(particle);
        
        // Create a std::string with the maximum required capacity.
        std::string formatted;
        formatted.resize(MAX_ASCII_LINE_LENGTH);

        // Write directly into the string's buffer.
        auto result = std::format_to_n(
            formatted.data(), formatted.size(),
            "{} {:14.7e} {:14.7e} {:14.7e} {:14.7e} {:14.7e} {:14.7e} {:14.7e} {:14.7e} {} {} {} {} {} {}\n",
            kpar, e, x, y, z, u, v, w, weight, static_cast<long long>(dn),
            ilb[0], ilb[1], ilb[2], ilb[3], ilb[4]);

        // Determine how many characters were actually written
        std::size_t output_length = static_cast<std::size_t>(result.out - formatted.data());
        if (output_length > MAX_ASCII_LINE_LENGTH) {
            throw std::runtime_error("Particle data exceeded maximum length per particle.");
        }

        // Resize the string to the actual output length.
        formatted.resize(output_length);
        return formatted;
    }



    // Parse DeltaN (the 10th whitespace-separated field) from a particle line, returning 0 if it cannot be parsed
    static int parseDeltaN(std::string_view line) {
        constexpr std::string_view whitespace = " \t\r\n\v\f";
        std::size_t pos = 0;
        for (int field = 0; field < 9; field++) {
            pos = line.find_first_not_of(whitespace, pos);
            pos = line.find_first_of(whitespace, pos);
            if (pos == std::string_view::npos) return 0;
        }
        pos = line.find_first_not_of(whitespace, pos);
        if (pos == std::string_view::npos) return 0;
        if (line[pos] == '+') pos++;
        int deltaN = 0;
        std::from_chars(line.data() + pos, line.data() + line.size(), deltaN);
        return deltaN;
    }

    std::pair<std::size_t, std::uint64_t> countParticlesAndSumDeltaN(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << filename << std::endl;
            return {0, 0};
        }

        // Report an empty file
        file.seekg(0, std::ios::end);
        if (file.tellg() == 0) {
            throw std::runtime_error("penEasy phase space file is empty: " + filename);
        }
        file.seekg(0, std::ios::beg);

        ByteBuffer buffer;
        std::size_t lineCount = 0;
        std::uint64_t totalDeltaN = 0;
        std::string currentLine;
        currentLine.reserve(256); // Reserve space for typical line length

        while (!file.eof() && file.peek() != std::ifstream::traits_type::eof()) {
            size_t bytesRead = buffer.setData(file);
            const char* data = reinterpret_cast<const char*>(buffer.data());
            const char* end = data + bytesRead;

            while (data < end) {
                const char* newline = static_cast<const char*>(std::memchr(data, '\n', end - data));
                if (!newline) {
                    currentLine.append(data, end); // partial line, completed by the next block
                    break;
                }
                currentLine.append(data, newline);
                data = newline + 1;
                lineCount++;

                // Skip header lines (first two lines)
                if (lineCount > 2 && !currentLine.empty()) {
                    totalDeltaN += parseDeltaN(currentLine);
                }
                currentLine.clear();
            }
        }

        // Handle last line if file doesn't end with newline
        file.clear(); // reset eof/fail flags from the final read so the seek below succeeds
        file.seekg(0, std::ios::end);
        if (file.tellg() > 0) {
            if (lineCount == 0) {
                lineCount = 1; // Single line file
            } else if (!currentLine.empty()) {
                lineCount++;
                // Process the last line for DeltaN if it's a data line
                if (lineCount > 2) {
                    totalDeltaN += parseDeltaN(currentLine);
                }
            }
        }

        return {lineCount > 2 ? lineCount - 2 : 0, totalDeltaN}; // Subtract 2 for header lines
    }


    Reader::Reader(const std::string & fileName, const UserOptions & options)
    : PhaseSpaceFileReader("penEasy", fileName, options, FormatType::ASCII)
    {
        auto [particleCount, totalDeltaN] = countParticlesAndSumDeltaN(fileName);
        numberOfParticles_ = particleCount;
        numberOfOriginalHistories_ = totalDeltaN;
    }

    Particle Reader::readASCIIParticle(const std::string & line)
    {
        // Parse the line to extract particle data
        int kpar;
        float e, x, y, z, u, v, w, weight;
        int dn;
        std::array<int,5> ilb;

        // Use std::istringstream to parse the line
        std::istringstream iss(line);
        if (!(iss >> kpar >> e >> x >> y >> z >> u >> v >> w >> weight >> dn 
                >> ilb[0] >> ilb[1] >> ilb[2] >> ilb[3] >> ilb[4]))
        {
            throw std::runtime_error("Failed to parse particle data from line: " + line);
        }

        // Create a new Particle object and set its properties
        ParticleType particleType;
        switch (kpar) {
            case 1:
                particleType = ParticleType::Electron;
                break;
            case 2:
                particleType = ParticleType::Photon;
                break;
            case 3:
                particleType = ParticleType::Positron;
                break;
            case 4:
                particleType = ParticleType::Proton;
                break;
            default:
                throw std::runtime_error("Unsupported particle type.");
        }

        float kineticEnergy = e * 1.e-6f; // convert to MeV

        bool isNewHistory = (dn >= 1);

        Particle particle(particleType, kineticEnergy, x, y, z, u, v, w, isNewHistory, weight);
        particle.reserveIntProperties(6); // Reserve space for 6 int properties

        particle.setIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER, dn);

        Penelope::ApplyILBArrayToParticle(particle, ilb);

        return particle;
    }


}