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


    enum class IntPropertyType {
        INVALID,
        INCREMENTAL_HISTORY_NUMBER,
        EGS_LATCH,
        PENELOPE_ILB1,
        PENELOPE_ILB2,
        PENELOPE_ILB3,
        PENELOPE_ILB4,
        PENELOPE_ILB5,
        CUSTOM
    };

    enum class FloatPropertyType {
        INVALID,
        XLAST,
        YLAST,
        ZLAST,
        CUSTOM
    };

    enum class BoolPropertyType {
        INVALID,
        IS_MULTIPLE_CROSSER,
        IS_SECONDARY_PARTICLE,
        CUSTOM
    };


    /* Particle Class Definition */


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
            Particle();
            Particle(ParticleType type, float kineticEnergy, float x, float y, float z, float px, float py, float pz, bool isNewHistory = true, float weight = 1.0);

            // Getters and setters for basic particle properties

            void setKineticEnergy(float energy);
            void setX(float x);
            void setY(float y);
            void setZ(float z);
            void setPx(float px);
            void setPy(float py);
            void setPz(float pz);
            void setWeight(float weight);
            void setNewHistory(bool isNewHistory);

            ParticleType getType() const;
            float getKineticEnergy() const;
            float getX() const;
            float getY() const;
            float getZ() const;
            float getPx() const;
            float getPy() const;
            float getPz() const;
            float getWeight() const;
            bool  isNewHistory() const;

            // Setters and getters for advanced particle properties

            void reserveBoolProperties(unsigned int size);
            void reserveFloatProperties(unsigned int size);
            void reserveIntProperties(unsigned int size);

            int getNumberOfBoolProperties() const;
            int getNumberOfFloatProperties() const;
            int getNumberOfIntProperties() const;

            bool hasBoolProperty(BoolPropertyType type) const;
            bool hasFloatProperty(FloatPropertyType type) const;
            bool hasIntProperty(IntPropertyType type) const;

            std::int32_t getIntProperty(IntPropertyType type) const;
            float getFloatProperty(FloatPropertyType type) const;
            bool getBoolProperty(BoolPropertyType type) const;

            void setBoolProperty(BoolPropertyType type, bool value);
            void setFloatProperty(FloatPropertyType type, float value);
            void setIntProperty(IntPropertyType type, std::int32_t value);
            void setStringProperty(std::string value);

            const std::vector<bool>& getCustomBoolProperties() const;
            const std::vector<float>& getCustomFloatProperties() const;
            const std::vector<std::int32_t>& getCustomIntProperties() const;
            const std::vector<std::string>& getCustomStringProperties() const;

        private:

            const ParticleType type_;
            float kineticEnergy_;
            float x_;
            float y_;
            float z_;
            float px_;
            float py_;
            float pz_;
            bool isNewHistory_;
            float weight_;
            ParticleProperties properties_;
            
            int  getBoolPropertyIndex(BoolPropertyType type) const;
            int  getFloatPropertyIndex(FloatPropertyType type) const;
            int  getIntPropertyIndex(IntPropertyType type) const;
            void normalizeDirectionalCosines();
    };


    /* Implementation of Particle class methods */

    inline Particle::Particle()
    :Particle(ParticleType::Unsupported, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, false, 1.f)
    {}

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
    inline void Particle::setPx(float px) { px_ = px; }
    inline void Particle::setPy(float py) { py_ = py; }
    inline void Particle::setPz(float pz) { pz_ = pz; }
    inline void Particle::setWeight(float weight) { weight_ = weight; }
    inline void Particle::setNewHistory(bool isNewHistory) { isNewHistory_ = isNewHistory; }

    inline ParticleType Particle::getType() const { return type_; }
    inline float Particle::getKineticEnergy() const { return kineticEnergy_; }
    inline float Particle::getX() const { return x_; }
    inline float Particle::getY() const { return y_; }
    inline float Particle::getZ() const { return z_; }
    inline float Particle::getPx() const { return px_; }
    inline float Particle::getPy() const { return py_; }
    inline float Particle::getPz() const { return pz_; }
    inline float Particle::getWeight() const { return weight_; }
    inline bool  Particle::isNewHistory() const { return isNewHistory_; }

    inline int Particle::getNumberOfBoolProperties() const { return properties_.boolProperties.size(); }
    inline int Particle::getNumberOfFloatProperties() const { return properties_.floatProperties.size(); }
    inline int Particle::getNumberOfIntProperties() const { return properties_.intProperties.size(); }

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
                properties_.boolPropertyTypeIndices[type] = properties_.boolProperties.size() - 1;
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
                properties_.floatPropertyTypeIndices[type] = properties_.floatProperties.size() - 1;
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
                properties_.intPropertyTypeIndices[type] = properties_.intProperties.size() - 1;
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

} // namespace ParticleZoo