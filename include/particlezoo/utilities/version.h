#pragma once

#include <string>

/**
 * @namespace ParticleZoo
 * @brief Main namespace for the ParticleZoo phase space file processing library.
 * 
 * ParticleZoo is a comprehensive C++ library for reading, writing, and manipulating
 * phase space files from various Monte Carlo radiation transport codes. The library
 * provides a unified interface for working with different file formats while
 * preserving format-specific features and metadata.
 * 
 * The namespace contains all classes, functions, and utilities, including:
 * - Phase space file I/O operations
 * - Particle data structures and property management
 * - Universal interface for reading/writing to/from different phase space formats
 * - Format-specific readers and writers
 * 
 * Supported formats include EGS, IAEA, TOPAS, and others.
 */
namespace ParticleZoo {

    /**
     * @struct Version
     * @brief Version information and metadata for the ParticleZoo library.
     * 
     * This structure provides compile-time constants for version identification
     * and runtime functions for generating version strings. It serves as the
     * authoritative source for all version-related information used throughout
     * the library, documentation generation, and user applications.
     * 
     * The version follows semantic versioning (SemVer) principles with
     * MAJOR.MINOR.PATCH numbering plus development status indicators.
     */
    struct Version {

        /**
         * @brief Official project name.
         * 
         * The canonical name of the ParticleZoo library used in all
         * user-facing output, documentation, and version strings.
         */
        static constexpr const char* PROJECT_NAME   = "ParticleZoo";

        /**
         * @brief Major version number.
         * 
         * Incremented for backwards-incompatible API changes. Applications
         * built against different major versions may require code changes.
         */
        static constexpr const int   MAJOR_VERSION  = 1;

        /**
         * @brief Minor version number.
         * 
         * Incremented for backwards-compatible feature additions. Applications
         * built against the same major version should work with higher minor
         * versions without modification.
         */
        static constexpr const int   MINOR_VERSION  = 1;

        /**
         * @brief Patch version number.
         * 
         * Incremented for backwards-compatible bug fixes and minor improvements
         * that do not add new features. Applications should always be able to
         * upgrade to higher patch versions safely.
         */
        static constexpr const int   PATCH_VERSION  = 2;

        /**
         * @brief Development status indicator.
         * 
         * Indicates the current development status of this version. Only used to indicate 
         * pre-release or special build states.
         */
        static constexpr const char* CAVEAT         = "BETA";

        /**
         * @brief Generate a complete version string for display.
         * 
         * Creates a human-readable version string combining all version
         * components in the standard format: "ProjectName vMAJOR.MINOR.PATCH STATUS"

         * @return Complete version string including project name, version numbers, and status
         */        
        static std::string GetVersionString() {
            if (CAVEAT[0] != '\0') {
                return std::string(PROJECT_NAME) + " v" +
                       std::to_string(MAJOR_VERSION) + "." +
                       std::to_string(MINOR_VERSION) + "." +
                       std::to_string(PATCH_VERSION) + " " +
                       CAVEAT;
            } else {
                return std::string(PROJECT_NAME) + " v" +
                       std::to_string(MAJOR_VERSION) + "." +
                       std::to_string(MINOR_VERSION) + "." +
                       std::to_string(PATCH_VERSION);
            }
        }
    };

} // namespace ParticleZoo