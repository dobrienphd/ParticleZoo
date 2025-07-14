#include "particlezoo/peneasy/penEasyphspFile.h"

#include <format>
#include <stdexcept>
#include <cstddef>
#include <cstdint>
#include <sstream>

#include "particlezoo/Particle.h"
#include "particlezoo/ByteBuffer.h"

namespace ParticleZoo::penEasyphspFile
{

    Writer::Writer(const std::string & fileName, const UserOptions & options)
    : PhaseSpaceFileWriter(fileName, options, FormatType::ASCII)
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


    Reader::Reader(const std::string & fileName, const UserOptions & options)
    : PhaseSpaceFileReader(fileName, options, FormatType::ASCII),
      numberOfParticlesRead_(0),
      numberOfOriginalHistoriesRead_(0)
    {}

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

        bool isNewHistory = (dn == 1);

        Particle particle(particleType, kineticEnergy, x, y, z, u, v, w, isNewHistory, weight);
        particle.reserveIntProperties(6); // Reserve space for 6 int properties

        particle.setIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER, dn);

        for (int i = 0; i < 5; i++) {
            if (ilb[i] != 0) {
                particle.setIntProperty(IntPropertyType(PROPERTY_PENELOPE_ILB[i]), ilb[i]);
            }
        }

        if (isNewHistory) {
            numberOfOriginalHistoriesRead_++;
        }
        numberOfParticlesRead_++;
        
        return particle;
    }

}