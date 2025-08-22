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
        float u = particle.getPx();
        float v = particle.getPy();
        float w = particle.getPz();
        float weight = particle.getWeight();

        int dn;
        if (particle.hasIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER)) {
            dn = (int) particle.getIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER);
        } else {
            dn = (particle.isNewHistory()) ? 1 : 0;
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



    std::size_t countLinesInAsciiFile(const std::string& filename) {
        // Open the file in binary mode for efficient byte-level reading
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            // Return 0 and print an error message if the file cannot be opened
            std::cerr << "Error: Could not open file " << filename << std::endl;
            return 0;
        }

        // Create an instance of ByteBuffer
        ByteBuffer buffer;
        size_t lineCount = 0;

        // Read the file in chunks until the end of the file is reached
        while (!file.eof()) {
            // Read data from the file into the ByteBuffer
            size_t bytesRead = buffer.setData(file);

            // Iterate over the data in the buffer to count newlines
            // The buffer's data() method gives us a pointer to the raw data
            const byte* data = buffer.data();
            for (size_t i = 0; i < bytesRead; ++i) {
                if (data[i] == '\n') {
                    lineCount++;
                }
            }
        }

        // A final check to handle the case where the file is not empty but doesn't
        // end with a newline character. If the file is not empty and the last
        // character read was not a newline, we count the last line.
        file.seekg(0, std::ios::end);
        if (file.tellg() > 0 && lineCount == 0) {
            lineCount = 1; // It's a single-line file with no newline.
        } else if (file.tellg() > 0 && file.tellg() > buffer.length() && buffer.data()[buffer.length() - 1] != '\n') {
            // This case is for large files. If the last chunk of data read
            // doesn't end with a newline, we add one more to the count.
            file.seekg(-1, std::ios::end);
            char lastChar;
            file.read(&lastChar, 1);
            if (lastChar != '\n') {
                lineCount++;
            }
        }
        return lineCount;
    }


    Reader::Reader(const std::string & fileName, const UserOptions & options)
    : PhaseSpaceFileReader("penEasy", fileName, options, FormatType::ASCII),
      numberOfParticles_(0),
      numberOfParticlesRead_(0),
      numberOfOriginalHistoriesRead_(0)
    {
        std::size_t linesInASCIIFile = countLinesInAsciiFile(fileName);
        numberOfParticles_ = (linesInASCIIFile > 2) ? linesInASCIIFile - 2 : 0; // subtract 2 for the header
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

        if (isNewHistory) {
            numberOfOriginalHistoriesRead_+=dn;
        }
        numberOfParticlesRead_++;
        
        return particle;
    }

    std::uint64_t Reader::getNumberOfParticles() const {
        return numberOfParticles_;
    }


}