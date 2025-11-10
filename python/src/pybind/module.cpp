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
    py::class_<UserOptions>(m, "UserOptions")
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
    py::class_<ArgParser>(m, "ArgParser")
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
            "Parse command line arguments and return UserOptions");

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
    py::enum_<IntPropertyType>(m, "IntPropertyType")
        .value("INVALID", IntPropertyType::INVALID)
        .value("INCREMENTAL_HISTORY_NUMBER", IntPropertyType::INCREMENTAL_HISTORY_NUMBER)
        .value("EGS_LATCH", IntPropertyType::EGS_LATCH)
        .value("PENELOPE_ILB1", IntPropertyType::PENELOPE_ILB1)
        .value("PENELOPE_ILB2", IntPropertyType::PENELOPE_ILB2)
        .value("PENELOPE_ILB3", IntPropertyType::PENELOPE_ILB3)
        .value("PENELOPE_ILB4", IntPropertyType::PENELOPE_ILB4)
        .value("PENELOPE_ILB5", IntPropertyType::PENELOPE_ILB5)
        .value("CUSTOM", IntPropertyType::CUSTOM);

    py::enum_<FloatPropertyType>(m, "FloatPropertyType")
        .value("INVALID", FloatPropertyType::INVALID)
        .value("XLAST", FloatPropertyType::XLAST)
        .value("YLAST", FloatPropertyType::YLAST)
        .value("ZLAST", FloatPropertyType::ZLAST)
        .value("CUSTOM", FloatPropertyType::CUSTOM);

    py::enum_<BoolPropertyType>(m, "BoolPropertyType")
        .value("INVALID", BoolPropertyType::INVALID)
        .value("IS_MULTIPLE_CROSSER", BoolPropertyType::IS_MULTIPLE_CROSSER)
        .value("IS_SECONDARY_PARTICLE", BoolPropertyType::IS_SECONDARY_PARTICLE)
        .value("CUSTOM", BoolPropertyType::CUSTOM);

    // Helpers for PDG <-> ParticleType
    m.def("get_particle_type_from_pdgid", &getParticleTypeFromPDGID,
          py::arg("pdg"), "Map PDG code to ParticleType");
    m.def("get_pdgid", &getPDGIDFromParticleType,
          py::arg("type"), "Get PDG code from ParticleType");
    m.def("get_particle_type_name", &getParticleTypeName,
          py::arg("type"), "Get human-readable name for ParticleType");

    // Particle class
    py::class_<Particle>(m, "Particle")
        .def(py::init<>())
        .def(py::init<ParticleType, float, float, float, float, float, float, float, bool, float>(),
             py::arg("type"), py::arg("kineticEnergy"),
             py::arg("x"), py::arg("y"), py::arg("z"),
             py::arg("px"), py::arg("py"), py::arg("pz"),
             py::arg("isNewHistory") = true, py::arg("weight") = 1.0f)
        // getters
        .def_property("type", &Particle::getType, [](Particle &p, ParticleType t){ p = Particle(t, p.getKineticEnergy(), p.getX(), p.getY(), p.getZ(), p.getDirectionalCosineX(), p.getDirectionalCosineY(), p.getDirectionalCosineZ(), p.isNewHistory(), p.getWeight()); })
        .def_property("kinetic_energy", &Particle::getKineticEnergy, &Particle::setKineticEnergy)
        .def_property("x", &Particle::getX, &Particle::setX)
        .def_property("y", &Particle::getY, &Particle::setY)
        .def_property("z", &Particle::getZ, &Particle::setZ)
        .def_property("px", &Particle::getDirectionalCosineX, &Particle::setDirectionalCosineX)
        .def_property("py", &Particle::getDirectionalCosineY, &Particle::setDirectionalCosineY)
        .def_property("pz", &Particle::getDirectionalCosineZ, &Particle::setDirectionalCosineZ)
        .def_property("weight", &Particle::getWeight, &Particle::setWeight)
        .def_property("is_new_history", &Particle::isNewHistory, &Particle::setNewHistory)
        .def("project_to_x", &Particle::projectToXValue, py::arg("X"))
        .def("project_to_y", &Particle::projectToYValue, py::arg("Y"))
        .def("project_to_z", &Particle::projectToZValue, py::arg("Z"))
        .def("get_incremental_histories", &Particle::getIncrementalHistories)
        .def("set_incremental_histories", &Particle::setIncrementalHistories)
        ;

    // FixedValues struct for writers
    py::class_<FixedValues>(m, "FixedValues")
        .def(py::init<>())
        .def_readwrite("x_is_constant", &FixedValues::xIsConstant)
        .def_readwrite("y_is_constant", &FixedValues::yIsConstant)
        .def_readwrite("z_is_constant", &FixedValues::zIsConstant)
        .def_readwrite("px_is_constant", &FixedValues::pxIsConstant)
        .def_readwrite("py_is_constant", &FixedValues::pyIsConstant)
        .def_readwrite("pz_is_constant", &FixedValues::pzIsConstant)
        .def_readwrite("weight_is_constant", &FixedValues::weightIsConstant)
        .def_readwrite("constant_x", &FixedValues::constantX)
        .def_readwrite("constant_y", &FixedValues::constantY)
        .def_readwrite("constant_z", &FixedValues::constantZ)
        .def_readwrite("constant_px", &FixedValues::constantPx)
        .def_readwrite("constant_py", &FixedValues::constantPy)
        .def_readwrite("constant_pz", &FixedValues::constantPz)
        .def_readwrite("constant_weight", &FixedValues::constantWeight)
        .def("__eq__", &FixedValues::operator==);

    // PhaseSpaceFileReader (abstract base; created via FormatRegistry factories)
    py::class_<PhaseSpaceFileReader, std::unique_ptr<PhaseSpaceFileReader>>(m, "PhaseSpaceFileReader")
        .def("get_number_of_particles", &PhaseSpaceFileReader::getNumberOfParticles)
        .def("get_number_of_original_histories", &PhaseSpaceFileReader::getNumberOfOriginalHistories)
        .def("get_histories_read", &PhaseSpaceFileReader::getHistoriesRead)
        .def("get_particles_read", static_cast<std::uint64_t (PhaseSpaceFileReader::*)()>(&PhaseSpaceFileReader::getParticlesRead))
        .def("has_more_particles", &PhaseSpaceFileReader::hasMoreParticles)
        .def("get_next_particle", static_cast<Particle (PhaseSpaceFileReader::*)()>(&PhaseSpaceFileReader::getNextParticle))
        .def("get_file_size", &PhaseSpaceFileReader::getFileSize)
        .def("get_file_name", &PhaseSpaceFileReader::getFileName)
        .def("get_phsp_format", &PhaseSpaceFileReader::getPHSPFormat)
        .def("move_to_particle", &PhaseSpaceFileReader::moveToParticle, py::arg("index"))
        .def("is_x_constant", &PhaseSpaceFileReader::isXConstant)
        .def("is_y_constant", &PhaseSpaceFileReader::isYConstant)
        .def("is_z_constant", &PhaseSpaceFileReader::isZConstant)
        .def("is_px_constant", &PhaseSpaceFileReader::isPxConstant)
        .def("is_py_constant", &PhaseSpaceFileReader::isPyConstant)
        .def("is_pz_constant", &PhaseSpaceFileReader::isPzConstant)
        .def("is_weight_constant", &PhaseSpaceFileReader::isWeightConstant)
        .def("get_constant_x", &PhaseSpaceFileReader::getConstantX)
        .def("get_constant_y", &PhaseSpaceFileReader::getConstantY)
        .def("get_constant_z", &PhaseSpaceFileReader::getConstantZ)
        .def("get_constant_px", &PhaseSpaceFileReader::getConstantPx)
        .def("get_constant_py", &PhaseSpaceFileReader::getConstantPy)
        .def("get_constant_pz", &PhaseSpaceFileReader::getConstantPz)
        .def("get_constant_weight", &PhaseSpaceFileReader::getConstantWeight)
        .def("close", &PhaseSpaceFileReader::close)
        .def("__iter__", [](PhaseSpaceFileReader &self) -> PhaseSpaceFileReader& { return self; }, py::return_value_policy::reference_internal)
        .def("__next__", [](PhaseSpaceFileReader &self) {
            if (!self.hasMoreParticles()) throw py::stop_iteration();
            return self.getNextParticle();
        })
        ;

    // PhaseSpaceFileWriter (abstract base; created via FormatRegistry factories)
    py::class_<PhaseSpaceFileWriter, std::unique_ptr<PhaseSpaceFileWriter>>(m, "PhaseSpaceFileWriter")
        .def("write_particle", &PhaseSpaceFileWriter::writeParticle, py::arg("particle"),
             "Write a particle to the phase space file")
        .def("get_particles_written", &PhaseSpaceFileWriter::getParticlesWritten,
             "Get the number of particles written to the file")
        .def("get_histories_written", &PhaseSpaceFileWriter::getHistoriesWritten,
             "Get the number of Monte Carlo histories written")
        .def("add_additional_histories", &PhaseSpaceFileWriter::addAdditionalHistories, py::arg("count"),
             "Add additional Monte Carlo histories to the count (for empty histories)")
        .def("get_maximum_supported_particles", &PhaseSpaceFileWriter::getMaximumSupportedParticles,
             "Get the maximum number of particles this writer can support")
        .def("get_file_name", &PhaseSpaceFileWriter::getFileName,
             "Get the filename where the phase space file is being written")
        .def("get_phsp_format", &PhaseSpaceFileWriter::getPHSPFormat,
             "Get the phase space file format identifier")
        .def("close", &PhaseSpaceFileWriter::close,
             "Close the phase space file and finalize writing");

    // SupportedFormat struct
    py::class_<SupportedFormat>(m, "SupportedFormat")
        .def_property_readonly("name", [](const SupportedFormat& f){ return f.name; })
        .def_property_readonly("description", [](const SupportedFormat& f){ return f.description; })
        .def_property_readonly("file_extension", [](const SupportedFormat& f){ return f.fileExtension; })
        .def_property_readonly("file_extension_can_have_suffix", [](const SupportedFormat& f){ return f.fileExtensionCanHaveSuffix; })
        ;

    // FormatRegistry static interface
    py::class_<FormatRegistry>(m, "FormatRegistry")
        .def_static("register_standard_formats", &FormatRegistry::RegisterStandardFormats)
        .def_static("supported_formats", &FormatRegistry::SupportedFormats)
        .def_static("formats_for_extension", &FormatRegistry::FormatsForExtension, py::arg("extension"))
        .def_static("extension_for_format", &FormatRegistry::ExtensionForFormat, py::arg("format_name"))
        ;

    // Convenience factory functions for readers
    m.def("create_reader", 
          [](const std::string& filename, const UserOptions& options){
              FormatRegistry::RegisterStandardFormats();
              return FormatRegistry::CreateReader(filename, options);
          }, 
          py::arg("filename"), py::arg("options") = UserOptions{},
          "Create a reader with automatic format detection");
    
    m.def("create_reader_for_format", 
          [](const std::string& format, const std::string& filename, const UserOptions& options){
              FormatRegistry::RegisterStandardFormats();
              return FormatRegistry::CreateReader(format, filename, options);
          }, 
          py::arg("format_name"), py::arg("filename"), py::arg("options") = UserOptions{},
          "Create a reader for a specific format");

    // Convenience factory functions for writers
    m.def("create_writer", 
          [](const std::string& filename, const UserOptions& options, const FixedValues& fixed_values){
              FormatRegistry::RegisterStandardFormats();
              return FormatRegistry::CreateWriter(filename, options, fixed_values);
          }, 
          py::arg("filename"), py::arg("options") = UserOptions{}, py::arg("fixed_values") = FixedValues{},
          "Create a writer with automatic format detection");
    
    m.def("create_writer_for_format", 
          [](const std::string& format, const std::string& filename, const UserOptions& options, const FixedValues& fixed_values){
              FormatRegistry::RegisterStandardFormats();
              return FormatRegistry::CreateWriter(format, filename, options, fixed_values);
          }, 
          py::arg("format_name"), py::arg("filename"), py::arg("options") = UserOptions{}, py::arg("fixed_values") = FixedValues{},
          "Create a writer for a specific format");

    // Expose a simple alias to create a Particle by PDG code
    m.def("particle_from_pdg", [](int pdg, float kineticEnergy, float x, float y, float z, float px, float py, float pz, bool isNewHistory, float weight){
        ParticleType t = getParticleTypeFromPDGID(static_cast<std::int32_t>(pdg));
        return Particle(t, kineticEnergy, x, y, z, px, py, pz, isNewHistory, weight);
    }, py::arg("pdg"), py::arg("kineticEnergy"), py::arg("x"), py::arg("y"), py::arg("z"), py::arg("px"), py::arg("py"), py::arg("pz"), py::arg("isNewHistory")=true, py::arg("weight")=1.0f);

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
