
/*
 * PHSPImage - Particle Phase Space File to Image Converter
 * 
 * PURPOSE:
 * This application reads particle phase space files and generates 2D images that represent
 * the particle fluence (or energy fluence) distribution of particles projected onto a specified plane.
 * The tool is useful for visualizing particle beam profiles, energy distributions, and spatial patterns from
 * Monte Carlo simulation outputs.
 * 
 * SUPPORTED INPUT FORMATS:
 * - IAEA: International Atomic Energy Agency phase space format (.IAEAphsp)
 * - EGS: EGSnrc phase space format (.egsphsp, supports MODE0 and MODE2)
 * - TOPAS: TOPAS phase space format (.phsp, Binary/ASCII/Limited variants)
 * - penEasy: penEasy ASCII phase space format (.dat)
 * - ROOT: ROOT phase space format (.root) - if compiled with ROOT support
 * 
 * SUPPORTED OUTPUT FORMATS:
 * - TIFF: Tagged Image File Format (.tiff) - default, stores raw fluence data directly in 32-bit floating point precision
 * - BMP: Bitmap Image Format (.bmp) - basic raster format with automatic window-leveling performed to provide good contrast
 * 
 * COMMAND LINE OPTIONS:
 * Required Arguments:
 *   planeLocation             Location of the imaging plane in cm (e.g., 0.0 for Z=0 plane)
 *   inputfile                 Input phase space file
 *   outputfile                Output image file path
 * 
 * Optional Arguments:
 *   --plane <XY|XZ|YZ>        Imaging plane orientation (default: XY)
 *                             XY: view from Z-axis, XZ: view from Y-axis, YZ: view from X-axis
 *   
 *   Image Dimensions:
 *   --imageWidth <pixels>     Output image width in pixels (default: 1024)
 *   --imageHeight <pixels>    Output image height in pixels (default: 1024)
 *   
 *   Spatial Boundaries (in cm):
 *   --minX <value>            Minimum X coordinate for imaging region (default: -40 cm)
 *   --maxX <value>            Maximum X coordinate for imaging region (default: 40 cm)
 *   --minY <value>            Minimum Y coordinate for imaging region (default: -40 cm)
 *   --maxY <value>            Maximum Y coordinate for imaging region (default: 40 cm)
 *   --minZ <value>            Minimum Z coordinate for imaging region (default: -40 cm)
 *   --maxZ <value>            Maximum Z coordinate for imaging region (default: 40 cm)
 *   
 *   Plane Thickness:
 *   --widthX <value>          Half-width tolerance in X direction (cm) for YZ plane (default: 0.25 cm)
 *   --widthY <value>          Half-width tolerance in Y direction (cm) for XZ plane (default: 0.25 cm)
 *   --widthZ <value>          Half-width tolerance in Z direction (cm) for XY plane (default: 0.25 cm)
 *   
 *   Processing Options:
 *   --maxParticles <N>             Limit the maximum number of particles to process (default: all)
 *   --energyWeighted <true|false>  Set to true to produce energy fluence instead of particle fluence (default: false)
 *   --inputFormat <format>         Force input file format (default: auto-detect from extension)
 *   --outputFormat <tiff|bmp>      Force output image format (default: tiff)
 *   --formats                      Display supported input file formats and exit
 *   --help                         Display usage information and exit
 *
 * USAGE EXAMPLES:
 *   # Generate XY plane image at Z=0 with default settings
 *   PHSPImage 0.0 beam.egsphsp output.tiff
 * 
 *   # Create XZ plane view at Y=5cm with custom dimensions
 *   PHSPImage --plane XZ --minX -10 --maxX 10 --minZ -5 --maxZ 15 5.0 beam.IAEAphsp profile.tiff
 * 
 *   # Energy-weighted image with 1000x1000 resolution
 *   PHSPImage --energyWeighted true --imageWidth 1000 --imageHeight 1000 0.0 dose.phsp dose_map.bmp
 * 
 *   # Process only first 100,000 particles with thick imaging plane
 *   PHSPImage --maxParticles 100000 --widthZ 1.0 0.0 simulation.root beam_profile.tiff
 * 
 * BEHAVIOR:
 * - Particles are projected onto the specified 2D plane within the tolerance thickness
 * - Image pixel values represent particle fluence (particles/cm²) or energy fluence (MeV/cm²)
 * - Images are normalized by the total number of histories processed
 * - Progress is displayed during particle processing
 * - Particles outside the specified spatial boundaries are ignored
 * - Output images use grayscale values proportional to particle density
 * - TIFF format includes spatial calibration metadata for analysis tools
 */

#include <iostream>
#include <string>
#include <chrono>

#include "particlezoo/utilities/argParse.h"
#include "particlezoo/utilities/formats.h"
#include "particlezoo/utilities/pzimages.h"
#include "particlezoo/utilities/pzbitmap.h"
#include "particlezoo/utilities/pztiff.h"
#include "particlezoo/utilities/progress.h"
#include "particlezoo/PhaseSpaceFileReader.h"
#include "particlezoo/PhaseSpaceFileWriter.h"

// Enum for the imaging plane
enum Plane {
    XY,
    XZ,
    YZ
};

// Enum for the image format
enum ImageFormat {
    TIFF,
    BMP
};

int main(int argc, char* argv[]) {

    // Initial setup
    using namespace ParticleZoo;
    int errorCode = 0;
    FormatRegistry::RegisterStandardFormats();
    ImageFormat outputFormat = TIFF; // default output format
    bool energyWeighted = false; // default to particle fluence instead of energy fluence
    Plane plane = XY; // default plane
    float planeLocation;

    // Set default dimensions and image size
    float minDim1 = -40 * cm;
    float maxDim1 = 40 * cm;
    float minDim2 = -40 * cm;
    float maxDim2 = 40 * cm;
    float widthDim3 = 0.25 * cm; // tolerance for the third dimension
    int imageWidth = 1024;
    int imageHeight = 1024;

    // Define usage message and parse command line arguments
    std::string usageMessage = "Usage: PHSPImage [OPTIONS] <planeLocation> <inputfile> <outputfile>\n"
                                "\n"
                                "Convert particle phase space files to 2D images of the fluence distributions.\n"
                                "\n"
                                "Required Arguments:\n"
                                "  <planeLocation>           Location of imaging plane in cm (e.g., 0.0 for Z=0)\n"
                                "  <inputfile>               Input phase space file to visualize\n"
                                "  <outputfile>              Output image file path\n"
                                "\n"
                                "Optional Arguments:\n"
                                "  --plane <XY|XZ|YZ>             Imaging plane orientation (default: XY)\n"
                                "  --imageWidth <pixels>          Image width in pixels (default: 1024)\n"
                                "  --imageHeight <pixels>         Image height in pixels (default: 1024)\n"
                                "  --minX <cm>                    Minimum X coordinate (default: -40 cm)\n"
                                "  --maxX <cm>                    Maximum X coordinate (default: 40 cm)\n"
                                "  --minY <cm>                    Minimum Y coordinate (default: -40 cm)\n"
                                "  --maxY <cm>                    Maximum Y coordinate (default: 40 cm)\n"
                                "  --minZ <cm>                    Minimum Z coordinate (default: -40 cm)\n"
                                "  --maxZ <cm>                    Maximum Z coordinate (default: 40 cm)\n"
                                "  --widthX <cm>                  Half-width tolerance for YZ plane\n"
                                "  --widthY <cm>                  Half-width tolerance for XZ plane\n"
                                "  --widthZ <cm>                  Half-width tolerance for XY plane (default: 0.25 cm)\n"
                                "  --maxParticles <N>             Maximum particles to process (default: all)\n"
                                "  --energyWeighted <true|false>  Set to true to produce energy fluence instead of particle fluence (default: false)\n"
                                "  --inputFormat <format>         Force input format (default: auto-detect)\n"
                                "  --outputFormat <tiff|bmp>      Output image format (default: tiff)\n"
                                "  --formats                      List supported input formats\n"
                                "\n"
                                "Examples:\n"
                                "  PHSPImage 0.0 beam.egsphsp output.tiff\n"
                                "  PHSPImage --plane XZ --minX -10 --maxX 10 5.0 beam.IAEAphsp profile.tiff\n"
                                "  PHSPImage --energyWeighted true --imageWidth 2048 0.0 dose.phsp dose_map.bmp";
    auto args = parseArgs(argc, argv, usageMessage, 3);

    // Validate parameters
    std::string planeLocation_str = args["positionals"][0];
    std::string inputFile = args["positionals"][1];
    std::string outputFile = args["positionals"][2];
    std::string inputFormat = args["inputFormat"].empty() ? "" : args["inputFormat"][0];
    if (inputFile.empty()) throw std::runtime_error("No input file specified.");
    if (outputFile.empty()) throw std::runtime_error("No output file specified.");
    if (inputFile == outputFile) throw std::runtime_error("Input and output files must be different.");
    try {
        planeLocation = atof(planeLocation_str.c_str());
    } catch (const std::exception& e) {
        throw std::runtime_error("Invalid plane location: " + planeLocation_str);
    }
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
    if (!args["energyWeighted"].empty()) {
        energyWeighted = (args["energyWeighted"][0] == "true");
    }
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

    // Create the reader for the input file
    std::unique_ptr<PhaseSpaceFileReader> reader;
    if (inputFormat.empty()) {
        reader = FormatRegistry::CreateReader(inputFile, args);
    } else {
        reader = FormatRegistry::CreateReader(inputFormat, inputFile, args);
    }

    // Print out the parameters
    std::cout << "Parameters:" << std::endl;
    std::cout << "  Image Format: " << (outputFormat == TIFF ? "TIFF" : "BMP") << std::endl;
    std::cout << "  Plane: " << (plane == XY ? "XY" : (plane == XZ ? "XZ" : "YZ")) << std::endl;
    std::cout << "  Plane Location: " << planeLocation << " cm" << std::endl;
    std::cout << "  Input File: " << inputFile << " (Format: " << reader->getPHSPFormat() << ")" << std::endl;
    std::cout << "  Output File: " << outputFile << std::endl; 
    std::cout << "  Image Width: " << imageWidth << " pixels" << std::endl;
    std::cout << "  Image Height: " << imageHeight << " pixels" << std::endl;
    std::cout << "  Dimensions: [" << minDim1 << ", " << maxDim1 << "] cm x [" 
              << minDim2 << ", " << maxDim2 << "] cm" << std::endl;
    std::cout << "  Thickness in third dimension: " << widthDim3 << " cm" << std::endl;
    std::cout << "  Energy Weighted: " << (energyWeighted ? "true" : "false") << std::endl;
    std::cout << "  Max Particles to Read: " 
              << (args["maxParticles"].empty() ? "all" : args["maxParticles"][0]) << std::endl;

    // Error handling for both reader and writer
    try {
        // Start the process
        std::cout << "Counting particles from " 
                  << inputFile << " (" << reader->getPHSPFormat() << ") to store in image "
                  << outputFile << "..." << std::endl;

        // Determine how many particles to read - capping out at maxParticles if a limit has been set
        uint64_t particlesInFile = reader->getNumberOfParticles();
        uint64_t maxParticles = args["maxParticles"].empty() ? particlesInFile : std::stoull(args["maxParticles"][0]);
        uint64_t particlesToRead = particlesInFile > maxParticles ? maxParticles : particlesInFile;
        
        // Determine progress update interval
        uint64_t onePercentInterval = particlesToRead >= 100 
                                    ? particlesToRead / 100 
                                    : 1;

        // Check if there are particles to read
        uint64_t numberOfHistories = reader->getNumberOfOriginalHistories();
        if (numberOfHistories == 0 || particlesToRead == 0) {
            throw std::runtime_error("No particle histories found in the input file.");
        }

        // Calculate pixel mapping
        float xPixelsPerUnitLength = static_cast<float>(imageWidth) / (maxDim1 - minDim1);
        float yPixelsPerUnitLength = static_cast<float>(imageHeight) / (maxDim2 - minDim2);
        float xOffset = static_cast<float>(minDim1) * xPixelsPerUnitLength;
        float yOffset = static_cast<float>(minDim2) * yPixelsPerUnitLength;
        float pixelArea = (maxDim1 - minDim1) * (maxDim2 - minDim2) / (imageWidth * imageHeight);

        // Start the timer
        auto start_time = std::chrono::steady_clock::now();

        // Create the image object
        Image<float> * image;
        if (outputFormat == TIFF) {
            image = new TiffImage<float>(imageWidth, imageHeight, xPixelsPerUnitLength, yPixelsPerUnitLength, xOffset, yOffset);
        } else if (outputFormat == BMP) {
            image = new BitmapImage<float>(imageWidth, imageHeight);
        } else {
            throw std::runtime_error("Unsupported output format.");
        }

        // Set up the progress bar for the current file
        Progress<uint64_t> progress(particlesToRead);
        progress.Start("Reading particles:");

        // Read the particles from the input file and build the image data
        for (uint64_t particlesSoFar = 1 ; reader->hasMoreParticles() && particlesSoFar <= particlesToRead ; particlesSoFar++) {
            Particle particle = reader->getNextParticle();

            // Process particle
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

            // Determine the weight to associate with this particle
            float weight = particle.getWeight();
            if (energyWeighted) {
                weight *= particle.getKineticEnergy() / MeV;
            }

            // Set pixel color based on the particle's weight
            float weightPerUnitArea = weight / pixelArea; // counts per cm2 or MeV per cm2
            float pixelValue = image->getGrayscaleValue(pixelX, pixelY) + weightPerUnitArea;
            image->setGrayscaleValue(pixelX, pixelY, pixelValue);

            // Update progress bar every 1% of particles read
            if (particlesSoFar % onePercentInterval == 0) {
                progress.Update(particlesSoFar, "Processed " + std::to_string(reader->getHistoriesRead()) + " histories.");
            }
        }

        // Finalize the image by normalizing the data by the number of histories
        std::uint64_t historiesRead = particlesToRead < particlesInFile ? reader->getHistoriesRead() : numberOfHistories;
        image->normalize(static_cast<float>(historiesRead));

        // Save the image to the output file
        image->save(outputFile);

        // Complete the progress bar
        progress.Complete("Image generation complete. Processed " + std::to_string(historiesRead) + " histories.");

        // Clean up resources
        delete image;

        // Measure elapsed time and report it
        auto end_time = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(end_time - start_time).count();
        std::cout << "Time taken: " << elapsed << " seconds" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error occurred: " << e.what() << std::endl;
        errorCode = 1;
    }

    // Ensure that the reader is closed even if an exception occurs
    if (reader) reader->close();

    // Return the error code
    return errorCode;
}