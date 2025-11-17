#pragma once

#include <unordered_map>
#include <string>
#include <cmath>
#include <stdexcept>
#include <vector>
#include <cstdint>
#include <string_view>

#include "particlezoo/utilities/units.h"
#include "particlezoo/PDGParticleCodes.h"

namespace ParticleZoo {


    /* Particle Property Types */

    /**
     * @brief Enumeration of integer property types for particles.
     * 
     * Defines standardized integer properties that can be associated with particles
     * from different Monte Carlo simulation codes.
     */
    enum class IntPropertyType {
        INVALID,                        ///< Invalid property type, used for error checking
        INCREMENTAL_HISTORY_NUMBER,     ///< Sequential history number for tracking, tracks the number of new histories since the last particle was recorded
        EGS_LATCH,                      ///< EGS-specific latch variable (see BEAMnrc User Manual, Chapter 8 for details)
        PENELOPE_ILB1,                  ///< PENELOPE ILB array value 1, corresponds to the generation of the particle (1 for primary, 2 for secondary, etc.)
        PENELOPE_ILB2,                  ///< PENELOPE ILB array value 2, corresponds to the particle type of the particle's parent (applies only if ILB1 > 1)
        PENELOPE_ILB3,                  ///< PENELOPE ILB array value 3, corresponds to the interaction type that created the particle (applies only if ILB1 > 1)
        PENELOPE_ILB4,                  ///< PENELOPE ILB array value 4, is non-zero if the particle is created by atomic relaxation and corresponds to the atomic transistion that created the particle
        PENELOPE_ILB5,                  ///< PENELOPE ILB array value 5, a user-defined value which is passed on to all descendant particles created by this particle
        CUSTOM                          ///< Custom integer property type, can be used for any user-defined purpose
    };

    /**
     * @brief Enumeration of floating-point property types for particles.
     * 
     * Defines standardized float properties that can be associated with particles
     * from different Monte Carlo simulation codes.
     */
    enum class FloatPropertyType {
        INVALID,    ///< Invalid property type, used for error checking
        XLAST,      ///< EGS-specific XLAST variable, for photons it is the X position of the last interaction, for electrons/positrons it is the X position it (or it's ancestor) was created at by a photon
        YLAST,      ///< EGS-specific YLAST variable, for photons it is the Y position of the last interaction, for electrons/positrons it is the Y position it (or it's ancestor) was created at by a photon
        ZLAST,      ///< EGS-specific ZLAST variable, for photons it is the Z position of the last interaction, for electrons/positrons it is the Z position it (or it's ancestor) was created at by a photon 
        CUSTOM      ///< Custom float property type, can be used for any user-defined purpose
    };

    /**
     * @brief Enumeration of boolean property types for particles.
     * 
     * Defines standardized boolean flags that can be associated with particles
     * from different Monte Carlo simulation codes.
     */
    enum class BoolPropertyType {
        INVALID,                ///< Invalid property type
        IS_MULTIPLE_CROSSER,    ///< Flag indicating that the particle crossed the phase space plane multiple times (assuming the phase space is planar)
        IS_SECONDARY_PARTICLE,  ///< Flag indicating that the particle is a secondary
        CUSTOM                  ///< Custom boolean property type, can be used for any user-defined purpose
    };

    /**
     * @brief Structure defining constant (fixed) values for particle properties.
     * 
     * Used to optimize phase space files by storing constant values once rather
     * than repeating them for every particle. Useful when all particles share
     * certain properties (e.g., all particles start from the same position).
     */
    struct FixedValues
    {
        bool xIsConstant{false};        ///< True if X coordinate is constant for all particles
        bool yIsConstant{false};        ///< True if Y coordinate is constant for all particles
        bool zIsConstant{false};        ///< True if Z coordinate is constant for all particles
        bool pxIsConstant{false};       ///< True if X directional cosine is constant for all particles
        bool pyIsConstant{false};       ///< True if Y directional cosine is constant for all particles
        bool pzIsConstant{false};       ///< True if Z directional cosine is constant for all particles
        bool weightIsConstant{false};   ///< True if statistical weight is constant for all particles
        float constantX{0};             ///< Constant X coordinate value (when xIsConstant is true)
        float constantY{0};             ///< Constant Y coordinate value (when yIsConstant is true)
        float constantZ{0};             ///< Constant Z coordinate value (when zIsConstant is true)
        float constantPx{0};            ///< Constant X directional cosine value (when pxIsConstant is true)
        float constantPy{0};            ///< Constant Y directional cosine value (when pyIsConstant is true)
        float constantPz{0};            ///< Constant Z directional cosine value (when pzIsConstant is true)
        float constantWeight{1};        ///< Constant statistical weight value (when weightIsConstant is true)

        /**
         * @brief Equality comparison operator for FixedValues.
         * 
         * @param other The other FixedValues object to compare with
         * @return true if all members are equal
         * @return false if any members differ
         */
        bool operator==(const FixedValues& other) const {
            return xIsConstant == other.xIsConstant &&
                yIsConstant == other.yIsConstant &&
                zIsConstant == other.zIsConstant &&
                pxIsConstant == other.pxIsConstant &&
                pyIsConstant == other.pyIsConstant &&
                pzIsConstant == other.pzIsConstant &&
                weightIsConstant == other.weightIsConstant &&
                constantX == other.constantX &&
                constantY == other.constantY &&
                constantZ == other.constantZ &&
                constantPx == other.constantPx &&
                constantPy == other.constantPy &&
                constantPz == other.constantPz &&
                constantWeight == other.constantWeight;
        }
    };


    /* Particle Class Definition */

    /**
     * @brief Represents a particle in phase space.
     * 
     * The Particle class encapsulates all the information about a single particle
     * including its position, momentum direction, kinetic energy, statistical weight,
     * and additional properties specific to different simulation codes. It provides
     * methods for manipulating particle properties, projecting particle trajectories,
     * and storing format-specific metadata.
     */
    class Particle {

        struct ParticleProperties {
            // well defined properties
            std::unordered_map<IntPropertyType, unsigned int> intPropertyTypeIndices;
            std::unordered_map<FloatPropertyType, unsigned int> floatPropertyTypeIndices;
            std::unordered_map<BoolPropertyType, unsigned int> boolPropertyTypeIndices;
            std::vector<BoolPropertyType>   boolPropertyTypes;
            std::vector<FloatPropertyType>  floatPropertyTypes;
            std::vector<IntPropertyType>    intPropertyTypes;
            std::vector<bool>               boolProperties;
            std::vector<float>              floatProperties;
            std::vector<std::int32_t>       intProperties;

            // custom properties
            std::vector<bool>               customBoolProperties;
            std::vector<float>              customFloatProperties;
            std::vector<std::int32_t>       customIntProperties;
            std::vector<std::string>        customStringProperties;
        };

        public:
            /**
             * @brief Default constructor for Particle.
             * 
             * Creates a particle with default values (unsupported type, zero energy, etc.).
             */
            Particle() = default;
            
            /**
             * @brief Construct a Particle with specified properties.
             * 
             * Creates a particle with the given position, momentum direction, energy, and other properties.
             * The directional cosines are automatically normalized to ensure they represent a unit vector.
             * 
             * @param type The particle type (electron, photon, proton, etc.)
             * @param kineticEnergy The kinetic energy of the particle
             * @param x The X coordinate position
             * @param y The Y coordinate position  
             * @param z The Z coordinate position
             * @param directionalCosineX The X component of the momentum unit vector
             * @param directionalCosineY The Y component of the momentum unit vector
             * @param directionalCosineZ The Z component of the momentum unit vector
             * @param isNewHistory Whether this particle starts a new Monte Carlo history (default: true)
             * @param weight The statistical weight of the particle (default: 1.0)
             */
            Particle(ParticleType type, float kineticEnergy, float x, float y, float z, float directionalCosineX, float directionalCosineY, float directionalCosineZ, bool isNewHistory = true, float weight = 1.0f);

            /**
             * @brief Construct a Particle with specified properties.
             * 
             * Creates a particle with the given position, momentum direction, energy, and other properties.
             * The directional cosines are automatically normalized to ensure they represent a unit vector.
             * 
             * This overload accepts double precision inputs and converts them to float internally.
             * 
             * @param type The particle type (electron, photon, proton, etc.)
             * @param kineticEnergy The kinetic energy of the particle
             * @param x The X coordinate position
             * @param y The Y coordinate position  
             * @param z The Z coordinate position
             * @param directionalCosineX The X component of the momentum unit vector
             * @param directionalCosineY The Y component of the momentum unit vector
             * @param directionalCosineZ The Z component of the momentum unit vector
             * @param isNewHistory Whether this particle starts a new Monte Carlo history (default: true)
             * @param weight The statistical weight of the particle (default: 1.0)
             */
            Particle(ParticleType type, double kineticEnergy, double x, double y, double z, double directionalCosineX, double directionalCosineY, double directionalCosineZ, bool isNewHistory = true, double weight = 1.0);

            // Getters and setters for basic particle properties

            /**
             * @brief Set the kinetic energy of the particle.
             * 
             * @param energy The kinetic energy value to set
             */
            void setKineticEnergy(float energy);
            
            /**
             * @brief Set the X coordinate position of the particle.
             * 
             * @param x The X coordinate value to set
             */
            void setX(float x);
            
            /**
             * @brief Set the Y coordinate position of the particle.
             * 
             * @param y The Y coordinate value to set
             */
            void setY(float y);
            
            /**
             * @brief Set the Z coordinate position of the particle.
             * 
             * @param z The Z coordinate value to set
             */
            void setZ(float z);
            
            /**
             * @brief Set the X component of the directional cosine (momentum unit vector).
             * 
             * @param px The X component of the directional cosine to set
             */
            void setDirectionalCosineX(float px);
            
            /**
             * @brief Set the Y component of the directional cosine (momentum unit vector).
             * 
             * @param py The Y component of the directional cosine to set
             */
            void setDirectionalCosineY(float py);
            
            /**
             * @brief Set the Z component of the directional cosine (momentum unit vector).
             * 
             * @param pz The Z component of the directional cosine to set
             */
            void setDirectionalCosineZ(float pz);
            
            /**
             * @brief Set the statistical weight of the particle.
             * 
             * @param weight The statistical weight value to set
             */
            void setWeight(float weight);
            
            /**
             * @brief Set whether this particle starts a new Monte Carlo history.
             * 
             * @param isNewHistory True if this particle starts a new history, false otherwise
             */
            void setNewHistory(bool isNewHistory);

            /**
             * @brief Convenience function to set the number of incremental histories using the INCREMENTAL_HISTORY_NUMBER integer property.
             * 
             * @param incrementalHistories The number of incremental histories to set (must be greater than 0)
             */
            void setIncrementalHistories(std::uint32_t incrementalHistories);

            /**
             * @brief Get the particle type.
             * 
             * @return ParticleType The type of particle (electron, photon, proton, etc.)
             */
            ParticleType getType() const;

            /**
             * @brief Get the PDG identification code of the particle.
             * 
             * @return std::int32_t The PDG code corresponding to the particle type
             */
            std::int32_t getPDGCode() const;

            /**
             * @brief Get the kinetic energy of the particle.
             * 
             * @return float The kinetic energy value
             */
            float getKineticEnergy() const;
            
            /**
             * @brief Get the X coordinate position of the particle.
             * 
             * @return float The X coordinate value
             */
            float getX() const;
            
            /**
             * @brief Get the Y coordinate position of the particle.
             * 
             * @return float The Y coordinate value
             */
            float getY() const;
            
            /**
             * @brief Get the Z coordinate position of the particle.
             * 
             * @return float The Z coordinate value
             */
            float getZ() const;
            
            /**
             * @brief Get the X component of the directional cosine (momentum unit vector).
             * 
             * @return float The X component of the directional cosine
             */
            float getDirectionalCosineX() const;
            
            /**
             * @brief Get the Y component of the directional cosine (momentum unit vector).
             * 
             * @return float The Y component of the directional cosine
             */
            float getDirectionalCosineY() const;
            
            /**
             * @brief Get the Z component of the directional cosine (momentum unit vector).
             * 
             * @return float The Z component of the directional cosine
             */
            float getDirectionalCosineZ() const;
            
            /**
             * @brief Get the statistical weight of the particle.
             * 
             * @return float The statistical weight value
             */
            float getWeight() const;
            
            /**
             * @brief Check if this particle starts a new Monte Carlo history.
             * 
             * @return true if this particle starts a new history
             * @return false if this particle continues an existing history
             */
            bool  isNewHistory() const;

            /**
             * @brief Convenience function to get the number of incremental histories regardless of whether the property is set. If the property is not set, it returns 1 if the particle is marked as a new history, otherwise 0.
             * 
             * @return std::uint32_t The number of incremental histories
             */
            std::uint32_t getIncrementalHistories() const;

            // Setters and getters for advanced particle properties

            /**
             * @brief Reserve memory for boolean properties
             * 
             * @param size The number of boolean properties to reserve space for
             */
            void reserveBoolProperties(unsigned int size);
            
            /**
             * @brief Reserve memory for float properties
             * 
             * @param size The number of float properties to reserve space for
             */
            void reserveFloatProperties(unsigned int size);
            
            /**
             * @brief Reserve memory for integer properties
             * 
             * @param size The number of integer properties to reserve space for
             */
            void reserveIntProperties(unsigned int size);

            /**
             * @brief Get the number of boolean properties currently stored.
             * 
             * @return int The number of boolean properties
             */
            int getNumberOfBoolProperties() const;
            
            /**
             * @brief Get the number of float properties currently stored.
             * 
             * @return int The number of float properties
             */
            int getNumberOfFloatProperties() const;
            
            /**
             * @brief Get the number of integer properties currently stored.
             * 
             * @return int The number of integer properties
             */
            int getNumberOfIntProperties() const;

            /**
             * @brief Check if a boolean property of the specified type exists.
             * 
             * @param type The boolean property type to check for
             * @return true if the property exists
             * @return false if the property does not exist
             */
            bool hasBoolProperty(BoolPropertyType type) const;
            
            /**
             * @brief Check if a float property of the specified type exists.
             * 
             * @param type The float property type to check for
             * @return true if the property exists
             * @return false if the property does not exist
             */
            bool hasFloatProperty(FloatPropertyType type) const;
            
            /**
             * @brief Check if an integer property of the specified type exists.
             * 
             * @param type The integer property type to check for
             * @return true if the property exists
             * @return false if the property does not exist
             */
            bool hasIntProperty(IntPropertyType type) const;

            /**
             * @brief Get the value of an integer property.
             * 
             * @param type The integer property type to retrieve
             * @return std::int32_t The value of the integer property
             * @throws std::invalid_argument if the property type is invalid or not found
             */
            std::int32_t getIntProperty(IntPropertyType type) const;
            
            /**
             * @brief Get the value of a float property.
             * 
             * @param type The float property type to retrieve
             * @return float The value of the float property
             * @throws std::invalid_argument if the property type is invalid or not found
             */
            float getFloatProperty(FloatPropertyType type) const;
            
            /**
             * @brief Get the value of a boolean property.
             * 
             * @param type The boolean property type to retrieve
             * @return bool The value of the boolean property
             * @throws std::invalid_argument if the property type is invalid or not found
             */
            bool getBoolProperty(BoolPropertyType type) const;

            /**
             * @brief Set the value of a boolean property.
             * 
             * If the property doesn't exist, it will be created. If it exists, the value will be updated.
             * 
             * @param type The boolean property type to set
             * @param value The value to set for the property
             */
            void setBoolProperty(BoolPropertyType type, bool value);
            
            /**
             * @brief Set the value of a float property.
             * 
             * If the property doesn't exist, it will be created. If it exists, the value will be updated.
             * 
             * @param type The float property type to set
             * @param value The value to set for the property
             */
            void setFloatProperty(FloatPropertyType type, float value);
            
            /**
             * @brief Set the value of an integer property.
             * 
             * If the property doesn't exist, it will be created. If it exists, the value will be updated.
             * 
             * @param type The integer property type to set
             * @param value The value to set for the property
             */
            void setIntProperty(IntPropertyType type, std::int32_t value);
            
            /**
             * @brief Add a custom string property.
             * 
             * Associate a string value with this particle. Multiple string properties can be added.
             * 
             * @param value The string value to add as a property
             */
            void setStringProperty(std::string value);

            /**
             * @brief Get a reference to all custom boolean properties.
             * 
             * @return const std::vector<bool>& Reference to the vector of custom boolean properties
             */
            const std::vector<bool>& getCustomBoolProperties() const;
            
            /**
             * @brief Get a reference to all custom float properties.
             * 
             * @return const std::vector<float>& Reference to the vector of custom float properties
             */
            const std::vector<float>& getCustomFloatProperties() const;
            
            /**
             * @brief Get a reference to all custom integer properties.
             * 
             * @return const std::vector<std::int32_t>& Reference to the vector of custom integer properties
             */
            const std::vector<std::int32_t>& getCustomIntProperties() const;
            
            /**
             * @brief Get a reference to all custom string properties.
             * 
             * @return const std::vector<std::string>& Reference to the vector of custom string properties
             */
            const std::vector<std::string>& getCustomStringProperties() const;

            // Methods for projecting the particle's position

            /**
             * @brief Project the particle's trajectory to a specific X coordinate.
             * 
             * Calculates where the particle would be when it reaches the specified X value,
             * assuming it travels in a straight line. Updates the Y and Z coordinates accordingly.
             * 
             * @param X The target X coordinate to project to
             * @return true if projection was successful
             * @return false if projection is impossible (particle has no movement in X direction)
             */
            bool projectToXValue(float X);
            
            /**
             * @brief Project the particle's trajectory to a specific Y coordinate.
             * 
             * Calculates where the particle would be when it reaches the specified Y value,
             * assuming it travels in a straight line. Updates the X and Z coordinates accordingly.
             * 
             * @param Y The target Y coordinate to project to
             * @return true if projection was successful
             * @return false if projection is impossible (particle has no movement in Y direction)
             */
            bool projectToYValue(float Y);
            
            /**
             * @brief Project the particle's trajectory to a specific Z coordinate.
             * 
             * Calculates where the particle would be when it reaches the specified Z value,
             * assuming it travels in a straight line. Updates the X and Y coordinates accordingly.
             * 
             * @param Z The target Z coordinate to project to
             * @return true if projection was successful
             * @return false if projection is impossible (particle has no movement in Z direction)
             */
            bool projectToZValue(float Z);

        private:

            ParticleType type_{ParticleType::Unsupported};
            float kineticEnergy_{0.f};
            float x_{0.f};
            float y_{0.f};
            float z_{0.f};
            float px_{0.f};
            float py_{0.f};
            float pz_{0.f};
            bool isNewHistory_{false};
            float weight_{0.f};
            ParticleProperties properties_{};
            
            int  getBoolPropertyIndex(BoolPropertyType type) const;
            int  getFloatPropertyIndex(FloatPropertyType type) const;
            int  getIntPropertyIndex(IntPropertyType type) const;
            void normalizeDirectionalCosines();
    };


    /* Implementation of Particle class methods */

    inline Particle::Particle(ParticleType type, double kineticEnergy, double x, double y, double z, double px, double py, double pz, bool isNewHistory, double weight)
    :   type_(type),
        kineticEnergy_(static_cast<float>(kineticEnergy)),
        x_(static_cast<float>(x)),
        y_(static_cast<float>(y)),
        z_(static_cast<float>(z)),
        px_(static_cast<float>(px)),
        py_(static_cast<float>(py)),
        pz_(static_cast<float>(pz)),
        isNewHistory_(isNewHistory),
        weight_(static_cast<float>(weight)),
        properties_
        ()
    {
        // Normalize the directional cosines
        normalizeDirectionalCosines();
    }

    inline Particle::Particle(ParticleType type, float kineticEnergy, float x, float y, float z, float px, float py, float pz, bool isNewHistory, float weight)
    : type_(type), kineticEnergy_(kineticEnergy), x_(x), y_(y), z_(z), px_(px), py_(py), pz_(pz), isNewHistory_(isNewHistory), weight_(weight), properties_()
    {
        // Normalize the directional cosines
        normalizeDirectionalCosines();
    }

    inline void Particle::setKineticEnergy(float energy) { kineticEnergy_ = energy; }
    inline void Particle::setX(float x) { x_ = x; }
    inline void Particle::setY(float y) { y_ = y; }
    inline void Particle::setZ(float z) { z_ = z; }
    inline void Particle::setDirectionalCosineX(float px) { px_ = px; }
    inline void Particle::setDirectionalCosineY(float py) { py_ = py; }
    inline void Particle::setDirectionalCosineZ(float pz) { pz_ = pz; }
    inline void Particle::setWeight(float weight) { weight_ = weight; }
    inline void Particle::setNewHistory(bool isNewHistory) { isNewHistory_ = isNewHistory; }

    inline void Particle::setIncrementalHistories(std::uint32_t incrementalHistories)
    {
        if (incrementalHistories == 0) {
            throw std::invalid_argument("Incremental histories must be greater than 0.");
        }
        isNewHistory_ = true; // Setting incremental histories implies this is a new history
        setIntProperty(IntPropertyType::INCREMENTAL_HISTORY_NUMBER, static_cast<std::int32_t>(incrementalHistories));
    }

    inline ParticleType Particle::getType() const { return type_; }
    inline float Particle::getKineticEnergy() const { return kineticEnergy_; }
    inline float Particle::getX() const { return x_; }
    inline float Particle::getY() const { return y_; }
    inline float Particle::getZ() const { return z_; }
    inline float Particle::getDirectionalCosineX() const { return px_; }
    inline float Particle::getDirectionalCosineY() const { return py_; }
    inline float Particle::getDirectionalCosineZ() const { return pz_; }
    inline float Particle::getWeight() const { return weight_; }
    inline bool  Particle::isNewHistory() const { return isNewHistory_; }

    inline std::uint32_t Particle::getIncrementalHistories() const {
        if (!isNewHistory_) return 0; // If not a new history, return 0
        int index = getIntPropertyIndex(IntPropertyType::INCREMENTAL_HISTORY_NUMBER);
        if (index >= 0) {
            return static_cast<std::uint32_t>(properties_.intProperties[index]); // Return the set property value
        } else {
            return 1; // Default to 1 if property not set
        }
    }

    inline int Particle::getNumberOfBoolProperties() const { return static_cast<int>(properties_.boolProperties.size()); }
    inline int Particle::getNumberOfFloatProperties() const { return static_cast<int>(properties_.floatProperties.size()); }
    inline int Particle::getNumberOfIntProperties() const { return static_cast<int>(properties_.intProperties.size()); }

    inline const std::vector<bool>& Particle::getCustomBoolProperties() const { return properties_.customBoolProperties; }
    inline const std::vector<float>& Particle::getCustomFloatProperties() const { return properties_.customFloatProperties; }
    inline const std::vector<std::int32_t>& Particle::getCustomIntProperties() const { return properties_.customIntProperties; }
    inline const std::vector<std::string>& Particle::getCustomStringProperties() const { return properties_.customStringProperties; }

    inline bool Particle::hasBoolProperty(BoolPropertyType type) const
    {
        auto it = properties_.boolPropertyTypeIndices.find(type);
        if (it != properties_.boolPropertyTypeIndices.end()) {
            return true;
        } else {
            return false; // Not found
        }
    }

    inline bool Particle::hasFloatProperty(FloatPropertyType type) const
    {
        auto it = properties_.floatPropertyTypeIndices.find(type);
        if (it != properties_.floatPropertyTypeIndices.end()) {
            return true;
        } else {
            return false; // Not found
        }
    }

    inline bool Particle::hasIntProperty(IntPropertyType type) const
    {
        auto it = properties_.intPropertyTypeIndices.find(type);
        if (it != properties_.intPropertyTypeIndices.end()) {
            return true;
        } else {
            return false; // Not found
        }
    }

    inline int Particle::getBoolPropertyIndex(BoolPropertyType type) const {
        auto it = properties_.boolPropertyTypeIndices.find(type);
        if (it != properties_.boolPropertyTypeIndices.end()) {
            return it->second;
        } else {
            return -1; // Not found
        }
    }

    inline int Particle::getFloatPropertyIndex(FloatPropertyType type) const {
        auto it = properties_.floatPropertyTypeIndices.find(type);
        if (it != properties_.floatPropertyTypeIndices.end()) {
            return it->second;
        } else {
            return -1; // Not found
        }
    }

    inline int Particle::getIntPropertyIndex(IntPropertyType type) const {
        auto it = properties_.intPropertyTypeIndices.find(type);
        if (it != properties_.intPropertyTypeIndices.end()) {
            return it->second;
        } else {
            return -1; // Not found
        }
    }

    inline std::int32_t Particle::getIntProperty(IntPropertyType type) const {
        int index = getIntPropertyIndex(type);
        if (index == -1) {
            throw std::invalid_argument("Invalid integer property type.");
        }
        return properties_.intProperties[index];
    }

    inline float Particle::getFloatProperty(FloatPropertyType type) const {
        int index = getFloatPropertyIndex(type);
        if (index == -1) {
            throw std::invalid_argument("Invalid float property type.");
        }
        return properties_.floatProperties[index];
    }

    inline bool Particle::getBoolProperty(BoolPropertyType type) const {
        int index = getBoolPropertyIndex(type);
        if (index == -1) {
            throw std::invalid_argument("Invalid boolean property type.");
        }
        return properties_.boolProperties[index];
    }

    inline void Particle::reserveBoolProperties(unsigned int size) {
        properties_.boolProperties.reserve(size);
        properties_.boolPropertyTypes.reserve(size);
    }

    inline void Particle::reserveFloatProperties(unsigned int size) {
        properties_.floatProperties.reserve(size);
        properties_.floatPropertyTypes.reserve(size);
    }

    inline void Particle::reserveIntProperties(unsigned int size) {
        properties_.intProperties.reserve(size);
        properties_.intPropertyTypes.reserve(size);
    }

    inline void Particle::setBoolProperty(BoolPropertyType type, bool value) {
        if (type == BoolPropertyType::INVALID) return;
        if (type != BoolPropertyType::CUSTOM) {
            int index = getBoolPropertyIndex(type);
            if (index == -1) {
                properties_.boolProperties.push_back(value);
                properties_.boolPropertyTypes.push_back(type);
                properties_.boolPropertyTypeIndices[type] = static_cast<unsigned int>(properties_.boolProperties.size() - 1);
            } else {
                properties_.boolProperties[index] = value;
            }
        } else {
            properties_.customBoolProperties.push_back(value);
        }
    }

    inline void Particle::setFloatProperty(FloatPropertyType type, float value) {
        if (type == FloatPropertyType::INVALID) return;
        if (type != FloatPropertyType::CUSTOM) {
            int index = getFloatPropertyIndex(type);
            if (index == -1) {
                properties_.floatProperties.push_back(value);
                properties_.floatPropertyTypes.push_back(type);
                properties_.floatPropertyTypeIndices[type] = static_cast<unsigned int>(properties_.floatProperties.size() - 1);
            } else {
                properties_.floatProperties[index] = value;
            }
        } else {
            properties_.customFloatProperties.push_back(value);
        }
    }

    inline void Particle::setIntProperty(IntPropertyType type, std::int32_t value) {
        if (type == IntPropertyType::INVALID) return;
        if (type != IntPropertyType::CUSTOM) {
            int index = getIntPropertyIndex(type);
            if (index == -1) {
                properties_.intProperties.push_back(value);
                properties_.intPropertyTypes.push_back(type);
                properties_.intPropertyTypeIndices[type] = static_cast<unsigned int>(properties_.intProperties.size() - 1);
            } else {
                properties_.intProperties[index] = value;
            }
        } else {
            properties_.customIntProperties.push_back(value);
        }
    }

    inline void Particle::setStringProperty(std::string value) {
        properties_.customStringProperties.push_back(value);
    }

    inline void Particle::normalizeDirectionalCosines() {
        float magnitude = px_*px_ + py_*py_ + pz_*pz_;
        if (magnitude == 0 || magnitude == 1.f) return;
        magnitude = std::sqrt(magnitude);
        px_ /= magnitude;
        py_ /= magnitude;
        pz_ /= magnitude;
    }

    inline bool Particle::projectToXValue(float X) {
        if (x_ == X) return true; // Already at the desired X
        if (std::fabs(px_) < 1e-6f) return false; // Cannot project if no movement in x
        float t = (X - x_) / px_;
        x_ = X;
        y_ += py_ * t;
        z_ += pz_ * t;
        return true;
    }

    inline bool Particle::projectToYValue(float Y) {
        if (y_ == Y) return true; // Already at the desired Y
        if (std::fabs(py_) < 1e-6f) return false; // Cannot project if no movement in y
        float t = (Y - y_) / py_;
        y_ = Y;
        x_ += px_ * t;
        z_ += pz_ * t;
        return true;
    }

    inline bool Particle::projectToZValue(float Z) {
        if (z_ == Z) return true; // Already at the desired Z
        if (std::fabs(pz_) < 1e-6f) return false; // Cannot project if no movement in z
        float t = (Z - z_) / pz_;
        z_ = Z;
        x_ += px_ * t;
        y_ += py_ * t;
        return true;
    }

} // namespace ParticleZoo