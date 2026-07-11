
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
 *   --planeLocation <value>   Location of the imaging plane in cm (default: 0.0)
 *   --projectTo <value>       Project particles to this plane location in cm
 *   --projectionType <type>   Projection scheme: none, project, or flatten (default: flatten)
 *   --imageWidth <pixels>     Output image width in pixels (default: 1024)
 *   --imageHeight <pixels>    Output image height in pixels (default: 1024)
 *   --minX/maxX/minY/maxY/minZ/maxZ <value>  Spatial bounds in cm (default: +/-40 cm)
 *   --square <value>          Side length of square region for imaging
 *   --tolerance <value>       Tolerance in the perpendicular direction (default: 0.25 cm)
 *   --maxParticles <N>        Limit the maximum number of particles to process (default: all)
 *   --energyWeighted          Score energy fluence instead of particle fluence
 *   --score <type>            Quantity to score: count, energy, xDir, yDir, zDir
 *   --normalizeByParticles    Normalize by particles instead of histories
 *   --inputFormat <format>    Force input file format (default: auto-detect)
 *   --outputFormat <tiff|bmp> Force output image format (default: tiff)
 *   --primariesOnly           Only process primary particles
 *   --excludePrimaries        Exclude primary particles
 *   --generations <min> <max> Filter particles by generation range
 *   --showDetails             Print detailed information about the parameters being used
 *   --errorOnWarning          Treat warnings as errors when returning exit code
 *   --formats                 Display supported input file formats and exit
 *   --help                    Display usage information and exit
 */

#include <iostream>
#include <string>
#include <string_view>

#include "particlezoo/utilities/argParse.h"
#include "particlezoo/utilities/formats.h"
#include "particlezoo/utilities/units.h"
#include "particlezoo/egs/EGSLATCH.h"
#include "particlezoo/operations/GenerateImage.h"


// Anonymous namespace for CLI command definitions
namespace {

    using namespace ParticleZoo;
    using EGSphspFile::EGSLATCHFilterCommand;

    constexpr std::string_view usageMessage =
        "Usage: PHSPImage [OPTIONS] <inputfile> <outputfile>\n"
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

    const CLICommand INPUT_FORMAT_COMMAND        = CLICommand(NONE, "", "inputFormat",        "Force input file format (default: auto-detect from extension)",                                                                                              { CLI_STRING });
    const CLICommand OUTPUT_FORMAT_COMMAND       = CLICommand(NONE, "", "outputFormat",       "Force output image format (tiff or bmp)",                                                                                                                    { CLI_STRING }, { "tiff" });
    const CLICommand PLANE_COMMAND               = CLICommand(NONE, "", "plane",              "Imaging plane orientation (XY, XZ, or YZ)",                                                                                                                  { CLI_STRING }, { "XY" });
    const CLICommand PLANE_LOCATION_COMMAND      = CLICommand(NONE, "", "planeLocation",      "Location of the imaging plane in cm",                                                                                                                        { CLI_FLOAT },  { GenerateImageOptions::DEFAULT_PLANE_LOCATION / cm });
    const CLICommand PROJECT_TO_COMMAND          = CLICommand(NONE, "", "projectTo",          "Project particles to this plane location in cm (enables projection mode)",                                                                                   { CLI_FLOAT });
    const CLICommand PROJECTION_TYPE_COMMAND     = CLICommand(NONE, "", "projectionType",     "Projection scheme: none, project, or flatten",                                                                                                               { CLI_STRING }, { "flatten" });
    const CLICommand IMAGE_WIDTH_COMMAND         = CLICommand(NONE, "", "imageWidth",         "Output image width in pixels",                                                                                                                               { CLI_INT },    { GenerateImageOptions::DEFAULT_IMAGE_SIDE });
    const CLICommand IMAGE_HEIGHT_COMMAND        = CLICommand(NONE, "", "imageHeight",        "Output image height in pixels",                                                                                                                              { CLI_INT },    { GenerateImageOptions::DEFAULT_IMAGE_SIDE });
    const CLICommand MINIMUM_X_COMMAND           = CLICommand(NONE, "", "minX",              "Minimum X coordinate for imaging region in cm (default: -40.0 cm)",                                                                                          { CLI_FLOAT });
    const CLICommand MAXIMUM_X_COMMAND           = CLICommand(NONE, "", "maxX",              "Maximum X coordinate for imaging region in cm (default: 40.0 cm)",                                                                                           { CLI_FLOAT });
    const CLICommand MINIMUM_Y_COMMAND           = CLICommand(NONE, "", "minY",              "Minimum Y coordinate for imaging region in cm (default: -40.0 cm)",                                                                                          { CLI_FLOAT });
    const CLICommand MAXIMUM_Y_COMMAND           = CLICommand(NONE, "", "maxY",              "Maximum Y coordinate for imaging region in cm (default: 40.0 cm)",                                                                                           { CLI_FLOAT });
    const CLICommand MINIMUM_Z_COMMAND           = CLICommand(NONE, "", "minZ",              "Minimum Z coordinate for imaging region in cm (default: -40.0 cm)",                                                                                          { CLI_FLOAT });
    const CLICommand MAXIMUM_Z_COMMAND           = CLICommand(NONE, "", "maxZ",              "Maximum Z coordinate for imaging region in cm (default: 40.0 cm)",                                                                                           { CLI_FLOAT });
    const CLICommand SQUARE_COMMAND              = CLICommand(NONE, "", "square",             "Side length of square region (centered at 0,0) for imaging in cm (overrides min/max for both dimensions)",                                                   { CLI_FLOAT });
    const CLICommand TOLERANCE_COMMAND           = CLICommand(NONE, "", "tolerance",          "Tolerance in the direction perpendicular to the plane in cm",                                                                                               { CLI_FLOAT },  { GenerateImageOptions::DEFAULT_TOLERANCE / cm });
    const CLICommand MAX_PARTICLES_COMMAND       = CLICommand(NONE, "", "maxParticles",       "Maximum number of particles to process (default: unlimited)",                                                                                               { CLI_UINT });
    const CLICommand ENERGY_WEIGHTED_COMMAND     = CLICommand(NONE, "", "energyWeighted",     "Score energy fluence (equivalent to --score energy)",                                                                                                       { CLI_VALUELESS });
    const CLICommand QUANTITY_TYPE_COMMAND       = CLICommand(NONE, "", "score",              "Quantity to score: count, energy, xDir, yDir, zDir",                                                                                                        { CLI_STRING }, { "count" });
    const CLICommand PRIMARIES_ONLY_COMMAND      = CLICommand(NONE, "", "primariesOnly",      "Only process primary particles from the phase space file",                                                                                                   { CLI_VALUELESS });
    const CLICommand EXCLUDE_PRIMARIES_COMMAND   = CLICommand(NONE, "", "excludePrimaries",   "Exclude primary particles from processing",                                                                                                                  { CLI_VALUELESS });
    const CLICommand GENERATION_FILTER_COMMAND   = CLICommand(NONE, "", "generations",        "Filter particles by generation range (min and max)",                                                                                                         { CLI_INT, CLI_INT });
    const CLICommand NORMALIZE_BY_PARTICLES_COMMAND = CLICommand(NONE, "", "normalizeByParticles", "Normalize by particles instead of histories",                                                                                                          { CLI_VALUELESS });
    const CLICommand SHOW_DETAILS_COMMAND        = CLICommand(NONE, "", "showDetails",        "Show detailed info about the parameters being used",                                                                                                         { CLI_VALUELESS });
    const CLICommand ERROR_ON_WARNING_COMMAND    = CLICommand(NONE, "", "errorOnWarning",     "Treat warnings as errors when returning exit code",                                                                                                          { CLI_VALUELESS });

} // anonymous namespace


int main(int argc, char* argv[]) {

    using namespace ParticleZoo;

    constexpr int SUCCESS_CODE = 0;
    constexpr int ERROR_CODE   = 1;
    constexpr int MINIMUM_REQUIRED_POSITIONAL_ARGS = 2;

    ArgParser::RegisterCommands({
        INPUT_FORMAT_COMMAND,
        OUTPUT_FORMAT_COMMAND,
        PLANE_COMMAND,
        PLANE_LOCATION_COMMAND,
        PROJECT_TO_COMMAND,
        PROJECTION_TYPE_COMMAND,
        IMAGE_WIDTH_COMMAND,
        IMAGE_HEIGHT_COMMAND,
        MINIMUM_X_COMMAND,
        MAXIMUM_X_COMMAND,
        MINIMUM_Y_COMMAND,
        MAXIMUM_Y_COMMAND,
        MINIMUM_Z_COMMAND,
        MAXIMUM_Z_COMMAND,
        SQUARE_COMMAND,
        TOLERANCE_COMMAND,
        MAX_PARTICLES_COMMAND,
        ENERGY_WEIGHTED_COMMAND,
        QUANTITY_TYPE_COMMAND,
        PRIMARIES_ONLY_COMMAND,
        EXCLUDE_PRIMARIES_COMMAND,
        GENERATION_FILTER_COMMAND,
        NORMALIZE_BY_PARTICLES_COMMAND,
        SHOW_DETAILS_COMMAND,
        EGSLATCHFilterCommand
    });

    auto userOptions = ArgParser::ParseArgs(argc, argv, usageMessage, MINIMUM_REQUIRED_POSITIONAL_ARGS);

    std::string inputFile  = userOptions.extractPositional(0);
    std::string outputFile = userOptions.extractPositional(1);

    // Build GenerateImageOptions from parsed CLI args
    GenerateImageOptions options;

    options.inputFormat       = userOptions.extractStringOption(INPUT_FORMAT_COMMAND);
    options.normalizeByParticles = userOptions.contains(NORMALIZE_BY_PARTICLES_COMMAND);
    options.showDetails       = userOptions.contains(SHOW_DETAILS_COMMAND);
    options.errorOnWarning    = userOptions.contains(ERROR_ON_WARNING_COMMAND);

    // Image format
    if (userOptions.contains(OUTPUT_FORMAT_COMMAND)) {
        std::string fmt = userOptions.extractStringOption(OUTPUT_FORMAT_COMMAND);
        if (fmt == "bmp" || fmt == "BMP")
            options.outputFormat = ImageOutputFormat::BMP;
        else
            options.outputFormat = ImageOutputFormat::TIFF;
    }

    // Plane
    if (userOptions.contains(PLANE_COMMAND)) {
        std::string p = userOptions.extractStringOption(PLANE_COMMAND);
        if (p == "XZ")      options.plane = ImagePlane::XZ;
        else if (p == "YZ") options.plane = ImagePlane::YZ;
        else                options.plane = ImagePlane::XY;
    }

    // Projection type / projectTo
    if (userOptions.contains(PROJECT_TO_COMMAND)) {
        options.projectTo = userOptions.extractFloatOption(PROJECT_TO_COMMAND) * cm;
    } else if (userOptions.contains(PROJECTION_TYPE_COMMAND)) {
        std::string pt = userOptions.extractStringOption(PROJECTION_TYPE_COMMAND);
        if (pt == "none")         options.projectionType = ImageProjectionType::NONE;
        else if (pt == "project") options.projectionType = ImageProjectionType::PROJECT;
        else                      options.projectionType = ImageProjectionType::FLATTEN;
    }

    // Plane location (only used if no projectTo)
    if (!options.projectTo.has_value() && userOptions.contains(PLANE_LOCATION_COMMAND))
        options.planeLocation = userOptions.extractFloatOption(PLANE_LOCATION_COMMAND) * cm;

    // Image dimensions
    options.imageWidth  = userOptions.extractIntOption(IMAGE_WIDTH_COMMAND,  GenerateImageOptions::DEFAULT_IMAGE_SIDE);
    options.imageHeight = userOptions.extractIntOption(IMAGE_HEIGHT_COMMAND, GenerateImageOptions::DEFAULT_IMAGE_SIDE);

    // Spatial bounds
    if (userOptions.contains(MINIMUM_X_COMMAND)) options.minX = userOptions.extractFloatOption(MINIMUM_X_COMMAND) * cm;
    if (userOptions.contains(MAXIMUM_X_COMMAND)) options.maxX = userOptions.extractFloatOption(MAXIMUM_X_COMMAND) * cm;
    if (userOptions.contains(MINIMUM_Y_COMMAND)) options.minY = userOptions.extractFloatOption(MINIMUM_Y_COMMAND) * cm;
    if (userOptions.contains(MAXIMUM_Y_COMMAND)) options.maxY = userOptions.extractFloatOption(MAXIMUM_Y_COMMAND) * cm;
    if (userOptions.contains(MINIMUM_Z_COMMAND)) options.minZ = userOptions.extractFloatOption(MINIMUM_Z_COMMAND) * cm;
    if (userOptions.contains(MAXIMUM_Z_COMMAND)) options.maxZ = userOptions.extractFloatOption(MAXIMUM_Z_COMMAND) * cm;
    if (userOptions.contains(SQUARE_COMMAND))    options.square = userOptions.extractFloatOption(SQUARE_COMMAND) * cm;

    options.tolerance = userOptions.extractFloatOption(TOLERANCE_COMMAND, GenerateImageOptions::DEFAULT_TOLERANCE / cm) * cm;

    if (userOptions.contains(MAX_PARTICLES_COMMAND))
        options.maxParticles = static_cast<uint64_t>(userOptions.extractUIntOption(MAX_PARTICLES_COMMAND));

    // Score / energyWeighted
    options.energyWeighted = userOptions.contains(ENERGY_WEIGHTED_COMMAND);
    if (userOptions.contains(QUANTITY_TYPE_COMMAND)) {
        std::string q = userOptions.extractStringOption(QUANTITY_TYPE_COMMAND);
        if (q == "energy")       options.score = ImageQuantityType::ENERGY;
        else if (q == "xDir")    options.score = ImageQuantityType::X_DIR;
        else if (q == "yDir")    options.score = ImageQuantityType::Y_DIR;
        else if (q == "zDir")    options.score = ImageQuantityType::Z_DIR;
        else                     options.score = ImageQuantityType::COUNT;
    }

    // Generation filter
    options.primariesOnly    = userOptions.contains(PRIMARIES_ONLY_COMMAND);
    options.excludePrimaries = userOptions.contains(EXCLUDE_PRIMARIES_COMMAND);
    if (userOptions.contains(GENERATION_FILTER_COMMAND)) {
        auto range = userOptions.extractValues(GENERATION_FILTER_COMMAND);
        options.generations = { std::get<int>(range[0]), std::get<int>(range[1]) };
    }

    // LATCH filter
    options.useLATCHFilter = userOptions.contains(EGSLATCHFilterCommand);
    options.LATCHFilter    = userOptions.extractUIntOption(EGSLATCHFilterCommand, 0);

    try {
        GenerateImage(inputFile, outputFile, options);
        return SUCCESS_CODE;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return ERROR_CODE;
    }
}
