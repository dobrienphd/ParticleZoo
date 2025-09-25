
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
 *   inputfile                 Input phase space file
 *   outputfile                Output image file path
 * 
 * Optional Arguments:
 *   --plane <XY|XZ|YZ>        Imaging plane orientation (default: XY)
 *                             XY: view from Z-axis, XZ: view from Y-axis, YZ: view from X-axis
 *   --planeLocation <value>   Location of the imaging plane in cm (default: 0.0)
 *   --projectTo <value>       Project particles to this plane location in cm
 *   --projectionType <type>   Projection scheme: none, project, or flatten (default: flatten)
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
 *   --square <value>          Side length of square region for imaging
 *   
 *   Plane Thickness:
 *   --tolerance <value>          Tolerance in the perpendicular direction (default: 0.25 cm)
 *   
 *   Processing Options:
 *   --maxParticles <N>             Limit the maximum number of particles to process (default: all)
 *   --energyWeighted <true|false>  Set to true to produce energy fluence instead of particle fluence (default: false)
 *   --normalizeByParticles <true|false>  Normalize by particles instead of histories (default: false)
 *   --inputFormat <format>         Force input file format (default: auto-detect from extension)
 *   --outputFormat <tiff|bmp>      Force output image format (default: tiff)
 *   --showDetails                  Print detailed information about the parameters being used
 *   --formats                      Display supported input file formats and exit
 *   --help                         Display usage information and exit
 *
 * USAGE EXAMPLES:
 *   # Generate XY plane image at Z=0 with default settings
 *   PHSPImage beam.egsphsp output.tiff
 * 
 *   # Create XZ plane view at Y=5cm with custom dimensions
 *   PHSPImage --plane XZ --projectionType none --planeLocation 5.0 --minX -10 --maxX 10 --minZ -5 --maxZ 15 beam.IAEAphsp profile.tiff
 * 
 *   # Energy-weighted image with 1000x1000 resolution
 *   PHSPImage --energyWeighted true --imageWidth 1000 --imageHeight 1000 dose.phsp dose_map.bmp
 * 
 *   # Process only first 100,000 particles with thick imaging plane
 *   PHSPImage --maxParticles 100000 --tolerance 1.0 simulation.root beam_profile.tiff
 * 
 *   # Project particles to a specific plane location
 *   PHSPImage --projectionType project --projectTo 10.0 beam.phsp projected.tiff
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

enum class ProjectionType {
    FLATTEN,    // All particle Z coordinates will be forced to the plane location
    PROJECTION, // Particles will be projected onto the plane based on their direction
    NONE        // Only particles that are already at the plane location will be counted
};

int main(int argc, char* argv[]) {

    // Initial setup
    using namespace ParticleZoo;
    int errorCode = 0;
    ImageFormat outputFormat = TIFF; // default output format
    bool energyWeighted = false; // default to particle fluence instead of energy fluence
    bool normalizeByParticles = false; // default to normalizing by histories instead of particles
    bool printDetails = false; // default to not printing detailed info about the parameters being used
    Plane plane = XY; // default plane
    float planeLocation = 0; // default plane location
    ProjectionType projectionType = ProjectionType::FLATTEN; // default projection type

    // Set default dimensions and image size
    float minDim1 = -40 * cm;
    float maxDim1 = 40 * cm;
    float minDim2 = -40 * cm;
    float maxDim2 = 40 * cm;
    float tolerance = 0.25 * cm; // tolerance for the third dimension
    int imageWidth = 1024;
    int imageHeight = 1024;

    // Custom command line arguments
    const CLICommand INPUT_FORMAT_COMMAND = CLICommand(NONE, "", "inputFormat", "Force input file format (default: auto-detect from extension)", { CLI_STRING });
    const CLICommand OUTPUT_FORMAT_COMMAND = CLICommand(NONE, "", "outputFormat", "Force output image format (tiff or bmp)", { CLI_STRING }, { "tiff" });
    const CLICommand PLANE_COMMAND = CLICommand(NONE, "", "plane", "Imaging plane orientation (XY, XZ, or YZ)", { CLI_STRING }, { "XY" });
    const CLICommand PLANE_LOCATION_COMMAND = CLICommand(NONE, "", "planeLocation", "Location of the imaging plane in cm", { CLI_FLOAT }, { 0.0f });
    const CLICommand PROJECT_TO_COMMAND = CLICommand(NONE, "", "projectTo", "Project particles to this plane location in cm (enables projection mode)", { CLI_FLOAT });
    const CLICommand PROJECTION_TYPE_COMMAND = CLICommand(NONE, "", "projectionType", "Projection scheme: none, project, or flatten", { CLI_STRING }, { "flatten" });
    const CLICommand IMAGE_WIDTH_COMMAND = CLICommand(NONE, "", "imageWidth", "Output image width in pixels", { CLI_INT }, { 1024 });
    const CLICommand IMAGE_HEIGHT_COMMAND = CLICommand(NONE, "", "imageHeight", "Output image height in pixels", { CLI_INT }, { 1024 });
    const CLICommand MIN_X_COMMAND = CLICommand(NONE, "", "minX", "Minimum X coordinate for imaging region in cm", { CLI_FLOAT }, { -40.0f });
    const CLICommand MAX_X_COMMAND = CLICommand(NONE, "", "maxX", "Maximum X coordinate for imaging region in cm", { CLI_FLOAT }, { 40.0f });
    const CLICommand MIN_Y_COMMAND = CLICommand(NONE, "", "minY", "Minimum Y coordinate for imaging region in cm", { CLI_FLOAT }, { -40.0f });
    const CLICommand MAX_Y_COMMAND = CLICommand(NONE, "", "maxY", "Maximum Y coordinate for imaging region in cm", { CLI_FLOAT }, { 40.0f });
    const CLICommand MIN_Z_COMMAND = CLICommand(NONE, "", "minZ", "Minimum Z coordinate for imaging region in cm", { CLI_FLOAT }, { -40.0f });
    const CLICommand MAX_Z_COMMAND = CLICommand(NONE, "", "maxZ", "Maximum Z coordinate for imaging region in cm", { CLI_FLOAT }, { 40.0f });
    const CLICommand SQUARE_COMMAND = CLICommand(NONE, "", "square", "Side length of square region (centered at 0,0) for imaging in cm (overrides min/max for both dimensions)", { CLI_FLOAT });
    const CLICommand TOLERANCE_COMMAND = CLICommand(NONE, "", "tolerance", "Tolerance in the direction perpendicular to the plane in cm", { CLI_FLOAT }, { 0.25f });
    const CLICommand MAX_PARTICLES_COMMAND = CLICommand(NONE, "", "maxParticles", "Maximum number of particles to process (default: unlimited)", { CLI_INT });
    const CLICommand ENERGY_WEIGHTED_COMMAND = CLICommand(NONE, "", "energyWeighted", "Score energy fluence instead of particle fluence", { CLI_VALUELESS });
    const CLICommand NORMALIZE_BY_PARTICLES_COMMAND = CLICommand(NONE, "", "normalizeByParticles", "Normalize by particles instead of histories", { CLI_VALUELESS });
    const CLICommand SHOW_DETAILS_COMMAND = CLICommand(NONE, "", "showDetails", "Show detailed info about the parameters being used", { CLI_VALUELESS });
    ArgParser::RegisterCommand(INPUT_FORMAT_COMMAND);
    ArgParser::RegisterCommand(OUTPUT_FORMAT_COMMAND);
    ArgParser::RegisterCommand(PLANE_COMMAND);
    ArgParser::RegisterCommand(PLANE_LOCATION_COMMAND);
    ArgParser::RegisterCommand(PROJECT_TO_COMMAND);
    ArgParser::RegisterCommand(PROJECTION_TYPE_COMMAND);
    ArgParser::RegisterCommand(IMAGE_WIDTH_COMMAND);
    ArgParser::RegisterCommand(IMAGE_HEIGHT_COMMAND);
    ArgParser::RegisterCommand(MIN_X_COMMAND);
    ArgParser::RegisterCommand(MAX_X_COMMAND);
    ArgParser::RegisterCommand(MIN_Y_COMMAND);
    ArgParser::RegisterCommand(MAX_Y_COMMAND);
    ArgParser::RegisterCommand(MIN_Z_COMMAND);
    ArgParser::RegisterCommand(MAX_Z_COMMAND);
    ArgParser::RegisterCommand(SQUARE_COMMAND);
    ArgParser::RegisterCommand(TOLERANCE_COMMAND);
    ArgParser::RegisterCommand(MAX_PARTICLES_COMMAND);
    ArgParser::RegisterCommand(ENERGY_WEIGHTED_COMMAND);
    ArgParser::RegisterCommand(NORMALIZE_BY_PARTICLES_COMMAND);
    
    // Define usage message and parse command line arguments
    std::string usageMessage = "Usage: PHSPImage [OPTIONS] <inputfile> <outputfile>\n"
                                "\n"
                                "Convert particle phase space files to 2D images of the fluence distributions.\n"
                                "\n"
                                "Required Arguments:\n"
                                "  <inputfile>               Input phase space file to visualize\n"
                                "  <outputfile>              Output image file path\n"
                                "\n"
                                "Examples:\n"
                                "  PHSPImage beam.egsphsp output.tiff\n"
                                "  PHSPImage --plane XZ --square 10 beam.IAEAphsp XZ10x10.tiff\n"
                                "  PHSPImage --energyWeighted --imageWidth 2048 input.phsp hiResEnergyFluence.bmp\n"
                                "  PHSPImage --projectTo 100.0 beam.phsp projectedAtIso.tiff";
    auto userOptions = ArgParser::ParseArgs(argc, argv, usageMessage, 2);

    // Validate parameters
    std::uint64_t maxParticles = userOptions.contains(MAX_PARTICLES_COMMAND) ? std::get<int>(userOptions.at(MAX_PARTICLES_COMMAND)[0]) : std::numeric_limits<uint64_t>::max();
    std::vector<CLIValue> positionals = userOptions.contains(CLI_POSITIONALS) ? userOptions.at(CLI_POSITIONALS) : std::vector<CLIValue>{ "", "" };
    std::string inputFile = std::get<std::string>(positionals[0]);
    std::string outputFile = std::get<std::string>(positionals[1]);
    std::string inputFormat = userOptions.contains(INPUT_FORMAT_COMMAND) ? (userOptions.at(INPUT_FORMAT_COMMAND).empty() ? "" : std::get<std::string>(userOptions.at(INPUT_FORMAT_COMMAND)[0])) : "";
    if (inputFile.empty()) throw std::runtime_error("No input file specified.");
    if (outputFile.empty()) throw std::runtime_error("No output file specified.");
    if (inputFile == outputFile) throw std::runtime_error("Input and output files must be different.");
    if (userOptions.contains(PROJECT_TO_COMMAND)) {
        try {
            projectionType = ProjectionType::PROJECTION;
            planeLocation = std::get<float>(userOptions.at(PROJECT_TO_COMMAND)[0]) * cm;
        } catch (const std::exception&) {
            throw std::runtime_error("Invalid project-to plane location");
        }
    }
    if (userOptions.contains(PLANE_LOCATION_COMMAND)) {
        planeLocation = std::get<float>(userOptions.at(PLANE_LOCATION_COMMAND)[0]) * cm;
    }
    if (userOptions.contains(PROJECTION_TYPE_COMMAND)) {
        std::string projectionType_str = std::get<std::string>(userOptions.at(PROJECTION_TYPE_COMMAND)[0]);
        if (projectionType_str == "none") {
            projectionType = ProjectionType::NONE;
        } else if (projectionType_str == "project") {
            projectionType = ProjectionType::PROJECTION;
        } else if (projectionType_str == "flatten") {
            projectionType = ProjectionType::FLATTEN;
        } else {
            throw std::runtime_error("Invalid projection type specified. Use none, project, or flatten.");
        }
    }
    if (userOptions.contains(PLANE_COMMAND)) {
        std::string planeStr = std::get<std::string>(userOptions.at(PLANE_COMMAND)[0]);
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
    if (userOptions.contains(IMAGE_WIDTH_COMMAND)) {
        imageWidth = std::get<int>(userOptions.at(IMAGE_WIDTH_COMMAND)[0]);
        if (imageWidth <= 0) {
            throw std::runtime_error("Image width must be a positive integer.");
        }
    }
    if (userOptions.contains(IMAGE_HEIGHT_COMMAND)) {
        imageHeight = std::get<int>(userOptions.at(IMAGE_HEIGHT_COMMAND)[0]);
        if (imageHeight <= 0) {
            throw std::runtime_error("Image height must be a positive integer.");
        }
    }
    if (userOptions.contains(SQUARE_COMMAND)) {
        float squareSide = std::get<float>(userOptions.at(SQUARE_COMMAND)[0]) * cm;
        minDim1 = -squareSide/2.f;
        minDim2 = minDim1;
        maxDim1 = squareSide/2.f;
        maxDim2 = maxDim1;
    }
    if (plane == XY && userOptions.contains(MIN_X_COMMAND)) {
        minDim1 = std::get<float>(userOptions.at(MIN_X_COMMAND)[0]) * cm;
    }
    if (plane == XY && userOptions.contains(MAX_X_COMMAND)) {
        maxDim1 = std::get<float>(userOptions.at(MAX_X_COMMAND)[0]) * cm;
    }
    if (plane == XY && userOptions.contains(MIN_Y_COMMAND)) {
        minDim2 = std::get<float>(userOptions.at(MIN_Y_COMMAND)[0]) * cm;
    }
    if (plane == XY && userOptions.contains(MAX_Y_COMMAND)) {
        maxDim2 = std::get<float>(userOptions.at(MAX_Y_COMMAND)[0]) * cm;
    }
    if (plane == XZ && userOptions.contains(MIN_X_COMMAND)) {
        minDim1 = std::get<float>(userOptions.at(MIN_X_COMMAND)[0]) * cm;
    }
    if (plane == XZ && userOptions.contains(MAX_X_COMMAND)) {
        maxDim1 = std::get<float>(userOptions.at(MAX_X_COMMAND)[0]) * cm;
    }
    if (plane == XZ && userOptions.contains(MIN_Z_COMMAND)) {
        minDim2 = std::get<float>(userOptions.at(MIN_Z_COMMAND)[0]) * cm;
    }
    if (plane == XZ && userOptions.contains(MAX_Z_COMMAND)) {
        maxDim2 = std::get<float>(userOptions.at(MAX_Z_COMMAND)[0]) * cm;
    }
    if (plane == YZ && userOptions.contains(MIN_Y_COMMAND)) {
        minDim1 = std::get<float>(userOptions.at(MIN_Y_COMMAND)[0]) * cm;
    }
    if (plane == YZ && userOptions.contains(MAX_Y_COMMAND)) {
        maxDim1 = std::get<float>(userOptions.at(MAX_Y_COMMAND)[0]) * cm;
    }
    if (plane == YZ && userOptions.contains(MIN_Z_COMMAND)) {
        minDim2 = std::get<float>(userOptions.at(MIN_Z_COMMAND)[0]) * cm;
    }
    if (plane == YZ && userOptions.contains(MAX_Z_COMMAND)) {
        maxDim2 = std::get<float>(userOptions.at(MAX_Z_COMMAND)[0]) * cm;
    }
    if (userOptions.contains(TOLERANCE_COMMAND)) {
        tolerance = std::get<float>(userOptions.at(TOLERANCE_COMMAND)[0]) * cm;
        if (tolerance < 0) {
            throw std::runtime_error("Tolerance cannot be a negative number.");
        }
    }
    if (minDim1 >= maxDim1 || minDim2 >= maxDim2) {
        throw std::runtime_error("Invalid dimensions specified. Ensure that min < max for both dimensions.");
    }
    if (userOptions.contains(ENERGY_WEIGHTED_COMMAND)) {
        energyWeighted = true;
    }
    if (userOptions.contains(NORMALIZE_BY_PARTICLES_COMMAND)) {
        normalizeByParticles = true;
    }
    if (userOptions.contains(SHOW_DETAILS_COMMAND)) {
        printDetails = true;
    }
    if (userOptions.contains(OUTPUT_FORMAT_COMMAND)) {
        std::string formatStr = std::get<std::string>(userOptions.at(OUTPUT_FORMAT_COMMAND)[0]);
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
        reader = FormatRegistry::CreateReader(inputFile, userOptions);
    } else {
        reader = FormatRegistry::CreateReader(inputFormat, inputFile, userOptions);
    }

    if (printDetails) {
        // Print out the parameters
        std::cout << "Parameters:" << std::endl;
        std::cout << "  Image Format: " << (outputFormat == TIFF ? "TIFF" : "BMP") << std::endl;
        std::cout << "  Plane: " << (plane == XY ? "XY" : (plane == XZ ? "XZ" : "YZ")) << std::endl;
        if (projectionType != ProjectionType::FLATTEN) {
            std::cout << "  Plane Location: " << planeLocation << " cm" << std::endl;
        }
        std::cout << "  Projection Scheme: " << (projectionType == ProjectionType::PROJECTION ? "Projection" : projectionType == ProjectionType::FLATTEN ? "Flatten" : "None") << std::endl;
        std::cout << "  Input File: " << inputFile << " (Format: " << reader->getPHSPFormat() << ")" << std::endl;
        std::cout << "  Output File: " << outputFile << std::endl; 
        std::cout << "  Image Width: " << imageWidth << " pixels" << std::endl;
        std::cout << "  Image Height: " << imageHeight << " pixels" << std::endl;
        std::cout << "  Dimensions: [" << minDim1 << ", " << maxDim1 << "] cm x [" << minDim2 << ", " << maxDim2 << "] cm" << std::endl;
        if (projectionType == ProjectionType::NONE) {
            std::cout << "  Thickness in third dimension: " << tolerance << " cm" << std::endl;
        }
        std::cout << "  Energy Weighted: " << (energyWeighted ? "true" : "false") << std::endl;
        std::cout << "  Max Particles to Read: " << (maxParticles == std::numeric_limits<uint64_t>::max() ? "all" : std::to_string(maxParticles)) << std::endl;
    }

    // Error handling for both reader and writer
    try {
        // Start the process
        std::cout << "Counting particles from " 
                  << inputFile << " (" << reader->getPHSPFormat() << ") to store in image "
                  << outputFile << "..." << std::endl;

        // Determine how many particles to read - capping out at maxParticles if a limit has been set
        uint64_t particlesInFile = reader->getNumberOfParticles();
        uint64_t particlesToRead = particlesInFile > maxParticles ? maxParticles : particlesInFile;
        
        // Determine progress update interval
        uint64_t onePercentInterval = particlesToRead >= 100 
                                    ? particlesToRead / 100 
                                    : 1;

        // Check if there are particles to read
        if (particlesToRead == 0) {
            throw std::runtime_error("No particles found in the input file.");
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

            // project particle to the scoring plane based on selected projection scheme
            switch (projectionType)
            {
                case(ProjectionType::FLATTEN):
                    particle.setZ(planeLocation);
                    break;
                case(ProjectionType::PROJECTION):
                    switch (plane)
                    {
                        case(XY): particle.projectToZValue(planeLocation); break;
                        case(XZ): particle.projectToYValue(planeLocation); break;
                        case(YZ): particle.projectToXValue(planeLocation); break;
                    };
                    break;
                default:
                    break;
            }
            
            // Process particle
            float x = particle.getX() / cm;
            float y = particle.getY() / cm;
            float z = particle.getZ() / cm;

            // Determine pixel coordinates based on the selected plane
            int pixelX = 0, pixelY = 0;
            bool validPixel = false;
            if (plane == XY && std::abs(z - planeLocation) < tolerance && x >= minDim1 && x <= maxDim1 && y >= minDim2 && y <= maxDim2) {
                pixelX = static_cast<int>((x - minDim1) / (maxDim1 - minDim1) * imageWidth);
                pixelY = static_cast<int>((y - minDim2) / (maxDim2 - minDim2) * imageHeight);
                validPixel = true;
            } else if (plane == XZ && std::abs(y - planeLocation) < tolerance && x >= minDim1 && x <= maxDim1 && z >= minDim2 && z <= maxDim2) {
                pixelX = static_cast<int>((x - minDim1) / (maxDim1 - minDim1) * imageWidth);
                pixelY = static_cast<int>((z - minDim2) / (maxDim2 - minDim2) * imageHeight);
                validPixel = true;
            } else if (plane == YZ && std::abs(x - planeLocation) < tolerance && y >= minDim1 && y <= maxDim1 && z >= minDim2 && z <= maxDim2) {
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

        // Finalize the image by normalizing the data by the number of histories (or particles if specified by the user)
        uint64_t numberOfHistories = reader->getNumberOfOriginalHistories();
        uint64_t particlesRead = reader->getParticlesRead();
        std::uint64_t historiesRead = particlesRead < particlesInFile ? reader->getHistoriesRead() : numberOfHistories;
        if (normalizeByParticles) {
            image->normalize(static_cast<float>(particlesRead));
        } else {
            image->normalize(static_cast<float>(historiesRead));
        }

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
        std::cerr << std::endl << "Error occurred: " << e.what() << std::endl;
        errorCode = 1;
    }

    // Ensure that the reader is closed even if an exception occurs
    if (reader) reader->close();

    // Return the error code
    return errorCode;
}