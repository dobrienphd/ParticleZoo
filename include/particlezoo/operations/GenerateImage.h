#pragma once

#include <string>
#include <optional>
#include <utility>
#include <cstdint>
#include <limits>
#include "particlezoo/utilities/argParse.h"
#include "particlezoo/utilities/units.h"

namespace ParticleZoo {

/** @brief Imaging plane orientation for GenerateImage. */
enum class ImagePlane {
    XY, ///< View from the Z axis (horizontal slice)
    XZ, ///< View from the Y axis
    YZ  ///< View from the X axis
};

/** @brief How particles are projected onto the imaging plane. */
enum class ImageProjectionType {
    FLATTEN,  ///< Force all particle coordinates perpendicular to the plane to the plane location
    PROJECT,  ///< Project each particle along its direction of travel to the plane
    NONE      ///< Only score particles already within tolerance of the plane location
};

/** @brief Physical quantity accumulated in each image pixel. */
enum class ImageQuantityType {
    COUNT,   ///< Particle fluence (weighted particle count per unit area)
    ENERGY,  ///< Energy fluence (weighted energy per unit area)
    X_DIR,   ///< Mean X directional cosine (weighted)
    Y_DIR,   ///< Mean Y directional cosine (weighted)
    Z_DIR    ///< Mean Z directional cosine (weighted)
};

/** @brief Output image file format. */
enum class ImageOutputFormat {
    TIFF, ///< 32-bit floating-point TIFF with spatial calibration metadata (default)
    BMP   ///< 8-bit BMP with automatic window-level contrast adjustment
};

/**
 * @brief Options for the GenerateImage operation.
 *
 * All fields have sensible defaults. File paths are passed as positional
 * parameters to GenerateImage(). Numeric values that represent physical
 * quantities use the internal unit system — multiply by the constants in
 * particlezoo/utilities/units.h (e.g. @c 5.0f * cm).
 */
struct GenerateImageOptions {
    // --- Default values (single source of truth, shared with the CLI apps) ---
    static constexpr float    DEFAULT_DISTANCE       = 40.0f * cm; ///< Default half-extent of the imaging region
    static constexpr float    DEFAULT_TOLERANCE      = 0.25f * cm; ///< Default perpendicular scoring tolerance
    static constexpr int      DEFAULT_IMAGE_SIDE     = 1024;       ///< Default image width and height in pixels
    static constexpr float    DEFAULT_PLANE_LOCATION = 0.0f * cm;  ///< Default location of the imaging plane
    static constexpr uint64_t DEFAULT_MAX_PARTICLES  = std::numeric_limits<uint32_t>::max(); ///< Default particle limit (effectively unlimited)

    // --- Plane geometry ---
    ImagePlane plane                       = ImagePlane::XY;               ///< Imaging plane orientation
    float      planeLocation               = DEFAULT_PLANE_LOCATION;       ///< Location of the imaging plane (internal units, use * cm)
    ImageProjectionType projectionType     = ImageProjectionType::FLATTEN; ///< Particle projection scheme
    std::optional<float> projectTo;                                        ///< If set, project particles to this location and override projectionType to PROJECT (internal units)

    // --- Image dimensions ---
    int imageWidth  = DEFAULT_IMAGE_SIDE; ///< Output image width in pixels
    int imageHeight = DEFAULT_IMAGE_SIDE; ///< Output image height in pixels

    // --- Spatial bounds (internal units, use * cm); nullopt → ±DEFAULT_DISTANCE ---
    std::optional<float> minX; ///< Minimum X coordinate of the imaging region
    std::optional<float> maxX; ///< Maximum X coordinate of the imaging region
    std::optional<float> minY; ///< Minimum Y coordinate of the imaging region
    std::optional<float> maxY; ///< Maximum Y coordinate of the imaging region
    std::optional<float> minZ; ///< Minimum Z coordinate of the imaging region
    std::optional<float> maxZ; ///< Maximum Z coordinate of the imaging region
    std::optional<float> square; ///< If set, use a square region of this side length centred at the origin (overrides min/max for both in-plane axes)

    float tolerance = DEFAULT_TOLERANCE; ///< Half-thickness of the scoring slab in the direction perpendicular to the plane (used when projectionType == NONE; internal units)

    // --- Processing ---
    uint64_t maxParticles = DEFAULT_MAX_PARTICLES; ///< Maximum particles to process (default: unlimited)
    ImageQuantityType score = ImageQuantityType::COUNT;           ///< Quantity to accumulate per pixel
    bool energyWeighted     = false;                              ///< Convenience flag: sets score to ENERGY
    bool normalizeByParticles = false;                            ///< Normalise by particle count instead of history count

    // --- I/O ---
    std::string inputFormat             = "";                     ///< Force input format; empty = auto-detect
    ImageOutputFormat outputFormat      = ImageOutputFormat::TIFF; ///< Output image format
    UserOptions formatOptions{};                                  ///< Format-specific reader options (e.g. EGS LATCH option, IAEA options)

    // --- Generation filter (at most one may be set) ---
    bool primariesOnly    = false;                                ///< Score only primary particles
    bool excludePrimaries = false;                                ///< Exclude primary particles
    std::optional<std::pair<int,int>> generations;                ///< Score particles whose generation is in [first, second]

    // --- EGS-specific ---
    bool     useLATCHFilter = false; ///< Apply an EGS LATCH bitmask filter
    uint32_t LATCHFilter    = 0;     ///< LATCH bitmask (used when useLATCHFilter is true)

    bool errorOnWarning = false; ///< Throw an exception for warnings instead of printing them
    bool showDetails    = false; ///< Print detailed parameter information before processing
};

/**
 * @brief Generate a 2D fluence image from a phase space file.
 *
 * Reads particles from @p inputFile, projects them onto the specified plane,
 * accumulates the selected quantity into a pixel grid, normalises by history
 * (or particle) count, and saves the result to @p outputFile.
 *
 * @param inputFile   Input phase space file path.
 * @param outputFile  Output image file path (.tiff or .bmp).
 * @param options     GenerateImage options (default: all defaults).
 * @throws std::runtime_error on invalid parameters or I/O errors.
 */
void GenerateImage(const std::string& inputFile,
                   const std::string& outputFile,
                   const GenerateImageOptions& options = GenerateImageOptions{});

} // namespace ParticleZoo
