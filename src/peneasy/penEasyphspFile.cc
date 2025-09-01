#include "particlezoo/peneasy/penEasyphspFile.h"

#include <format>
#include <stdexcept>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <iostream>
#include <fstream>
#include <string>

#include "particlezoo/Particle.h"
#include "particlezoo/ByteBuffer.h"

namespace ParticleZoo::penEasyphspFile
{

    Writer::Writer(const std::string & fileName, const UserOptions & options)
    : PhaseSpaceFileWriter("penEasy", fileName, options, FormatType::ASCII)
    {}

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

        int ilb[5];
        for (int i = 0; i < 5; i++) {
            if (particle.hasIntProperty(PROPERTY_PENELOPE_ILB[i])) {
                ilb[i] = (int) particle.getIntProperty(PROPERTY_PENELOPE_ILB[i]);
            } else {
                ilb[i] = 0; // Default value if property is not set
            }
        }

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



    std::pair<std::size_t, std::uint64_t> countParticlesAndSumDeltaN(const std::string& filename) {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << filename << std::endl;
            return {0, 0};
        }

        ByteBuffer buffer;
        std::size_t lineCount = 0;
        std::uint64_t totalDeltaN = 0;
        std::string currentLine;
        currentLine.reserve(256); // Reserve space for typical line length

        while (!file.eof()) {
            size_t bytesRead = buffer.setData(file);
            const byte* data = buffer.data();
            
            for (size_t i = 0; i < bytesRead; ++i) {
                char ch = static_cast<char>(data[i]);
                
                if (ch == '\n') {
                    lineCount++;
                    
                    // Skip header lines (first two lines)
                    if (lineCount > 2 && !currentLine.empty()) {
                        // Parse DeltaN from the line (10th field)
                        std::istringstream iss(currentLine);
                        std::string field;
                        int fieldCount = 0;
                        
                        // Skip first 9 fields, then read DeltaN
                        while (fieldCount < 10 && iss >> field) {
                            fieldCount++;
                            if (fieldCount == 10) {
                                try {
                                    int deltaN = std::stoi(field);
                                    totalDeltaN += deltaN;
                                } catch (const std::exception&) {
                                    // If parsing fails, skip this line
                                }
                                break;
                            }
                        }
                    }
                    currentLine.clear();
                } else {
                    currentLine += ch;
                }
            }
        }

        // Handle last line if file doesn't end with newline
        file.seekg(0, std::ios::end);
        if (file.tellg() > 0) {
            if (lineCount == 0) {
                lineCount = 1; // Single line file
            } else if (!currentLine.empty()) {
                lineCount++;
                // Process the last line for DeltaN if it's a data line
                if (lineCount > 2) {
                    std::istringstream iss(currentLine);
                    std::string field;
                    int fieldCount = 0;
                    
                    while (fieldCount < 10 && iss >> field) {
                        fieldCount++;
                        if (fieldCount == 10) {
                            try {
                                int deltaN = std::stoi(field);
                                totalDeltaN += deltaN;
                            } catch (const std::exception&) {
                                // If parsing fails, skip this line
                            }
                            break;
                        }
                    }
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
        int ilb[5];

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

        for (int i = 0; i < 5; i++) {
            if (ilb[i] != 0) {
                particle.setIntProperty(IntPropertyType(PROPERTY_PENELOPE_ILB[i]), ilb[i]);
            }
        }

        return particle;
    }


}