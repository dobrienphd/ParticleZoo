#pragma once

#include <string>

namespace ParticleZoo {

    struct Version {
        static constexpr const char* PROJECT_NAME   = "ParticleZoo";
        static constexpr const int   MAJOR_VERSION  = 1;
        static constexpr const int   MINOR_VERSION  = 0;
        static constexpr const int   PATCH_VERSION  = 0;
        static constexpr const char* CAVEAT         = "";

        static std::string GetVersionString() {
            return std::string(PROJECT_NAME) + " v" +
                   std::to_string(MAJOR_VERSION) + "." +
                   std::to_string(MINOR_VERSION) + "." +
                   std::to_string(PATCH_VERSION) + " " +
                   CAVEAT;
        }
    };

} // namespace ParticleZoo