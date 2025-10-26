#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "particlezoo/Particle.h"
#include "particlezoo/PDGParticleCodes.h"
#include "particlezoo/PhaseSpaceFileReader.h"
#include "particlezoo/utilities/formats.h"

namespace py = pybind11;
using namespace ParticleZoo;

PYBIND11_MODULE(_pz, m) {
    m.doc() = "Python bindings for ParticleZoo core and IAEA reader";

    // Ensure built-in formats are registered before any factory calls
    try { FormatRegistry::RegisterStandardFormats(); } catch (...) { /* idempotent; ignore */ }

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
    m.def("create_reader", [](const std::string& filename){
        FormatRegistry::RegisterStandardFormats();
        return FormatRegistry::CreateReader(filename, UserOptions{});
    }, py::arg("filename"));
    m.def("create_reader_for_format", [](const std::string& format, const std::string& filename){
        FormatRegistry::RegisterStandardFormats();
        return FormatRegistry::CreateReader(format, filename, UserOptions{});
    }, py::arg("format_name"), py::arg("filename"));

    // Expose a simple alias to create a Particle by PDG code
    m.def("particle_from_pdg", [](int pdg, float kineticEnergy, float x, float y, float z, float px, float py, float pz, bool isNewHistory, float weight){
        ParticleType t = getParticleTypeFromPDGID(static_cast<std::int32_t>(pdg));
        return Particle(t, kineticEnergy, x, y, z, px, py, pz, isNewHistory, weight);
    }, py::arg("pdg"), py::arg("kineticEnergy"), py::arg("x"), py::arg("y"), py::arg("z"), py::arg("px"), py::arg("py"), py::arg("pz"), py::arg("isNewHistory")=true, py::arg("weight")=1.0f);
}
