#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "particlezoo/Particle.h"
#include "particlezoo/PDGParticleCodes.h"
#include "particlezoo/IAEA/IAEAphspFile.h"

namespace py = pybind11;
using namespace ParticleZoo;

PYBIND11_MODULE(_pz, m) {
    m.doc() = "Python bindings for ParticleZoo core and IAEA reader";

    // ParticleType enum (expose only sentinel values; use helpers for full mapping)
    py::enum_<ParticleType>(m, "ParticleType")
        .value("Unsupported", ParticleType::Unsupported)
        .value("PseudoParticle", ParticleType::PseudoParticle);

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

    // IAEA Reader
    using IAEAReader = IAEAphspFile::Reader;
    py::class_<IAEAReader>(m, "IAEAReader")
        .def(py::init<const std::string&, const UserOptions&>(),
             py::arg("filename"), py::arg("options") = UserOptions{})
        .def("get_number_of_particles", static_cast<std::uint64_t (IAEAReader::*)() const>(&IAEAReader::getNumberOfParticles))
        .def("get_number_of_particles_by_type", static_cast<std::uint64_t (IAEAReader::*)(ParticleType) const>(&IAEAReader::getNumberOfParticles), py::arg("particle_type"))
    .def("get_number_of_original_histories", &IAEAReader::getNumberOfOriginalHistories)
    .def("has_more_particles", &IAEAReader::hasMoreParticles)
    .def("get_next_particle", static_cast<Particle (IAEAReader::*)()>(&IAEAReader::getNextParticle))
    .def("get_file_size", &IAEAReader::getFileSize)
    .def("get_file_name", &IAEAReader::getFileName)
    .def("close", &IAEAReader::close)
        .def("__iter__", [](IAEAReader &self) -> IAEAReader& { return self; }, py::return_value_policy::reference_internal)
        .def("__next__", [](IAEAReader &self) {
            if (!self.hasMoreParticles()) throw py::stop_iteration();
            return self.getNextParticle();
        })
        ;

    // Expose a simple alias to create a Particle by PDG code
    m.def("particle_from_pdg", [](int pdg, float kineticEnergy, float x, float y, float z, float px, float py, float pz, bool isNewHistory, float weight){
        ParticleType t = getParticleTypeFromPDGID(static_cast<std::int32_t>(pdg));
        return Particle(t, kineticEnergy, x, y, z, px, py, pz, isNewHistory, weight);
    }, py::arg("pdg"), py::arg("kineticEnergy"), py::arg("x"), py::arg("y"), py::arg("z"), py::arg("px"), py::arg("py"), py::arg("pz"), py::arg("isNewHistory")=true, py::arg("weight")=1.0f);
}
