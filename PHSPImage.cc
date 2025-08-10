
#include <iostream>
#include <string>
#include <chrono>

#include "particlezoo/utilities/argParse.h"
#include "particlezoo/utilities/formats.h"
#include "particlezoo/utilities/pzimages.h"
#include "particlezoo/utilities/pzbitmap.h"
#include "particlezoo/utilities/pztiff.h"
#include "particlezoo/PhaseSpaceFileReader.h"
#include "particlezoo/PhaseSpaceFileWriter.h"

enum Plane {
    XY,
    XZ,
    YZ
};

enum ImageFormat {
    TIFF,
    BMP
};

int main(int argc, char* argv[]) {

    /*
     * This program reads a particle phase space file in any supported format and generates an image that represents the particle distribution.
     * Mandatory parameters are:
     * -input: the file to read from
     * 
     * By default, the format of both the input is determined by the file extension.
     * However, you can specify the format explicitly by using the --inputFormat and --outputFormat option.
     * 
     * The supported formats can be checked using the --formats option.
     * 
     * Example usages:
     * ./PHSPImage -plane XY ZVALUE inputfile.egsphsp output.bmp
     * ./PHSPImage --inputFormat egsphsp -plane XZ YVALUE inputfile.egsphsp output.bmp
     */

    using namespace ParticleZoo;

    FormatRegistry::RegisterStandardFormats();

    ImageFormat outputFormat = TIFF; // default output format

    std::string usageMessage = "Usage: PHSPImage [--<option> <value> ...] planeLocation inputfile outputfile";
    auto args = parseArgs(argc, argv, usageMessage, 3);

    // validate mandatory parameters
    std::string planeLocation_str = args["positionals"][0];
    float planeLocation;
    try {
        planeLocation = atof(planeLocation_str.c_str());
    } catch (const std::exception& e) {
        throw std::runtime_error("Invalid plane location: " + planeLocation_str);
    }
    std::string inputFile = args["positionals"][1];
    std::string outputFile = args["positionals"][2];
    if (inputFile.empty()) {
        throw std::runtime_error("No input file specified.");
    }
    if (outputFile.empty()) {
        throw std::runtime_error("No output file specified.");
    }
    if (inputFile == outputFile) {
        throw std::runtime_error("Input and output files must be different.");
    }

    Plane plane = XY; // default plane
    if (!args["plane"].empty()) {
        std::string planeStr = args["plane"][0];
        if (planeStr == "XY") {
            plane = XY;
        } else if (planeStr == "XZ") {
            plane = XZ;
        } else if (planeStr == "YZ") {
            plane = YZ;
        } else {
            throw std::runtime_error("Invalid plane specified. Use XY, XZ, or YZ.");
        }
    }

    // Set default dimensions and image size
    float minDim1 = -40 * cm;
    float maxDim1 = 40 * cm;
    float minDim2 = -40 * cm;
    float maxDim2 = 40 * cm;
    float widthDim3 = 0.25 * cm; // tolerance for the third dimension
    int imageWidth = 1024;
    int imageHeight = 1024;

    if (args["imageWidth"].size() == 1) {
        imageWidth = std::stoi(args["imageWidth"][0]);
        if (imageWidth <= 0) {
            throw std::runtime_error("Image width must be a positive integer.");
        }
    }

    if (args["imageHeight"].size() == 1) {
        imageHeight = std::stoi(args["imageHeight"][0]);
        if (imageHeight <= 0) {
            throw std::runtime_error("Image height must be a positive integer.");
        }
    }

    if (plane == XY && args["minX"].size() == 1) {
        minDim1 = atof(args["minX"][0].c_str()) * cm;
    }
    if (plane == XY && args["maxX"].size() == 1) {
        maxDim1 = atof(args["maxX"][0].c_str()) * cm;
    }
    if (plane == XY && args["minY"].size() == 1) {
        minDim2 = atof(args["minY"][0].c_str()) * cm;
    }
    if (plane == XY && args["maxY"].size() == 1) {
        maxDim2 = atof(args["maxY"][0].c_str()) * cm;
    }

    if (plane == XZ && args["minX"].size() == 1) {
        minDim1 = atof(args["minX"][0].c_str()) * cm;
    }
    if (plane == XZ && args["maxX"].size() == 1) {
        maxDim1 = atof(args["maxX"][0].c_str()) * cm;
    }
    if (plane == XZ && args["minZ"].size() == 1) {
        minDim2 = atof(args["minZ"][0].c_str()) * cm;
    }
    if (plane == XZ && args["maxZ"].size() == 1) {
        maxDim2 = atof(args["maxZ"][0].c_str()) * cm;
    }

    if (plane == YZ && args["minY"].size() == 1) {
        minDim1 = atof(args["minY"][0].c_str()) * cm;
    }
    if (plane == YZ && args["maxY"].size() == 1) {
        maxDim1 = atof(args["maxY"][0].c_str()) * cm;
    }
    if (plane == YZ && args["minZ"].size() == 1) {
        minDim2 = atof(args["minZ"][0].c_str()) * cm;
    }
    if (plane == YZ && args["maxZ"].size() == 1) {
        maxDim2 = atof(args["maxZ"][0].c_str()) * cm;
    }

    if (plane == XY && args["widthZ"].size() == 1) {
        widthDim3 = 0.5 * atof(args["widthZ"][0].c_str()) * cm;
    } else if (plane == XZ && args["widthY"].size() == 1) {
        widthDim3 = 0.5 * atof(args["widthY"][0].c_str()) * cm;
    } else if (plane == YZ && args["widthX"].size() == 1) {
        widthDim3 = 0.5 * atof(args["widthX"][0].c_str()) * cm;
    }

    if (minDim1 >= maxDim1 || minDim2 >= maxDim2) {
        throw std::runtime_error("Invalid dimensions specified. Ensure that min < max for both dimensions.");
    }

    bool energyWeighted = false;
    if (!args["energyWeighted"].empty()) {
        energyWeighted = (args["energyWeighted"][0] == "true");
    }

    std::string inputFormat = args["inputFormat"].empty() ? "" : args["inputFormat"][0];

    if (args["outputFormat"].size() == 1) {
        std::string formatStr = args["outputFormat"][0];
        if (formatStr == "tiff" || formatStr == "TIFF") {
            outputFormat = TIFF;
        } else if (formatStr == "bmp" || formatStr == "BMP") {
            outputFormat = BMP;
        } else {
            throw std::runtime_error("Unsupported output format: " + formatStr);
        }
    }

    // print out the parameters
    std::cout << "Parameters:" << std::endl;
    std::cout << "  Image Format: " << (outputFormat == TIFF ? "TIFF" : "BMP") << std::endl;
    std::cout << "  Plane: " << (plane == XY ? "XY" : (plane == XZ ? "XZ" : "YZ")) << std::endl;
    std::cout << "  Plane Location: " << planeLocation << " cm" << std::endl;
    std::cout << "  Input File: " << inputFile << " (Format: " << inputFormat << ")" << std::endl;
    std::cout << "  Output File: " << outputFile << std::endl; 
    std::cout << "  Image Width: " << imageWidth << " pixels" << std::endl;
    std::cout << "  Image Height: " << imageHeight << " pixels" << std::endl;
    std::cout << "  Dimensions: [" << minDim1 << ", " << maxDim1 << "] cm x [" 
              << minDim2 << ", " << maxDim2 << "] cm" << std::endl;
    std::cout << "  Width in third dimension: " << widthDim3 << " cm" << std::endl;
    std::cout << "  Energy Weighted: " << (energyWeighted ? "true" : "false") << std::endl;
    std::cout << "  Max Particles to Read: " 
              << (args["maxParticles"].empty() ? "all" : args["maxParticles"][0]) << std::endl;

    int errorCode = 0;

    std::unique_ptr<PhaseSpaceFileReader> reader;
    if (inputFormat.empty()) {
        reader = FormatRegistry::CreateReader(inputFile, args);
    } else {
        reader = FormatRegistry::CreateReader(inputFormat, inputFile, args);
    }

    try {
        std::cout << "Counting particles from " 
                  << inputFile << " (" << inputFormat << ") to store in image "
                  << outputFile << "..." << std::endl;

        int lastPercentageProgress = 0;
        std::cout << "[--------------------] 0% complete" << std::flush;

        uint64_t particlesInFile = reader->getNumberOfParticles();
        uint64_t maxParticles = args["maxParticles"].empty() ? particlesInFile : std::stoull(args["maxParticles"][0]);
        uint64_t particlesToRead = particlesInFile > maxParticles ? maxParticles : particlesInFile;
        
        uint64_t onePercentInterval = particlesToRead >= 100 
                                    ? particlesToRead / 100 
                                    : 1;

        uint64_t numberOfHistories = reader->getNumberOfOriginalHistories();
        if (numberOfHistories == 0 || particlesToRead == 0) {
            throw std::runtime_error("No particle histories found in the input file.");
        }

        auto start_time = std::chrono::steady_clock::now();

        Image<float> * image;
        if (outputFormat == TIFF) {
            image = new TiffImage<float>(imageWidth, imageHeight);
        } else if (outputFormat == BMP) {
            image = new BitmapImage<float>(imageWidth, imageHeight);
        } else {
            throw std::runtime_error("Unsupported output format.");
        }

        if (particlesToRead > 0) {
            for (uint64_t particlesSoFar = 1 ; reader->hasMoreParticles() && particlesSoFar <= particlesToRead ; particlesSoFar++) {

                Particle particle = reader->getNextParticle();

                // Update progress bar every 1% of particles read
                if (particlesSoFar % onePercentInterval == 0) {
                    int progress = static_cast<int>(particlesSoFar * 20 / particlesToRead);
                    int percentageProgress = static_cast<int>(particlesSoFar * 100 / particlesToRead);
                    if (percentageProgress != lastPercentageProgress) {
                        lastPercentageProgress = percentageProgress;
                        std::cout << "\r[";
                        std::cout << std::string(progress, '#') << std::string(20 - progress, '-') << "] "
                            << percentageProgress << "% complete" << std::flush;
                    }
                }

                /* Process particle */
                float x = particle.getX() / cm;
                float y = particle.getY() / cm;
                float z = particle.getZ() / cm;

                // Determine pixel coordinates based on the selected plane
                int pixelX = 0, pixelY = 0;
                bool validPixel = false;
                if (plane == XY && std::abs(z - planeLocation) < widthDim3 && x >= minDim1 && x <= maxDim1 && y >= minDim2 && y <= maxDim2) {
                    pixelX = static_cast<int>((x - minDim1) / (maxDim1 - minDim1) * imageWidth);
                    pixelY = static_cast<int>((y - minDim2) / (maxDim2 - minDim2) * imageHeight);
                    validPixel = true;
                } else if (plane == XZ && std::abs(y - planeLocation) < widthDim3 && x >= minDim1 && x <= maxDim1 && z >= minDim2 && z <= maxDim2) {
                    pixelX = static_cast<int>((x - minDim1) / (maxDim1 - minDim1) * imageWidth);
                    pixelY = static_cast<int>((z - minDim2) / (maxDim2 - minDim2) * imageHeight);
                    validPixel = true;
                } else if (plane == YZ && std::abs(x - planeLocation) < widthDim3 && y >= minDim1 && y <= maxDim1 && z >= minDim2 && z <= maxDim2) {
                    pixelX = static_cast<int>((y - minDim1) / (maxDim1 - minDim1) * imageWidth);
                    pixelY = static_cast<int>((z - minDim2) / (maxDim2 - minDim2) * imageHeight);
                    validPixel = true;
                }
                
                // Check if pixel coordinates are valid
                validPixel = validPixel && (pixelX >= 0 && pixelX < imageWidth && pixelY >= 0 && pixelY < imageHeight);               
                if (!validPixel) {
                    continue; // Skip particles that do not fall within the specified plane and dimensions
                }

                float weight = particle.getWeight();
                if (energyWeighted) {
                    weight *= particle.getKineticEnergy();
                }

                // Set pixel color based on the particle's weight
                Pixel<float> pixel = image->getPixel(pixelX, pixelY);
                pixel.b += weight;
                pixel.g += weight;
                pixel.r += weight;
                image->setPixel(pixelX, pixelY, pixel);
            }
        }

        std::uint64_t historiesRead = particlesToRead < particlesInFile ? reader->getHistoriesRead() : numberOfHistories;

        image->normalize(static_cast<float>(historiesRead));

        image->save(outputFile);

        auto end_time = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(end_time - start_time).count();

        delete image;

        std::cout << "\r[####################] 100% complete" << std::endl;
        std::cout << "Time taken: " << elapsed << " seconds" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error occurred: " << e.what() << std::endl;
        errorCode = 1;
    }

    // Ensure that the reader and writer are closed even if an exception occurs
    if (reader) reader->close();

    return errorCode;
}