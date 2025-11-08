
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

// Anonymous namespace for internal definitions
namespace {

    using namespace ParticleZoo;

    // Usage message
    constexpr std::string_view usageMessage = "Usage: PHSPImage [OPTIONS] <inputfile> <outputfile>\n"
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


    // Default parameter values
    constexpr static float DEFAULT_DISTANCE = 40.0f * cm;
    constexpr static float DEFAULT_TOLERANCE = 0.25f * cm;
    constexpr static float DEFAULT_IMAGE_SIDE = 1024;
    constexpr static float DEFAULT_PLANE_LOCATION = 0.0f * cm;
    constexpr static std::uint64_t DEFAULT_MAX_PARTICLES = std::numeric_limits<std::uint64_t>::max();


    // Custom command line arguments
    const CLICommand INPUT_FORMAT_COMMAND = CLICommand(NONE, "", "inputFormat", "Force input file format (default: auto-detect from extension)", { CLI_STRING });
    const CLICommand OUTPUT_FORMAT_COMMAND = CLICommand(NONE, "", "outputFormat", "Force output image format (tiff or bmp)", { CLI_STRING }, { "tiff" });
    const CLICommand PLANE_COMMAND = CLICommand(NONE, "", "plane", "Imaging plane orientation (XY, XZ, or YZ)", { CLI_STRING }, { "XY" });
    const CLICommand PLANE_LOCATION_COMMAND = CLICommand(NONE, "", "planeLocation", "Location of the imaging plane in cm", { CLI_FLOAT }, { DEFAULT_PLANE_LOCATION });
    const CLICommand PROJECT_TO_COMMAND = CLICommand(NONE, "", "projectTo", "Project particles to this plane location in cm (enables projection mode)", { CLI_FLOAT });
    const CLICommand PROJECTION_TYPE_COMMAND = CLICommand(NONE, "", "projectionType", "Projection scheme: none, project, or flatten", { CLI_STRING }, { "flatten" });
    const CLICommand IMAGE_WIDTH_COMMAND = CLICommand(NONE, "", "imageWidth", "Output image width in pixels", { CLI_INT }, { DEFAULT_IMAGE_SIDE });
    const CLICommand IMAGE_HEIGHT_COMMAND = CLICommand(NONE, "", "imageHeight", "Output image height in pixels", { CLI_INT }, { DEFAULT_IMAGE_SIDE });
    const CLICommand MIN_X_COMMAND = CLICommand(NONE, "", "minX", "Minimum X coordinate for imaging region in cm (default: -40.0 cm)", { CLI_FLOAT });
    const CLICommand MAX_X_COMMAND = CLICommand(NONE, "", "maxX", "Maximum X coordinate for imaging region in cm (default: 40.0 cm)", { CLI_FLOAT });
    const CLICommand MIN_Y_COMMAND = CLICommand(NONE, "", "minY", "Minimum Y coordinate for imaging region in cm (default: -40.0 cm)", { CLI_FLOAT });
    const CLICommand MAX_Y_COMMAND = CLICommand(NONE, "", "maxY", "Maximum Y coordinate for imaging region in cm (default: 40.0 cm)", { CLI_FLOAT });
    const CLICommand MIN_Z_COMMAND = CLICommand(NONE, "", "minZ", "Minimum Z coordinate for imaging region in cm (default: -40.0 cm)", { CLI_FLOAT });
    const CLICommand MAX_Z_COMMAND = CLICommand(NONE, "", "maxZ", "Maximum Z coordinate for imaging region in cm (default: 40.0 cm)", { CLI_FLOAT });
    const CLICommand SQUARE_COMMAND = CLICommand(NONE, "", "square", "Side length of square region (centered at 0,0) for imaging in cm (overrides min/max for both dimensions)", { CLI_FLOAT });
    const CLICommand TOLERANCE_COMMAND = CLICommand(NONE, "", "tolerance", "Tolerance in the direction perpendicular to the plane in cm", { CLI_FLOAT }, { DEFAULT_TOLERANCE });
    const CLICommand MAX_PARTICLES_COMMAND = CLICommand(NONE, "", "maxParticles", "Maximum number of particles to process (default: unlimited)", { CLI_INT });
    const CLICommand ENERGY_WEIGHTED_COMMAND = CLICommand(NONE, "", "energyWeighted", "Score energy fluence (equivalent to --score energy)", { CLI_VALUELESS });
    const CLICommand QUANTITY_TYPE_COMMAND = CLICommand(NONE, "", "score", "Quantity to score (particle weight applies to all quantities and each is normalized by unit area): count, energy, xDir, yDir, zDir", { CLI_STRING }, { "count" });
    const CLICommand PRIMARIES_ONLY_COMMAND = CLICommand(NONE, "", "primariesOnly", "Only process primary particles from the phase space file", { CLI_VALUELESS });
    const CLICommand EXCLUDE_PRIMARIES_COMMAND = CLICommand(NONE, "", "excludePrimaries", "Exclude primary particles from processing", { CLI_VALUELESS });
    const CLICommand NORMALIZE_BY_PARTICLES_COMMAND = CLICommand(NONE, "", "normalizeByParticles", "Normalize by particles instead of histories", { CLI_VALUELESS });
    const CLICommand SHOW_DETAILS_COMMAND = CLICommand(NONE, "", "showDetails", "Show detailed info about the parameters being used", { CLI_VALUELESS });
    const CLICommand ERROR_ON_WARNING_COMMAND = CLICommand(NONE, "", "errorOnWarning", "Treat warnings as errors when returning exit code", { CLI_VALUELESS });


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


    // Enum for the projection type
    enum class ProjectionType {
        FLATTEN,    // All particle Z coordinates will be forced to the plane location
        PROJECTION, // Particles will be projected onto the plane based on their direction
        NONE        // Only particles that are already at the plane location will be counted
    };


    // Enum for the quantity type
    enum class QuantityType {
        PARTICLE_FLUENCE,
        ENERGY_FLUENCE,
        X_DIRECTIONAL_COSINE,
        Y_DIRECTIONAL_COSINE,
        Z_DIRECTIONAL_COSINE
    };


    // Enum for the particle generation type
    enum class GenerationType {
        ALL,
        PRIMARIES_ONLY,
        EXCLUDE_PRIMARIES
    };


    // App configuration state
    struct AppConfig
    {
            const Plane          plane;
            private:
            const std::array<float, 4> dimensionLimits; // [min1, max1], [min2, max2]
            public:
            const std::string    inputFile;
            const std::string    outputFile;
            const std::string    inputFormat;
            const ImageFormat    outputFormat;
            const std::uint64_t  maxParticles;
            const bool           normalizeByParticles;
            const bool           printDetails;

            const ProjectionType projectionType;
            const QuantityType   quantityType;
            const GenerationType generationType;

            const float          tolerance;
            const int            imageWidth;
            const int            imageHeight;

            const float          planeLocation;

            const bool           errorOnWarning;

            // Constructor to initialize from user options
            AppConfig(const UserOptions & userOptions)
            :   plane(determinePlane(userOptions)),
                dimensionLimits(determineDimensionLimits(userOptions)),
                inputFile(userOptions.extractPositional(0)),
                outputFile(userOptions.extractPositional(1)),
                inputFormat(userOptions.extractStringOption(INPUT_FORMAT_COMMAND)),
                outputFormat(determineOutputFormat(userOptions)),
                maxParticles(static_cast<std::uint64_t>(userOptions.extractIntOption(MAX_PARTICLES_COMMAND, DEFAULT_MAX_PARTICLES))),
                normalizeByParticles(userOptions.contains(NORMALIZE_BY_PARTICLES_COMMAND)),
                printDetails(userOptions.contains(SHOW_DETAILS_COMMAND)),
                projectionType(determineProjectionType(userOptions)),
                quantityType(determineQuantityType(userOptions)),
                generationType(determineGenerationType(userOptions)),
                tolerance(projectionType == ProjectionType::NONE ? userOptions.extractFloatOption(TOLERANCE_COMMAND, DEFAULT_TOLERANCE) * cm : 0.0f),
                imageWidth(userOptions.extractIntOption(IMAGE_WIDTH_COMMAND, DEFAULT_IMAGE_SIDE)),
                imageHeight(userOptions.extractIntOption(IMAGE_HEIGHT_COMMAND, DEFAULT_IMAGE_SIDE)),
                planeLocation(determinePlaneLocation(userOptions)),
                errorOnWarning(userOptions.contains(ERROR_ON_WARNING_COMMAND))
            {
                // Validate the configuration
                validate(userOptions);
            }

            float minDim1() const { return dimensionLimits[0]; }
            float maxDim1() const { return dimensionLimits[1]; }
            float minDim2() const { return dimensionLimits[2]; }
            float maxDim2() const { return dimensionLimits[3]; }

            void details(const std::string& detectedFormat = "") const {
                std::stringstream ss;
                ss << "Parameters:\n";
                ss << "  Image Format: " << (outputFormat == TIFF ? "TIFF" : "BMP") << "\n";
                ss << "  Plane: " << (plane == XY ? "XY" : (plane == XZ ? "XZ" : "YZ")) << "\n";
                if (projectionType != ProjectionType::FLATTEN) {
                    ss << "  Plane Location: " << planeLocation / cm << " cm\n";
                }
                ss << "  Projection Scheme: " << (projectionType == ProjectionType::PROJECTION ? "Projection" : projectionType == ProjectionType::FLATTEN ? "Flatten" : "None") << "\n";
                ss << "  Input File: " << inputFile;
                if (!detectedFormat.empty()) {
                    ss << " (Format: " << detectedFormat << ")";
                }
                ss << "\n";
                // Show input format if forced, otherwise the detected/auto mode
                ss << "  Input Format: ";
                if (!inputFormat.empty()) {
                    ss << inputFormat << " (forced)\n";
                } else if (!detectedFormat.empty()) {
                    ss << detectedFormat << " (auto-detected)\n";
                } else {
                    ss << "auto\n";
                }
                ss << "  Output File: " << outputFile << "\n";
                ss << "  Image Width: " << imageWidth << " pixels\n";
                ss << "  Image Height: " << imageHeight << " pixels\n";
                ss << "  Dimensions: [" << minDim1()/cm << ", " << maxDim1()/cm << "] cm x [" << minDim2()/cm << ", " << maxDim2()/cm << "] cm\n";
                if (projectionType == ProjectionType::NONE) {
                    ss << "  Thickness in third dimension: " << tolerance/cm << " cm\n";
                }
                ss << "  Quantity scored: " 
                << (quantityType == QuantityType::PARTICLE_FLUENCE ? "Particle Fluence" 
                    : quantityType == QuantityType::ENERGY_FLUENCE ? "Energy Fluence" 
                    : quantityType == QuantityType::X_DIRECTIONAL_COSINE ? "X Directional Cosine"
                    : quantityType == QuantityType::Y_DIRECTIONAL_COSINE ? "Y Directional Cosine"
                    : "Z Directional Cosine") << "\n";
                // Show generation filter
                ss << "  Particle selection: "
                   << (generationType == GenerationType::ALL ? "All"
                       : generationType == GenerationType::PRIMARIES_ONLY ? "Primaries only"
                       : "Exclude primaries")
                   << "\n";
                ss << "  Max Particles to Read: " << (maxParticles == std::numeric_limits<std::uint64_t>::max() ? "all" : std::to_string(maxParticles)) << "\n";
                // Show normalization mode
                ss << "  Normalization: by " << (normalizeByParticles ? "particles" : "histories") << "\n";
                // Show error handling preference
                ss << "  Error on warnings: " << (errorOnWarning ? "true" : "false") << "\n";
                
                std::cout << ss.str() << std::flush;
            }

        private:

            ImageFormat determineOutputFormat(const UserOptions & userOptions) const {
                if (userOptions.contains(OUTPUT_FORMAT_COMMAND)) {
                    std::string formatStr = userOptions.extractStringOption(OUTPUT_FORMAT_COMMAND);
                    if (formatStr == "tiff" || formatStr == "TIFF") {
                        return TIFF;
                    } else if (formatStr == "bmp" || formatStr == "BMP") {
                        return BMP;
                    } else {
                        throw std::runtime_error("Unsupported output image format: " + formatStr);
                    }
                } else return TIFF; // default to TIFF
            }

            Plane determinePlane(const UserOptions & userOptions) const {
                if (userOptions.contains(PLANE_COMMAND)) {
                    std::string planeStr = userOptions.extractStringOption(PLANE_COMMAND);
                    if (planeStr == "XY") {
                        return XY;
                    } else if (planeStr == "XZ") {
                        return XZ;
                    } else if (planeStr == "YZ") {
                        return YZ;
                    } else {
                        throw std::runtime_error("Invalid plane specified. Use XY, XZ, or YZ.");
                    }
                } else return XY; // default to XY
            }

            ProjectionType determineProjectionType(const UserOptions & userOptions) const {
                if (userOptions.contains(PROJECT_TO_COMMAND)) {
                    return ProjectionType::PROJECTION;
                } else if (userOptions.contains(PROJECTION_TYPE_COMMAND)) {
                    std::string projTypeStr = userOptions.extractStringOption(PROJECTION_TYPE_COMMAND);
                    if (projTypeStr == "none") {
                        return ProjectionType::NONE;
                    } else if (projTypeStr == "project") {
                        return ProjectionType::PROJECTION;
                    } else if (projTypeStr == "flatten") {
                        return ProjectionType::FLATTEN;
                    } else {
                        throw std::runtime_error("Invalid projection type specified. Use none, project, or flatten.");
                    }
                } else {
                    return ProjectionType::FLATTEN; // default to FLATTEN
                }
            }

            QuantityType determineQuantityType(const UserOptions & userOptions) const {
                if (userOptions.contains(QUANTITY_TYPE_COMMAND)) {
                    std::string quantityStr = userOptions.extractStringOption(QUANTITY_TYPE_COMMAND);
                    if (quantityStr == "count") {
                        return QuantityType::PARTICLE_FLUENCE;
                    } else if (quantityStr == "energy") {
                        return QuantityType::ENERGY_FLUENCE;
                    } else if (quantityStr == "xDir") {
                        return QuantityType::X_DIRECTIONAL_COSINE;
                    } else if (quantityStr == "yDir") {
                        return QuantityType::Y_DIRECTIONAL_COSINE;
                    } else if (quantityStr == "zDir") {
                        return QuantityType::Z_DIRECTIONAL_COSINE;
                    } else {
                        throw std::runtime_error("Invalid quantity type specified. Use count, energy, xDir, yDir, or zDir.");
                    }
                } else if (userOptions.contains(ENERGY_WEIGHTED_COMMAND)) {
                    return QuantityType::ENERGY_FLUENCE;
                } else {
                    return QuantityType::PARTICLE_FLUENCE; // default to PARTICLE_FLUENCE
                }
            }

            GenerationType determineGenerationType(const UserOptions & userOptions) const {
                if (userOptions.contains(PRIMARIES_ONLY_COMMAND) && userOptions.contains(EXCLUDE_PRIMARIES_COMMAND)) {
                    throw std::runtime_error("Cannot specify both --primariesOnly and --excludePrimaries.");
                } else if (userOptions.contains(PRIMARIES_ONLY_COMMAND)) {
                    return GenerationType::PRIMARIES_ONLY;
                } else if (userOptions.contains(EXCLUDE_PRIMARIES_COMMAND)) {
                    return GenerationType::EXCLUDE_PRIMARIES;
                } else {
                    return GenerationType::ALL; // default to ALL
                }
            }

            std::array<float, 4> determineDimensionLimits(const UserOptions & userOptions) const {
                // Start with defaults
                float min1, max1, min2, max2;
                min1 = min2 = -DEFAULT_DISTANCE;
                max1 = max2 =  DEFAULT_DISTANCE;
                // Set with square option if provided
                if (userOptions.contains(SQUARE_COMMAND)) {
                    float halfSide = (userOptions.extractFloatOption(SQUARE_COMMAND) * cm) / 2.0f;
                    min1 = -halfSide;
                    min2 = min1;
                    max1 = halfSide;
                    max2 = max1;
                }
                // Override with specific min/max if provided
                switch (plane) {
                    case XY:
                        min1 = userOptions.contains(MIN_X_COMMAND) ? userOptions.extractFloatOption(MIN_X_COMMAND) * cm : min1;
                        max1 = userOptions.contains(MAX_X_COMMAND) ? userOptions.extractFloatOption(MAX_X_COMMAND) * cm : max1;
                        min2 = userOptions.contains(MIN_Y_COMMAND) ? userOptions.extractFloatOption(MIN_Y_COMMAND) * cm : min2;
                        max2 = userOptions.contains(MAX_Y_COMMAND) ? userOptions.extractFloatOption(MAX_Y_COMMAND) * cm : max2;
                        break;
                    case XZ:
                        min1 = userOptions.contains(MIN_X_COMMAND) ? userOptions.extractFloatOption(MIN_X_COMMAND) * cm : min1;
                        max1 = userOptions.contains(MAX_X_COMMAND) ? userOptions.extractFloatOption(MAX_X_COMMAND) * cm : max1;
                        min2 = userOptions.contains(MIN_Z_COMMAND) ? userOptions.extractFloatOption(MIN_Z_COMMAND) * cm : min2;
                        max2 = userOptions.contains(MAX_Z_COMMAND) ? userOptions.extractFloatOption(MAX_Z_COMMAND) * cm : max2;
                        break;
                    case YZ:
                        min1 = userOptions.contains(MIN_Y_COMMAND) ? userOptions.extractFloatOption(MIN_Y_COMMAND) * cm : min1;
                        max1 = userOptions.contains(MAX_Y_COMMAND) ? userOptions.extractFloatOption(MAX_Y_COMMAND) * cm : max1;
                        min2 = userOptions.contains(MIN_Z_COMMAND) ? userOptions.extractFloatOption(MIN_Z_COMMAND) * cm : min2;
                        max2 = userOptions.contains(MAX_Z_COMMAND) ? userOptions.extractFloatOption(MAX_Z_COMMAND) * cm : max2;
                        break;
                }
                return {min1, max1, min2, max2};
            }

            float determinePlaneLocation(const UserOptions & userOptions) const {
                if (userOptions.contains(PROJECT_TO_COMMAND)) {
                    try {
                        return userOptions.extractFloatOption(PROJECT_TO_COMMAND) * cm;
                    } catch (const std::exception&) {
                        throw std::runtime_error("Invalid project-to plane location");
                    }
                } else if (userOptions.contains(PLANE_LOCATION_COMMAND)) {
                    try {
                        return userOptions.extractFloatOption(PLANE_LOCATION_COMMAND) * cm;
                    } catch (const std::exception&) {
                        throw std::runtime_error("Invalid plane location");
                    }
                } else {
                    return DEFAULT_PLANE_LOCATION;
                }
            }

            void validate(const UserOptions & userOptions) const {
                // Validate parameters
                if (inputFile.empty()) throw std::runtime_error("No input file specified.");
                if (outputFile.empty()) throw std::runtime_error("No output file specified.");
                if (inputFile == outputFile) throw std::runtime_error("Input and output files must be different.");
                if (minDim1() >= maxDim1()) throw std::runtime_error("Invalid dimensions specified. Ensure that min < max for both dimensions.");
                if (minDim2() >= maxDim2()) throw std::runtime_error("Invalid dimensions specified. Ensure that min < max for both dimensions.");
                if (tolerance < 0) throw std::runtime_error("Tolerance cannot be a negative number.");
                if (imageWidth <= 0) throw std::runtime_error("Image width must be a positive integer.");
                if (imageHeight <= 0) throw std::runtime_error("Image height must be a positive integer.");
            }
    };

} // end anonymous namespace


int main(int argc, char* argv[]) {

    // Use ParticleZoo namespace
    using namespace ParticleZoo;

    // Define constants
    constexpr int SUCCESS_CODE = 0;
    constexpr int ERROR_CODE = 1;
    constexpr int MINUMUM_REQUIRED_POSITIONAL_ARGS = 2;
    constexpr std::uint64_t MAX_PERCENTAGE = 100;

    // Register custom command line arguments
    ArgParser::RegisterCommands({
        INPUT_FORMAT_COMMAND,
        OUTPUT_FORMAT_COMMAND,
        PLANE_COMMAND,
        PLANE_LOCATION_COMMAND,
        PROJECT_TO_COMMAND,
        PROJECTION_TYPE_COMMAND,
        IMAGE_WIDTH_COMMAND,
        IMAGE_HEIGHT_COMMAND,
        MIN_X_COMMAND,
        MAX_X_COMMAND,
        MIN_Y_COMMAND,
        MAX_Y_COMMAND,
        MIN_Z_COMMAND,
        MAX_Z_COMMAND,
        SQUARE_COMMAND,
        TOLERANCE_COMMAND,
        MAX_PARTICLES_COMMAND,
        ENERGY_WEIGHTED_COMMAND,
        QUANTITY_TYPE_COMMAND,
        PRIMARIES_ONLY_COMMAND,
        EXCLUDE_PRIMARIES_COMMAND,
        NORMALIZE_BY_PARTICLES_COMMAND,
        SHOW_DETAILS_COMMAND
    });
    
    // Define usage message and parse command line arguments
    auto userOptions = ArgParser::ParseArgs(argc, argv, usageMessage, MINUMUM_REQUIRED_POSITIONAL_ARGS);
    const AppConfig config(userOptions);

    // Create the reader for the input file
    std::unique_ptr<PhaseSpaceFileReader> reader;
    if (config.inputFormat.empty()) {
        reader = FormatRegistry::CreateReader(config.inputFile, userOptions);
    } else {
        reader = FormatRegistry::CreateReader(config.inputFormat, config.inputFile, userOptions);
    }

    // Keep a list of errors and warnings encountered during processing
    std::vector<std::string> errorMessages;
    std::vector<std::string> warningMessages;

    if (config.printDetails) {
        config.details(reader->getPHSPFormat());
    }

    bool generationDetectionFailed = false;

    // Error handling for both reader and writer
    try {
        // Start the process
        std::cout << "Counting particles from " 
                  << config.inputFile << " (" << reader->getPHSPFormat() << ") to store in image "
                  << config.outputFile << "..." << std::endl;

        // Determine how many particles to read - capping out at maxParticles if a limit has been set
        uint64_t particlesInFile = reader->getNumberOfParticles();
        uint64_t particlesToRead = particlesInFile > config.maxParticles ? config.maxParticles : particlesInFile;
        
        // Determine progress update interval
        uint64_t onePercentInterval = particlesToRead >= MAX_PERCENTAGE 
                                    ? particlesToRead / MAX_PERCENTAGE 
                                    : 1;

        // Check if there are particles to read
        if (particlesToRead == 0) {
            throw std::runtime_error("No particles found in the input file.");
        }

        // Calculate pixel mapping
        float xPixelsPerUnitLength = static_cast<float>(config.imageWidth) / (config.maxDim1() - config.minDim1());
        float yPixelsPerUnitLength = static_cast<float>(config.imageHeight) / (config.maxDim2() - config.minDim2());
        float xOffset = static_cast<float>(config.minDim1()) * xPixelsPerUnitLength;
        float yOffset = static_cast<float>(config.minDim2()) * yPixelsPerUnitLength;
        float pixelArea = (config.maxDim1() - config.minDim1()) * (config.maxDim2() - config.minDim2()) / (config.imageWidth * config.imageHeight);
        pixelArea /= cm2; // convert to cm^2

        // Start the timer
        auto start_time = std::chrono::steady_clock::now();

        // Create the image object
        Image<float> * image;
        if (config.outputFormat == TIFF) {
            image = new TiffImage<float>(config.imageWidth, config.imageHeight, xPixelsPerUnitLength, yPixelsPerUnitLength, xOffset, yOffset);
        } else if (config.outputFormat == BMP) {
            image = new BitmapImage<float>(config.imageWidth, config.imageHeight);
        } else {
            throw std::runtime_error("Unsupported output format.");
        }

        // Set up the progress bar for the current file
        Progress<uint64_t> progress(particlesToRead);
        progress.Start("Reading particles:");

        // Read the particles from the input file and build the image data
        while (reader->hasMoreParticles() && reader->getParticlesRead() < particlesToRead) {
            Particle particle = reader->getNextParticle();

            if (particle.getType() == ParticleType::Unsupported) {
                throw std::runtime_error("Encountered unsupported particle type in the input file.");
            }

            if (particle.getType() == ParticleType::PseudoParticle) continue; // Skip pseudo-particles

            // project particle to the scoring plane based on selected projection scheme
            switch (config.projectionType)
            {
                case(ProjectionType::FLATTEN):
                    particle.setZ(config.planeLocation);
                    break;
                case(ProjectionType::PROJECTION):
                    switch (config.plane)
                    {
                        case(XY): particle.projectToZValue(config.planeLocation); break;
                        case(XZ): particle.projectToYValue(config.planeLocation); break;
                        case(YZ): particle.projectToXValue(config.planeLocation); break;
                    };
                    break;
                default:
                    break;
            }
            
            // Get the particle's position
            float x = particle.getX();
            float y = particle.getY();
            float z = particle.getZ();

            // Determine pixel coordinates based on the selected plane
            int pixelX = 0, pixelY = 0;
            bool validPixel = false;
            if (config.plane == XY && std::abs(z - config.planeLocation) <= config.tolerance && x >= config.minDim1() && x <= config.maxDim1() && y >= config.minDim2() && y <= config.maxDim2()) {
                pixelX = static_cast<int>((x - config.minDim1()) / (config.maxDim1() - config.minDim1()) * config.imageWidth);
                pixelY = static_cast<int>((y - config.minDim2()) / (config.maxDim2() - config.minDim2()) * config.imageHeight);
                validPixel = true;
            } else if (config.plane == XZ && std::abs(y - config.planeLocation) <= config.tolerance && x >= config.minDim1() && x <= config.maxDim1() && z >= config.minDim2() && z <= config.maxDim2()) {
                pixelX = static_cast<int>((x - config.minDim1()) / (config.maxDim1() - config.minDim1()) * config.imageWidth);
                pixelY = static_cast<int>((z - config.minDim2()) / (config.maxDim2() - config.minDim2()) * config.imageHeight);
                validPixel = true;
            } else if (config.plane == YZ && std::abs(x - config.planeLocation) <= config.tolerance && y >= config.minDim1() && y <= config.maxDim1() && z >= config.minDim2() && z <= config.maxDim2()) {
                pixelX = static_cast<int>((y - config.minDim1()) / (config.maxDim1() - config.minDim1()) * config.imageWidth);
                pixelY = static_cast<int>((z - config.minDim2()) / (config.maxDim2() - config.minDim2()) * config.imageHeight);
                validPixel = true;
            }
            
            // Process the particle if it falls within the image boundaries
            validPixel = validPixel && (pixelX >= 0 && pixelX < config.imageWidth && pixelY >= 0 && pixelY < config.imageHeight);

            // Check if the particle is a included based on generation type
            if (config.generationType != GenerationType::ALL) {
                if (particle.hasBoolProperty(BoolPropertyType::IS_SECONDARY_PARTICLE)) {
                    bool isPrimary = particle.getBoolProperty(BoolPropertyType::IS_SECONDARY_PARTICLE) ? false : true;
                    if (config.generationType == GenerationType::PRIMARIES_ONLY && !isPrimary) {
                        validPixel = false;
                    } else if (config.generationType == GenerationType::EXCLUDE_PRIMARIES && isPrimary) {
                        validPixel = false;
                    }
                } else if (!generationDetectionFailed) {
                    warningMessages.push_back("Could not determine particle generation (primary/secondary) from the phase space file. Generation-based filtering was not applied.");
                    generationDetectionFailed = true;
                }
            }

            if (validPixel) {
                // Determine the weight to associate with this particle
                float weight = particle.getWeight();
                switch (config.quantityType) {
                    case QuantityType::ENERGY_FLUENCE:
                        weight *= particle.getKineticEnergy() / MeV;
                        break;
                    case QuantityType::X_DIRECTIONAL_COSINE:
                        weight *= particle.getDirectionalCosineX();
                        break;
                    case QuantityType::Y_DIRECTIONAL_COSINE:
                        weight *= particle.getDirectionalCosineY();
                        break;
                    case QuantityType::Z_DIRECTIONAL_COSINE:
                        weight *= particle.getDirectionalCosineZ();
                        break;
                    case QuantityType::PARTICLE_FLUENCE:
                    default:
                        // weight remains unchanged
                        break;
                }

                // Set pixel color based on the particle's weight
                float weightPerUnitArea = weight / pixelArea; // counts per cm2 or MeV per cm2
                float pixelValue = image->getGrayscaleValue(pixelX, pixelY) + weightPerUnitArea;
                image->setGrayscaleValue(pixelX, pixelY, pixelValue);
            }

            std::uint64_t particlesSoFar = reader->getParticlesRead();
            // Update progress bar every 1% of particles read
            if (particlesSoFar % onePercentInterval == 0) {
                progress.Update(particlesSoFar, "Processed " + std::to_string(reader->getHistoriesRead()) + " histories.");
            }
        }

        // Finalize the image by normalizing the data by the number of histories (or particles if specified by the user)
        uint64_t numberOfHistories = reader->getNumberOfOriginalHistories();
        uint64_t particlesRead = reader->getParticlesRead();
        std::uint64_t historiesRead = particlesRead < particlesInFile ? reader->getHistoriesRead() : numberOfHistories;
        if (config.normalizeByParticles) {
            image->normalize(static_cast<float>(particlesRead));
        } else {
            image->normalize(static_cast<float>(historiesRead));
        }

        // Save the image to the output file
        image->save(config.outputFile);

        // Complete the progress bar
        progress.Complete("Image generation complete. Processed " + std::to_string(historiesRead) + " histories.");

        if (config.normalizeByParticles) {
            std::cout << "Image normalized by particles (" << particlesRead << " particles read)." << std::endl;
        } else {
            std::cout << "Image normalized by histories (" << historiesRead << " histories read)." << std::endl;
        }

        // Clean up resources
        delete image;

        // Measure elapsed time and report it
        auto end_time = std::chrono::steady_clock::now();
        double elapsed = std::chrono::duration<double>(end_time - start_time).count();
        std::cout << "Time taken: " << elapsed << " seconds" << std::endl;

    } catch (const std::exception& e) {
        errorMessages.push_back(std::string(e.what()));
    }

    // Ensure that the reader is closed even if an exception occurs
    try { if (reader) reader->close(); } catch (const std::exception& e) { errorMessages.push_back("Error closing reader: " + std::string(e.what())); }

    // Output any error messages
    for (const auto& error : errorMessages) {
        std::cerr << "Error: " << error << std::endl;
    }

    // Output any warning messages
    for (const auto& warning : warningMessages) {
        std::cerr << "Warning: " << warning << std::endl;
    }

    // Return appropriate error code
    int errorCode = (!errorMessages.empty()
                        || (config.errorOnWarning && !warningMessages.empty()))
                        ? ERROR_CODE : SUCCESS_CODE;
    return errorCode;
}