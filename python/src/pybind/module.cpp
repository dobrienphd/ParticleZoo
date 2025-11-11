#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "particlezoo/Particle.h"
#include "particlezoo/PDGParticleCodes.h"
#include "particlezoo/PhaseSpaceFileReader.h"
#include "particlezoo/PhaseSpaceFileWriter.h"
#include "particlezoo/utilities/formats.h"
#include "particlezoo/utilities/argParse.h"
#include "particlezoo/utilities/units.h"

namespace py = pybind11;
using namespace ParticleZoo;

PYBIND11_MODULE(_pz, m) {
    m.doc() = "Python bindings for ParticleZoo core and IAEA reader";

    // Ensure built-in formats are registered before any factory calls
    try { FormatRegistry::RegisterStandardFormats(); } catch (...) { /* idempotent; ignore */ }

    // ===== UserOptions - expose extraction methods =====
    py::class_<UserOptions>(m, "UserOptions",
        "Container for command-line options parsed by ArgParser. "
        "Stores format-specific and general options for phase space file readers and writers. "
        "Use ArgParser.parse_args() to create from command-line arguments, then extract_*() methods to retrieve values.")
        .def(py::init<>(), "Create an empty UserOptions object")
        .def("extract_positional", &UserOptions::extractPositional, 
             py::arg("index"),
             "Extract a positional argument by index")
        .def("extract_string_option", &UserOptions::extractStringOption,
             py::arg("cmd"), py::arg("index") = 0,
             "Extract a string option value from a command")
        .def("extract_int_option",
             [](const UserOptions& self, const CLICommand& cmd, py::object defaultValue, int index) {
                 if (defaultValue.is_none()) {
                     return self.extractIntOption(cmd, std::nullopt, index);
                 } else {
                     return self.extractIntOption(cmd, py::cast<int>(defaultValue), index);
                 }
             },
             py::arg("cmd"), py::arg("default_value") = py::none(), py::arg("index") = 0,
             "Extract an integer option value from a command")
        .def("extract_uint_option",
             [](const UserOptions& self, const CLICommand& cmd, py::object defaultValue, int index) {
                 if (defaultValue.is_none()) {
                     return self.extractUIntOption(cmd, std::nullopt, index);
                 } else {
                     return self.extractUIntOption(cmd, py::cast<unsigned int>(defaultValue), index);
                 }
             },
             py::arg("cmd"), py::arg("default_value") = py::none(), py::arg("index") = 0,
             "Extract an unsigned integer option value from a command")
        .def("extract_float_option",
             [](const UserOptions& self, const CLICommand& cmd, py::object defaultValue, int index) {
                 if (defaultValue.is_none()) {
                     return self.extractFloatOption(cmd, std::nullopt, index);
                 } else {
                     return self.extractFloatOption(cmd, py::cast<float>(defaultValue), index);
                 }
             },
             py::arg("cmd"), py::arg("default_value") = py::none(), py::arg("index") = 0,
             "Extract a float option value from a command")
        .def("extract_bool_option",
             [](const UserOptions& self, const CLICommand& cmd, py::object defaultValue, int index) {
                 if (defaultValue.is_none()) {
                     return self.extractBoolOption(cmd, std::nullopt, index);
                 } else {
                     return self.extractBoolOption(cmd, py::cast<bool>(defaultValue), index);
                 }
             },
             py::arg("cmd"), py::arg("default_value") = py::none(), py::arg("index") = 0,
             "Extract a boolean option value from a command");

    // ===== ArgParser - expose ParseArgs =====
    py::class_<ArgParser>(m, "ArgParser",
        "Singleton class for parsing command line arguments. Provides a centralized system for "
        "registering command line argument definitions and parsing command line input. "
        "Designed to provide a consistent interface for command line parsing across ParticleZoo tools.")
        .def_static("parse_args", 
            [](const std::vector<std::string>& args, const std::string& usage, size_t min_positional) {
                // Convert Python list to argc/argv style
                // C++ ParseArgs expects argv[0] to be the program name, so prepend a dummy
                std::vector<std::string> argv_strings;
                argv_strings.push_back("python_script");  // dummy program name
                argv_strings.insert(argv_strings.end(), args.begin(), args.end());
                
                std::vector<char*> argv;
                for (auto& arg : argv_strings) {
                    argv.push_back(const_cast<char*>(arg.c_str()));
                }
                return ArgParser::ParseArgs(static_cast<int>(argv.size()), argv.data(), usage, min_positional);
            },
            py::arg("args"), py::arg("usage_message") = "", py::arg("min_positional_args") = 0,
            "Parse command line arguments and return UserOptions. "
            "Args should be a list of command line argument strings. "
            "Returns UserOptions object containing parsed arguments.");

    // ParticleType enum (full mapping from X-macro via helper in header)
    auto pyParticleType = py::enum_<ParticleType>(m, "ParticleType");
    for (const auto &entry : getAllParticleTypes()) {
        pyParticleType.value(std::string(entry.first).c_str(), entry.second);
    }
    // ensure enum is closed/usable
    pyParticleType.export_values();

    // Convenience: return all particle types as a dict name -> enum value
    m.def("all_particle_types", [](){
        py::dict d;
        for (const auto &entry : getAllParticleTypes()) {
            d[py::str(entry.first)] = entry.second;
        }
        return d;
    }, "Return a mapping of ParticleType names to enum values");

    // Property enums
    py::enum_<IntPropertyType>(m, "IntPropertyType",
        "Enumeration of integer property types for particles. "
        "Defines standardized integer properties that can be associated with particles from different Monte Carlo simulation codes.")
        .value("INVALID", IntPropertyType::INVALID,
               "Invalid property type, used for error checking")
        .value("INCREMENTAL_HISTORY_NUMBER", IntPropertyType::INCREMENTAL_HISTORY_NUMBER,
               "Sequential history number for tracking, tracks the number of new histories since the last particle was recorded")
        .value("EGS_LATCH", IntPropertyType::EGS_LATCH,
               "EGS-specific latch variable (see BEAMnrc User Manual, Chapter 8 for details)")
        .value("PENELOPE_ILB1", IntPropertyType::PENELOPE_ILB1,
               "PENELOPE ILB array value 1, corresponds to the generation of the particle (1 for primary, 2 for secondary, etc.)")
        .value("PENELOPE_ILB2", IntPropertyType::PENELOPE_ILB2,
               "PENELOPE ILB array value 2, corresponds to the particle type of the particle's parent (applies only if ILB1 > 1)")
        .value("PENELOPE_ILB3", IntPropertyType::PENELOPE_ILB3,
               "PENELOPE ILB array value 3, corresponds to the interaction type that created the particle (applies only if ILB1 > 1)")
        .value("PENELOPE_ILB4", IntPropertyType::PENELOPE_ILB4,
               "PENELOPE ILB array value 4, is non-zero if the particle is created by atomic relaxation and corresponds to the atomic transition that created the particle")
        .value("PENELOPE_ILB5", IntPropertyType::PENELOPE_ILB5,
               "PENELOPE ILB array value 5, a user-defined value which is passed on to all descendant particles created by this particle")
        .value("CUSTOM", IntPropertyType::CUSTOM,
               "Custom integer property type, can be used for any user-defined purpose");

    py::enum_<FloatPropertyType>(m, "FloatPropertyType",
        "Enumeration of floating-point property types for particles. "
        "Defines standardized float properties that can be associated with particles from different Monte Carlo simulation codes.")
        .value("INVALID", FloatPropertyType::INVALID,
               "Invalid property type, used for error checking")
        .value("XLAST", FloatPropertyType::XLAST,
               "EGS-specific XLAST variable, for photons it is the X position of the last interaction, for electrons/positrons it is the X position it (or its ancestor) was created at by a photon")
        .value("YLAST", FloatPropertyType::YLAST,
               "EGS-specific YLAST variable, for photons it is the Y position of the last interaction, for electrons/positrons it is the Y position it (or its ancestor) was created at by a photon")
        .value("ZLAST", FloatPropertyType::ZLAST,
               "EGS-specific ZLAST variable, for photons it is the Z position of the last interaction, for electrons/positrons it is the Z position it (or its ancestor) was created at by a photon")
        .value("CUSTOM", FloatPropertyType::CUSTOM,
               "Custom float property type, can be used for any user-defined purpose");

    py::enum_<BoolPropertyType>(m, "BoolPropertyType",
        "Enumeration of boolean property types for particles. "
        "Defines standardized boolean flags that can be associated with particles from different Monte Carlo simulation codes.")
        .value("INVALID", BoolPropertyType::INVALID,
               "Invalid property type")
        .value("IS_MULTIPLE_CROSSER", BoolPropertyType::IS_MULTIPLE_CROSSER,
               "Flag indicating that the particle crossed the phase space plane multiple times (assuming the phase space is planar)")
        .value("IS_SECONDARY_PARTICLE", BoolPropertyType::IS_SECONDARY_PARTICLE,
               "Flag indicating that the particle is a secondary")
        .value("CUSTOM", BoolPropertyType::CUSTOM,
               "Custom boolean property type, can be used for any user-defined purpose");

    // Helpers for PDG <-> ParticleType
    m.def("get_particle_type_from_pdgid", &getParticleTypeFromPDGID,
          py::arg("pdg"), 
          "Map PDG particle code to ParticleType enum. Returns ParticleType for recognized PDG codes.");
    m.def("get_pdgid", &getPDGIDFromParticleType,
          py::arg("type"), 
          "Get PDG particle code from ParticleType enum. Returns integer PDG code.");
    m.def("get_particle_type_name", &getParticleTypeName,
          py::arg("type"), 
          "Get human-readable name for ParticleType enum (e.g., 'electron', 'photon').");

    // Particle class
    py::class_<Particle>(m, "Particle", 
        "Represents a particle in phase space with position, momentum direction, kinetic energy, "
        "and additional properties. Used for Monte Carlo particle transport simulations.")
        .def(py::init<>(), 
             "Create a particle with default values (unsupported type, zero energy)")
        .def(py::init<ParticleType, float, float, float, float, float, float, float, bool, float>(),
             py::arg("type"), py::arg("kineticEnergy"),
             py::arg("x"), py::arg("y"), py::arg("z"),
             py::arg("px"), py::arg("py"), py::arg("pz"),
             py::arg("isNewHistory") = true, py::arg("weight") = 1.0f,
             "Create a particle with specified properties. Directional cosines (px, py, pz) are "
             "automatically normalized to form a unit vector.")
        // getters
        .def_property("type", &Particle::getType, 
             [](Particle &p, ParticleType t){ p = Particle(t, p.getKineticEnergy(), p.getX(), p.getY(), p.getZ(), p.getDirectionalCosineX(), p.getDirectionalCosineY(), p.getDirectionalCosineZ(), p.isNewHistory(), p.getWeight()); },
             "The particle type (electron, photon, proton, etc.)")
        .def_property("kinetic_energy", &Particle::getKineticEnergy, &Particle::setKineticEnergy,
                     "The kinetic energy of the particle")
        .def_property("x", &Particle::getX, &Particle::setX,
                     "The X coordinate position")
        .def_property("y", &Particle::getY, &Particle::setY,
                     "The Y coordinate position")
        .def_property("z", &Particle::getZ, &Particle::setZ,
                     "The Z coordinate position")
        .def_property("px", &Particle::getDirectionalCosineX, &Particle::setDirectionalCosineX,
                     "The X component of the directional cosine (momentum unit vector)")
        .def_property("py", &Particle::getDirectionalCosineY, &Particle::setDirectionalCosineY,
                     "The Y component of the directional cosine (momentum unit vector)")
        .def_property("pz", &Particle::getDirectionalCosineZ, &Particle::setDirectionalCosineZ,
                     "The Z component of the directional cosine (momentum unit vector)")
        .def_property("weight", &Particle::getWeight, &Particle::setWeight,
                     "The statistical weight of the particle")
        .def_property("is_new_history", &Particle::isNewHistory, &Particle::setNewHistory,
                     "Whether this particle starts a new Monte Carlo history")
        .def("project_to_x", &Particle::projectToXValue, py::arg("X"),
             "Project the particle's trajectory to a specific X coordinate. "
             "Calculates where the particle would be when it reaches the specified X value, "
             "assuming it travels in a straight line. Updates the Y and Z coordinates accordingly. "
             "Returns True if projection was successful, False if impossible (particle has no movement in X direction).")
        .def("project_to_y", &Particle::projectToYValue, py::arg("Y"),
             "Project the particle's trajectory to a specific Y coordinate. "
             "Calculates where the particle would be when it reaches the specified Y value, "
             "assuming it travels in a straight line. Updates the X and Z coordinates accordingly. "
             "Returns True if projection was successful, False if impossible (particle has no movement in Y direction).")
        .def("project_to_z", &Particle::projectToZValue, py::arg("Z"),
             "Project the particle's trajectory to a specific Z coordinate. "
             "Calculates where the particle would be when it reaches the specified Z value, "
             "assuming it travels in a straight line. Updates the X and Y coordinates accordingly. "
             "Returns True if projection was successful, False if impossible (particle has no movement in Z direction).")
        .def("get_incremental_histories", &Particle::getIncrementalHistories,
             "Convenience function to get the number of incremental histories regardless of whether the property is set. "
             "If the property is not set, it returns 1 if the particle is marked as a new history, otherwise 0.")
        .def("set_incremental_histories", &Particle::setIncrementalHistories,
             "Convenience function to set the number of incremental histories using the INCREMENTAL_HISTORY_NUMBER integer property. "
             "The value must be greater than 0.")
        ;

    // FixedValues struct for writers
    py::class_<FixedValues>(m, "FixedValues",
        "Structure defining constant (fixed) values for particle properties. "
        "Used to optimize phase space files by storing constant values once rather than "
        "repeating them for every particle. Useful when all particles share certain properties "
        "(e.g., all particles start from the same position).")
        .def(py::init<>(), 
             "Create a FixedValues object with all values set to non-constant (defaults)")
        .def_readwrite("x_is_constant", &FixedValues::xIsConstant,
                      "True if X coordinate is constant for all particles")
        .def_readwrite("y_is_constant", &FixedValues::yIsConstant,
                      "True if Y coordinate is constant for all particles")
        .def_readwrite("z_is_constant", &FixedValues::zIsConstant,
                      "True if Z coordinate is constant for all particles")
        .def_readwrite("px_is_constant", &FixedValues::pxIsConstant,
                      "True if X directional cosine is constant for all particles")
        .def_readwrite("py_is_constant", &FixedValues::pyIsConstant,
                      "True if Y directional cosine is constant for all particles")
        .def_readwrite("pz_is_constant", &FixedValues::pzIsConstant,
                      "True if Z directional cosine is constant for all particles")
        .def_readwrite("weight_is_constant", &FixedValues::weightIsConstant,
                      "True if statistical weight is constant for all particles")
        .def_readwrite("constant_x", &FixedValues::constantX,
                      "Constant X coordinate value (when x_is_constant is true)")
        .def_readwrite("constant_y", &FixedValues::constantY,
                      "Constant Y coordinate value (when y_is_constant is true)")
        .def_readwrite("constant_z", &FixedValues::constantZ,
                      "Constant Z coordinate value (when z_is_constant is true)")
        .def_readwrite("constant_px", &FixedValues::constantPx,
                      "Constant X directional cosine value (when px_is_constant is true)")
        .def_readwrite("constant_py", &FixedValues::constantPy,
                      "Constant Y directional cosine value (when py_is_constant is true)")
        .def_readwrite("constant_pz", &FixedValues::constantPz,
                      "Constant Z directional cosine value (when pz_is_constant is true)")
        .def_readwrite("constant_weight", &FixedValues::constantWeight,
                      "Constant statistical weight value (when weight_is_constant is true)")
        .def("__eq__", &FixedValues::operator==, 
             "Equality comparison operator");

    // PhaseSpaceFileReader (abstract base; created via FormatRegistry factories)
    py::class_<PhaseSpaceFileReader, std::unique_ptr<PhaseSpaceFileReader>>(m, "PhaseSpaceFileReader",
        "Reader for phase space files from various Monte Carlo simulation formats (EGS, IAEA, TOPAS, etc.). "
        "Provides unified interface for reading particle data from different file formats. "
        "Create using create_reader() or create_reader_for_format() factory functions.")
        .def("get_number_of_particles", &PhaseSpaceFileReader::getNumberOfParticles,
             "Get the total number of particles in the phase space file.")
        .def("get_number_of_original_histories", &PhaseSpaceFileReader::getNumberOfOriginalHistories,
             "Get the number of original Monte Carlo histories that generated this phase space.")
        .def("get_histories_read", &PhaseSpaceFileReader::getHistoriesRead,
             "Get the number of Monte Carlo histories read so far. Returns total original histories if end of file reached.")
        .def("get_particles_read", static_cast<std::uint64_t (PhaseSpaceFileReader::*)()>(&PhaseSpaceFileReader::getParticlesRead),
             "Get the number of particles read so far (excludes metadata and skipped particles).")
        .def("has_more_particles", &PhaseSpaceFileReader::hasMoreParticles,
             "Check if there are more particles available to read. Returns True if more particles remain, False at end of file.")
        .def("get_next_particle", static_cast<Particle (PhaseSpaceFileReader::*)()>(&PhaseSpaceFileReader::getNextParticle),
             "Read and return the next particle from the file. Automatically handles buffering and format-specific parsing.")
        .def("get_file_size", &PhaseSpaceFileReader::getFileSize,
             "Get the size of the phase space file in bytes.")
        .def("get_file_name", &PhaseSpaceFileReader::getFileName,
             "Get the filename/path of the phase space file being read.")
        .def("get_phsp_format", &PhaseSpaceFileReader::getPHSPFormat,
             "Get the phase space file format identifier (e.g., 'IAEA', 'EGS', 'TOPAS').")
        .def("move_to_particle", &PhaseSpaceFileReader::moveToParticle, py::arg("index"),
             "Move the file position to a specific particle index (0-based). Useful for random access.")
        .def("is_x_constant", &PhaseSpaceFileReader::isXConstant,
             "Check if X coordinate is constant for all particles.")
        .def("is_y_constant", &PhaseSpaceFileReader::isYConstant,
             "Check if Y coordinate is constant for all particles.")
        .def("is_z_constant", &PhaseSpaceFileReader::isZConstant,
             "Check if Z coordinate is constant for all particles.")
        .def("is_px_constant", &PhaseSpaceFileReader::isPxConstant,
             "Check if X directional cosine is constant for all particles.")
        .def("is_py_constant", &PhaseSpaceFileReader::isPyConstant,
             "Check if Y directional cosine is constant for all particles.")
        .def("is_pz_constant", &PhaseSpaceFileReader::isPzConstant,
             "Check if Z directional cosine is constant for all particles.")
        .def("is_weight_constant", &PhaseSpaceFileReader::isWeightConstant,
             "Check if statistical weight is constant for all particles.")
        .def("get_constant_x", &PhaseSpaceFileReader::getConstantX,
             "Get the constant X coordinate value (when is_x_constant returns True).")
        .def("get_constant_y", &PhaseSpaceFileReader::getConstantY,
             "Get the constant Y coordinate value (when is_y_constant returns True).")
        .def("get_constant_z", &PhaseSpaceFileReader::getConstantZ,
             "Get the constant Z coordinate value (when is_z_constant returns True).")
        .def("get_constant_px", &PhaseSpaceFileReader::getConstantPx,
             "Get the constant X directional cosine value (when is_px_constant returns True).")
        .def("get_constant_py", &PhaseSpaceFileReader::getConstantPy,
             "Get the constant Y directional cosine value (when is_py_constant returns True).")
        .def("get_constant_pz", &PhaseSpaceFileReader::getConstantPz,
             "Get the constant Z directional cosine value (when is_pz_constant returns True).")
        .def("get_constant_weight", &PhaseSpaceFileReader::getConstantWeight,
             "Get the constant statistical weight value (when is_weight_constant returns True).")
        .def("close", &PhaseSpaceFileReader::close,
             "Close the phase space file and release associated resources.")
        .def("__iter__", [](PhaseSpaceFileReader &self) -> PhaseSpaceFileReader& { return self; }, py::return_value_policy::reference_internal,
             "Return the reader as an iterator (enables 'for particle in reader: ...' syntax).")
        .def("__next__", [](PhaseSpaceFileReader &self) {
            if (!self.hasMoreParticles()) throw py::stop_iteration();
            return self.getNextParticle();
        }, "Get the next particle in iteration. Raises StopIteration when no more particles remain.")
        ;

    // PhaseSpaceFileWriter (abstract base; created via FormatRegistry factories)
    py::class_<PhaseSpaceFileWriter, std::unique_ptr<PhaseSpaceFileWriter>>(m, "PhaseSpaceFileWriter",
        "Writer for phase space files to various Monte Carlo simulation formats (EGS, IAEA, TOPAS, etc.). "
        "Provides unified interface for writing particle data to different file formats. "
        "Create using create_writer() or create_writer_for_format() factory functions.")
        .def("write_particle", &PhaseSpaceFileWriter::writeParticle, py::arg("particle"),
             "Write a particle to the phase space file. Automatically buffers and applies constant values.")
        .def("get_particles_written", &PhaseSpaceFileWriter::getParticlesWritten,
             "Get the number of particles written to the file (excludes pseudo-particles).")
        .def("get_histories_written", &PhaseSpaceFileWriter::getHistoriesWritten,
             "Get the number of Monte Carlo histories written to the file.")
        .def("add_additional_histories", &PhaseSpaceFileWriter::addAdditionalHistories, py::arg("count"),
             "Add additional Monte Carlo histories to the count. Used for simulation histories that produced no particles.")
        .def("get_maximum_supported_particles", &PhaseSpaceFileWriter::getMaximumSupportedParticles,
             "Get the maximum number of particles this format can support.")
        .def("get_file_name", &PhaseSpaceFileWriter::getFileName,
             "Get the filename/path where the phase space file is being written.")
        .def("get_phsp_format", &PhaseSpaceFileWriter::getPHSPFormat,
             "Get the phase space file format identifier (e.g., 'IAEA', 'EGS', 'TOPAS').")
        .def("close", &PhaseSpaceFileWriter::close,
             "Close the phase space file, flush buffered data, and finalize writing.");

    // SupportedFormat struct
    py::class_<SupportedFormat>(m, "SupportedFormat",
        "Information about a supported phase space file format including name, description, and file extension.")
        .def_property_readonly("name", [](const SupportedFormat& f){ return f.name; },
                              "Format identifier name (e.g., 'IAEA', 'EGS', 'TOPAS')")
        .def_property_readonly("description", [](const SupportedFormat& f){ return f.description; },
                              "Human-readable description of the format")
        .def_property_readonly("file_extension", [](const SupportedFormat& f){ return f.fileExtension; },
                              "Standard file extension for this format (e.g., '.phsp', '.IAEAphsp')")
        .def_property_readonly("file_extension_can_have_suffix", [](const SupportedFormat& f){ return f.fileExtensionCanHaveSuffix; },
                              "True if the file extension can have an additional suffix (e.g., '.egsphsp1')")
        ;

    // FormatRegistry static interface
    py::class_<FormatRegistry>(m, "FormatRegistry",
        "Registry for phase space file format plugins. Provides factory functions to create readers and writers "
        "for different simulation formats (EGS, IAEA, TOPAS, ROOT, etc.).")
        .def_static("register_standard_formats", &FormatRegistry::RegisterStandardFormats,
                   "Register all built-in phase space file formats. Must be called before creating readers/writers.")
        .def_static("supported_formats", &FormatRegistry::SupportedFormats,
                   "Get list of all registered supported formats. Returns list of SupportedFormat objects.")
        .def_static("formats_for_extension", &FormatRegistry::FormatsForExtension, py::arg("extension"),
                   "Get list of formats that match a given file extension. Returns list of format names.")
        .def_static("extension_for_format", &FormatRegistry::ExtensionForFormat, py::arg("format_name"),
                   "Get the standard file extension for a given format name.")
        ;

    // Convenience factory functions for readers
    m.def("create_reader", 
          [](const std::string& filename, const UserOptions& options){
              FormatRegistry::RegisterStandardFormats();
              return FormatRegistry::CreateReader(filename, options);
          }, 
          py::arg("filename"), py::arg("options") = UserOptions{},
          "Create a phase space file reader with automatic format detection based on file extension. "
          "Returns PhaseSpaceFileReader instance for reading particles from the file.");
    
    m.def("create_reader_for_format", 
          [](const std::string& format, const std::string& filename, const UserOptions& options){
              FormatRegistry::RegisterStandardFormats();
              return FormatRegistry::CreateReader(format, filename, options);
          }, 
          py::arg("format_name"), py::arg("filename"), py::arg("options") = UserOptions{},
          "Create a phase space file reader for a specific format (e.g., 'IAEA', 'EGS', 'TOPAS'). "
          "Returns PhaseSpaceFileReader instance configured for the specified format.");

    // Convenience factory functions for writers
    m.def("create_writer", 
          [](const std::string& filename, const UserOptions& options, const FixedValues& fixed_values){
              FormatRegistry::RegisterStandardFormats();
              return FormatRegistry::CreateWriter(filename, options, fixed_values);
          }, 
          py::arg("filename"), py::arg("options") = UserOptions{}, py::arg("fixed_values") = FixedValues{},
          "Create a phase space file writer with automatic format detection based on file extension. "
          "Returns PhaseSpaceFileWriter instance for writing particles to the file. "
          "Use fixed_values to specify constant properties for all particles.");
    
    m.def("create_writer_for_format", 
          [](const std::string& format, const std::string& filename, const UserOptions& options, const FixedValues& fixed_values){
              FormatRegistry::RegisterStandardFormats();
              return FormatRegistry::CreateWriter(format, filename, options, fixed_values);
          }, 
          py::arg("format_name"), py::arg("filename"), py::arg("options") = UserOptions{}, py::arg("fixed_values") = FixedValues{},
          "Create a phase space file writer for a specific format (e.g., 'IAEA', 'EGS', 'TOPAS'). "
          "Returns PhaseSpaceFileWriter instance configured for the specified format. "
          "Use fixed_values to specify constant properties for all particles.");

    // Expose a simple alias to create a Particle by PDG code
    m.def("particle_from_pdg", [](int pdg, float kineticEnergy, float x, float y, float z, float px, float py, float pz, bool isNewHistory, float weight){
        ParticleType t = getParticleTypeFromPDGID(static_cast<std::int32_t>(pdg));
        return Particle(t, kineticEnergy, x, y, z, px, py, pz, isNewHistory, weight);
    }, py::arg("pdg"), py::arg("kineticEnergy"), py::arg("x"), py::arg("y"), py::arg("z"), py::arg("px"), py::arg("py"), py::arg("pz"), py::arg("isNewHistory")=true, py::arg("weight")=1.0f,
    "Create a Particle using PDG particle code instead of ParticleType enum. "
    "Automatically converts PDG code to ParticleType. Directional cosines normalized automatically.");

    // ===== Units - expose as module constants =====
    
    // Base units
    m.attr("cm") = cm;
    m.attr("MeV") = MeV;
    m.attr("g") = g;
    m.attr("s") = s;
    m.attr("mol") = mol;
    m.attr("K") = K;
    m.attr("A") = A;
    m.attr("cd") = cd;

    // Numerical constants
    m.attr("radian") = radian;
    m.attr("PI") = PI;

    // Distance
    m.attr("km") = km;
    m.attr("m") = m;
    m.attr("mm") = mm;
    m.attr("um") = um;
    m.attr("nm") = nm;
    m.attr("angstrom") = angstrom;
    m.attr("inch") = in;
    m.attr("ft") = ft;

    // Area
    m.attr("km2") = km2;
    m.attr("m2") = m2;
    m.attr("cm2") = cm2;
    m.attr("mm2") = mm2;
    m.attr("um2") = um2;
    m.attr("nm2") = nm2;
    m.attr("angstrom2") = angstrom2;
    m.attr("in2") = in2;
    m.attr("ft2") = ft2;

    // Volume
    m.attr("km3") = km3;
    m.attr("m3") = m3;
    m.attr("cm3") = cm3;
    m.attr("mm3") = mm3;
    m.attr("um3") = um3;
    m.attr("nm3") = nm3;
    m.attr("angstrom3") = angstrom3;
    m.attr("in3") = in3;
    m.attr("ft3") = ft3;
    m.attr("L") = L;
    m.attr("mL") = mL;
    m.attr("uL") = uL;

    // Energy
    m.attr("eV") = eV;
    m.attr("keV") = keV;
    m.attr("GeV") = GeV;
    m.attr("TeV") = TeV;
    m.attr("J") = J;

    // Mass
    m.attr("ug") = ug;
    m.attr("mg") = mg;
    m.attr("kg") = kg;
    m.attr("lb") = lb;

    // Time
    m.attr("minute") = minute;
    m.attr("hour") = hour;
    m.attr("day") = day;
    m.attr("year") = year;

    // Frequency
    m.attr("Hz") = Hz;
    m.attr("kHz") = kHz;
    m.attr("MHz") = MHz;
    m.attr("GHz") = GHz;
    m.attr("THz") = THz;

    // Force
    m.attr("N") = N;
    m.attr("dyn") = dyn;
    m.attr("lbf") = lbf;

    // Pressure
    m.attr("Pa") = Pa;
    m.attr("kPa") = kPa;
    m.attr("MPa") = MPa;
    m.attr("GPa") = GPa;
    m.attr("atm") = atm;
    m.attr("bar") = bar;
    m.attr("mbar") = mbar;
    m.attr("torr") = torr;
    m.attr("mmHg") = mmHg;
    m.attr("psi") = psi;
    m.attr("baryn") = baryn;

    // Charge
    m.attr("C") = C;
    m.attr("mC") = mC;
    m.attr("uC") = uC;
    m.attr("nC") = nC;
    m.attr("pC") = pC;

    // Density
    m.attr("g_per_cm3") = g_per_cm3;
    m.attr("kg_per_m3") = kg_per_m3;

    // Dose
    m.attr("Gy") = Gy;
    m.attr("cGy") = cGy;
    m.attr("rad") = rad;
    m.attr("Sv") = Sv;
    m.attr("cSv") = cSv;
    m.attr("mSv") = mSv;
    m.attr("rem") = rem;

    // Angle
    m.attr("deg") = deg;
}
