#include "particlezoo/operations/GenerateImage.h"

#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <array>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

#include "particlezoo/utilities/formats.h"
#include "particlezoo/utilities/pzimages.h"
#include "particlezoo/utilities/pzbitmap.h"
#include "particlezoo/utilities/pztiff.h"
#include "particlezoo/utilities/progress.h"
#include "particlezoo/utilities/units.h"
#include "particlezoo/PhaseSpaceFileReader.h"
#include "particlezoo/egs/EGSLATCH.h"

#include "GenerationFilter.h"

namespace ParticleZoo {

namespace {

struct InternalImageConfig {
    const ImagePlane          plane;
    const std::array<float,4> dimensionLimits; // [min1, max1, min2, max2]
    const std::string         inputFormat;
    const ImageOutputFormat   outputFormat;
    const uint32_t            maxParticles;
    const bool                normalizeByParticles;
    const bool                printDetails;
    const ImageProjectionType projectionType;
    const ImageQuantityType   quantityType;
    const GenerationFilter    generationFilter;
    const float               tolerance;
    const int                 imageWidth;
    const int                 imageHeight;
    const float               planeLocation;
    const bool                errorOnWarning;
    const bool                useLATCHFilter;
    const uint32_t            LATCHFilter;

    float minDim1() const { return dimensionLimits[0]; }
    float maxDim1() const { return dimensionLimits[1]; }
    float minDim2() const { return dimensionLimits[2]; }
    float maxDim2() const { return dimensionLimits[3]; }

    explicit InternalImageConfig(const GenerateImageOptions& opts)
        : plane(opts.plane),
          dimensionLimits(computeDimensionLimits(opts)),
          inputFormat(opts.inputFormat),
          outputFormat(opts.outputFormat),
          maxParticles(static_cast<uint32_t>(std::min(opts.maxParticles,
                           static_cast<uint64_t>(std::numeric_limits<uint32_t>::max())))),
          normalizeByParticles(opts.normalizeByParticles),
          printDetails(opts.showDetails),
          projectionType(opts.projectTo.has_value() ? ImageProjectionType::PROJECT : opts.projectionType),
          quantityType(opts.energyWeighted ? ImageQuantityType::ENERGY : opts.score),
          generationFilter(GenerationFilter::Resolve(opts.primariesOnly, opts.excludePrimaries, opts.generations)),
          tolerance(projectionType == ImageProjectionType::NONE ? opts.tolerance : 0.0f),
          imageWidth(opts.imageWidth),
          imageHeight(opts.imageHeight),
          planeLocation(opts.projectTo.has_value() ? opts.projectTo.value() : opts.planeLocation),
          errorOnWarning(opts.errorOnWarning),
          useLATCHFilter(opts.useLATCHFilter),
          LATCHFilter(opts.LATCHFilter)
    {
        validate();
    }

    void details(const std::string& detectedFormat = "") const {
        std::stringstream ss;
        ss << "Parameters:\n";
        ss << "  Image Format: " << (outputFormat == ImageOutputFormat::TIFF ? "TIFF" : "BMP") << "\n";
        ss << "  Plane: " << (plane == ImagePlane::XY ? "XY" : (plane == ImagePlane::XZ ? "XZ" : "YZ")) << "\n";
        if (projectionType != ImageProjectionType::FLATTEN)
            ss << "  Plane Location: " << planeLocation / cm << " cm\n";
        ss << "  Projection Scheme: " << (projectionType == ImageProjectionType::PROJECT ? "Projection"
                                         : projectionType == ImageProjectionType::FLATTEN ? "Flatten"
                                         : "None") << "\n";
        if (!detectedFormat.empty())
            ss << "  Input Format: " << detectedFormat << " (auto-detected)\n";
        else if (!inputFormat.empty())
            ss << "  Input Format: " << inputFormat << " (forced)\n";
        else
            ss << "  Input Format: auto\n";
        ss << "  Image Width: "  << imageWidth  << " pixels\n";
        ss << "  Image Height: " << imageHeight << " pixels\n";
        ss << "  Dimensions: [" << minDim1()/cm << ", " << maxDim1()/cm << "] cm x ["
                                << minDim2()/cm << ", " << maxDim2()/cm << "] cm\n";
        if (projectionType == ImageProjectionType::NONE)
            ss << "  Thickness in third dimension: " << tolerance/cm << " cm\n";
        ss << "  Quantity scored: "
           << (quantityType == ImageQuantityType::COUNT  ? "Particle Fluence"
              : quantityType == ImageQuantityType::ENERGY ? "Energy Fluence"
              : quantityType == ImageQuantityType::X_DIR  ? "X Directional Cosine"
              : quantityType == ImageQuantityType::Y_DIR  ? "Y Directional Cosine"
              : "Z Directional Cosine") << "\n";
        bool primariesOnly = generationFilter.useFilter &&
                             generationFilter.minimumGeneration == 1 &&
                             generationFilter.maximumGeneration == 1;
        ss << "  Generation Filter: "
           << (!generationFilter.useFilter ? "Off"
              : primariesOnly ? "On (Primaries only)"
              : "On") << "\n";
        if (generationFilter.useFilter && !primariesOnly) {
            ss << "    Minimum Generation: " << generationFilter.minimumGeneration << "\n";
            ss << "    Maximum Generation: " << generationFilter.maximumGeneration << "\n";
        }
        ss << "  Max Particles to Read: " << (maxParticles == GenerateImageOptions::DEFAULT_MAX_PARTICLES ? "all" : std::to_string(maxParticles)) << "\n";
        ss << "  Normalization: by " << (normalizeByParticles ? "particles" : "histories") << "\n";
        ss << "  Error on warnings: " << (errorOnWarning ? "true" : "false") << "\n";
        if (useLATCHFilter)
            ss << "  EGS LATCH Filter: 0x" << std::hex << LATCHFilter << std::dec << "\n";
        std::cout << ss.str() << std::flush;
    }

private:
    static std::array<float,4> computeDimensionLimits(const GenerateImageOptions& opts) {
        float min1 = -GenerateImageOptions::DEFAULT_DISTANCE, max1 = GenerateImageOptions::DEFAULT_DISTANCE;
        float min2 = -GenerateImageOptions::DEFAULT_DISTANCE, max2 = GenerateImageOptions::DEFAULT_DISTANCE;

        if (opts.square.has_value()) {
            float halfSide = opts.square.value() / 2.0f;
            min1 = min2 = -halfSide;
            max1 = max2 =  halfSide;
        }

        switch (opts.plane) {
            case ImagePlane::XY:
                if (opts.minX) min1 = opts.minX.value();
                if (opts.maxX) max1 = opts.maxX.value();
                if (opts.minY) min2 = opts.minY.value();
                if (opts.maxY) max2 = opts.maxY.value();
                break;
            case ImagePlane::XZ:
                if (opts.minX) min1 = opts.minX.value();
                if (opts.maxX) max1 = opts.maxX.value();
                if (opts.minZ) min2 = opts.minZ.value();
                if (opts.maxZ) max2 = opts.maxZ.value();
                break;
            case ImagePlane::YZ:
                if (opts.minY) min1 = opts.minY.value();
                if (opts.maxY) max1 = opts.maxY.value();
                if (opts.minZ) min2 = opts.minZ.value();
                if (opts.maxZ) max2 = opts.maxZ.value();
                break;
        }
        return {min1, max1, min2, max2};
    }

    void validate() const {
        if (minDim1() >= maxDim1()) throw std::runtime_error("Invalid dimensions specified. Ensure that min < max for both dimensions.");
        if (minDim2() >= maxDim2()) throw std::runtime_error("Invalid dimensions specified. Ensure that min < max for both dimensions.");
        if (tolerance < 0)          throw std::runtime_error("Tolerance cannot be a negative number.");
        if (imageWidth  <= 0)       throw std::runtime_error("Image width must be a positive integer.");
        if (imageHeight <= 0)       throw std::runtime_error("Image height must be a positive integer.");
        // Generation-filter conflicts and range are validated by GenerationFilter::Resolve at construction
    }
};

} // anonymous namespace


void GenerateImage(const std::string& inputFile,
                   const std::string& outputFile,
                   const GenerateImageOptions& options)
{
    if (inputFile.empty())  throw std::runtime_error("No input file specified.");
    if (outputFile.empty()) throw std::runtime_error("No output file specified.");
    if (inputFile == outputFile) throw std::runtime_error("Input and output files must be different.");

    constexpr uint64_t MAX_PERCENTAGE = 100;

    const InternalImageConfig config(options);

    std::unique_ptr<PhaseSpaceFileReader> reader;
    reader = FormatRegistry::CreateReader(config.inputFormat, inputFile, options.formatOptions);

    std::vector<std::string> errorMessages;
    std::vector<std::string> warningMessages;

    if (config.printDetails) {
        config.details(reader->getPHSPFormat());
    }

    try {
        std::cout << "Counting particles from "
                  << inputFile << " (" << reader->getPHSPFormat() << ") to store in image "
                  << outputFile << "..." << std::endl;

        uint64_t particlesInFile = reader->getNumberOfParticles();
        uint64_t particlesToRead = particlesInFile > static_cast<uint64_t>(config.maxParticles)
                                  ? static_cast<uint64_t>(config.maxParticles)
                                  : particlesInFile;

        uint64_t onePercentInterval = particlesToRead >= MAX_PERCENTAGE
                                    ? particlesToRead / MAX_PERCENTAGE
                                    : 1;

        if (particlesToRead == 0)
            throw std::runtime_error("No particles found in the input file.");

        float xPixelsPerUnitLength = static_cast<float>(config.imageWidth)  / (config.maxDim1() - config.minDim1());
        float yPixelsPerUnitLength = static_cast<float>(config.imageHeight) / (config.maxDim2() - config.minDim2());
        // Image origin as a length; TiffImage::save() converts it to centimeters
        float xOffset    = static_cast<float>(config.minDim1());
        float yOffset    = static_cast<float>(config.minDim2());
        float pixelArea  = (config.maxDim1() - config.minDim1()) * (config.maxDim2() - config.minDim2())
                         / (static_cast<float>(config.imageWidth) * static_cast<float>(config.imageHeight));

        auto start_time = std::chrono::steady_clock::now();

        std::unique_ptr<Image<float>> image;
        if (config.outputFormat == ImageOutputFormat::TIFF) {
            image = std::make_unique<TiffImage<float>>(config.imageWidth, config.imageHeight, xPixelsPerUnitLength, yPixelsPerUnitLength, xOffset, yOffset);
        } else if (config.outputFormat == ImageOutputFormat::BMP) {
            image = std::make_unique<BitmapImage<float>>(config.imageWidth, config.imageHeight);
        } else {
            throw std::runtime_error("Unsupported output format.");
        }

        Progress<uint64_t> progress(particlesToRead);
        progress.Start("Reading particles:");

        while (reader->hasMoreParticles() && reader->getParticlesRead() < particlesToRead) {
            Particle particle = reader->getNextParticle();

            if (particle.getType() == ParticleType::Unsupported)
                throw std::runtime_error("Encountered unsupported particle type in the input file.");

            if (particle.getType() == ParticleType::PseudoParticle) continue;

            switch (config.projectionType) {
                case ImageProjectionType::FLATTEN:
                    switch (config.plane) {
                        case ImagePlane::XY: particle.setZ(config.planeLocation); break;
                        case ImagePlane::XZ: particle.setY(config.planeLocation); break;
                        case ImagePlane::YZ: particle.setX(config.planeLocation); break;
                    }
                    break;
                case ImageProjectionType::PROJECT:
                    switch (config.plane) {
                        case ImagePlane::XY: particle.projectToZValue(config.planeLocation); break;
                        case ImagePlane::XZ: particle.projectToYValue(config.planeLocation); break;
                        case ImagePlane::YZ: particle.projectToXValue(config.planeLocation); break;
                    }
                    break;
                default:
                    break;
            }

            float x = particle.getX();
            float y = particle.getY();
            float z = particle.getZ();

            int pixelX = 0, pixelY = 0;
            bool validPixel = false;

            if (config.plane == ImagePlane::XY &&
                std::abs(z - config.planeLocation) <= config.tolerance &&
                x >= config.minDim1() && x <= config.maxDim1() &&
                y >= config.minDim2() && y <= config.maxDim2()) {
                pixelX = static_cast<int>((x - config.minDim1()) / (config.maxDim1() - config.minDim1()) * config.imageWidth);
                pixelY = static_cast<int>((y - config.minDim2()) / (config.maxDim2() - config.minDim2()) * config.imageHeight);
                validPixel = true;
            } else if (config.plane == ImagePlane::XZ &&
                       std::abs(y - config.planeLocation) <= config.tolerance &&
                       x >= config.minDim1() && x <= config.maxDim1() &&
                       z >= config.minDim2() && z <= config.maxDim2()) {
                pixelX = static_cast<int>((x - config.minDim1()) / (config.maxDim1() - config.minDim1()) * config.imageWidth);
                pixelY = static_cast<int>((z - config.minDim2()) / (config.maxDim2() - config.minDim2()) * config.imageHeight);
                validPixel = true;
            } else if (config.plane == ImagePlane::YZ &&
                       std::abs(x - config.planeLocation) <= config.tolerance &&
                       y >= config.minDim1() && y <= config.maxDim1() &&
                       z >= config.minDim2() && z <= config.maxDim2()) {
                pixelX = static_cast<int>((y - config.minDim1()) / (config.maxDim1() - config.minDim1()) * config.imageWidth);
                pixelY = static_cast<int>((z - config.minDim2()) / (config.maxDim2() - config.minDim2()) * config.imageHeight);
                validPixel = true;
            }

            validPixel = validPixel && (pixelX >= 0 && pixelX < config.imageWidth &&
                                        pixelY >= 0 && pixelY < config.imageHeight);

            if (validPixel && config.generationFilter.useFilter) {
                if (particle.hasIntProperty(IntPropertyType::GENERATION)) {
                    const int generation = particle.getIntProperty(IntPropertyType::GENERATION);
                    validPixel = generation >= config.generationFilter.minimumGeneration &&
                                 generation <= config.generationFilter.maximumGeneration;
                } else {
                    throw std::runtime_error("Could not determine particle generation (primary/secondary) from the phase space file.");
                }
            }

            if (validPixel && config.useLATCHFilter) {
                validPixel = EGSphspFile::DoesParticlePassLATCHFilter(particle, config.LATCHFilter);
            }

            if (validPixel) {
                float weight = particle.getWeight();
                switch (config.quantityType) {
                    case ImageQuantityType::ENERGY:
                        weight *= particle.getKineticEnergy() / MeV;
                        break;
                    case ImageQuantityType::X_DIR:
                        weight *= particle.getDirectionalCosineX();
                        break;
                    case ImageQuantityType::Y_DIR:
                        weight *= particle.getDirectionalCosineY();
                        break;
                    case ImageQuantityType::Z_DIR:
                        weight *= particle.getDirectionalCosineZ();
                        break;
                    case ImageQuantityType::COUNT:
                    default:
                        break;
                }

                // Fluence is scored per cm^2, so convert the pixel area to cm^2 here
                float weightPerUnitArea = weight / (pixelArea / cm2);
                float pixelValue = image->getGrayscaleValue(pixelX, pixelY) + weightPerUnitArea;
                image->setGrayscaleValue(pixelX, pixelY, pixelValue);
            }

            uint64_t particlesSoFar = reader->getParticlesRead();
            if (particlesSoFar % onePercentInterval == 0) {
                progress.Update(particlesSoFar, "Processed " + std::to_string(reader->getHistoriesRead()) + " histories.");
            }
        }

        uint64_t numberOfHistories = reader->getNumberOfOriginalHistories();
        uint64_t particlesRead     = reader->getParticlesRead();
        uint64_t historiesRead     = particlesRead < particlesInFile ? reader->getHistoriesRead() : numberOfHistories;

        if (config.normalizeByParticles) {
            image->normalize(static_cast<float>(particlesRead));
        } else {
            image->normalize(static_cast<float>(historiesRead));
        }

        image->save(outputFile);
        progress.Complete("Image generation complete. Processed " + std::to_string(historiesRead) + " histories.");

        if (config.normalizeByParticles) {
            std::cout << "Image normalized by particles (" << particlesRead << " particles read)." << std::endl;
        } else {
            std::cout << "Image normalized by histories (" << historiesRead << " histories read)." << std::endl;
        }

        auto end_time = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(end_time - start_time).count();
        std::cout << "Time taken: " << elapsed << " seconds" << std::endl;

    } catch (const std::exception& e) {
        errorMessages.push_back(std::string(e.what()));
    }

    try { if (reader) reader->close(); } catch (const std::exception& e) { errorMessages.push_back("Error closing reader: " + std::string(e.what())); }

    std::cout << std::endl;

    for (const auto& warning : warningMessages)
        std::cerr << "Warning: " << warning << std::endl;

    if (!errorMessages.empty()) {
        for (const auto& error : errorMessages)
            std::cerr << "Error: " << error << std::endl;
        throw std::runtime_error(errorMessages[0]);
    }

    if (options.errorOnWarning && !warningMessages.empty()) {
        throw std::runtime_error("Warnings occurred during image generation (errorOnWarning is set): " + warningMessages[0]);
    }
}

} // namespace ParticleZoo
