#pragma once

namespace ParticleZoo {


    /*
     *
     * This file defines scaling constants for various units.
     * For internal consistency all values read from phase space data should be scaled by these constants.
     * Unit conversion can be done by multiplying the scaled value by the inverse of these constants.
     *
     * For example:
     *      // Convert 15 meters to kilometers
     *      float scaledValue = 15 * m;
     *      float valueInKilometers = scaledValue / km; // convert to kilometers
     * 
     * For frequent unit conversions you should pre-define the inverse of the constants that you need to avoid repeated division.
     *     e.g. constexpr float inverseKm = 1.f / km;
     *          float valueInKilometers = scaledValue * inverseKm; // convert to kilometers
     * 
     * The values of these constants and the choice of base units are subject to change in future versions of ParticleZoo.
     * Therefore you should not assume that because you have a value that is already in the same units as a base unit that
     * it doesn't need to be scaled.
     * 
     */


    /* Base units */


    constexpr float        cm = 1.0f;                            // centimeter
    constexpr float       MeV = 1.0f;                            // mega electron Volt
    constexpr float         g = 1.0f;                            // gram
    constexpr float         s = 1.0f;                            // second
    constexpr float       mol = 1.0f;                            // mole
    constexpr float         K = 1.0f;                            // Kelvin
    constexpr float         A = 1.0f;                            // Ampere
    constexpr float        cd = 1.0f;                            // candela


    /* Numerical constants */


    constexpr float    radian = 1.0f;                            // radian
    constexpr float        PI = 3.1415927f;                      // Pi constant

    
    /* Derived units */


    // Distance
    constexpr float        km = 1000.0f * cm;                    // kilometer
    constexpr float         m = 100.0f * cm;                     // meter
    constexpr float        mm = 0.1f * cm;                       // millimeter
    constexpr float        um = 1e-4f * cm;                      // micrometer
    constexpr float        nm = 1e-7f * cm;                      // nanometer
    constexpr float  angstrom = 1e-8f * cm;                      // angstrom
    constexpr float        in = 2.54f * cm;                      // inch
    constexpr float        ft = 30.48f * cm;                     // foot

    // Area
    constexpr float       km2 = km * km;                         // square kilometer
    constexpr float        m2 = m * m;                           // square meter
    constexpr float       cm2 = cm * cm;                         // square centimeter
    constexpr float       mm2 = mm * mm;                         // square millimeter
    constexpr float       um2 = um * um;                         // square micrometer
    constexpr float       nm2 = nm * nm;                         // square nanometer
    constexpr float angstrom2 = angstrom * angstrom;             // square angstrom
    constexpr float       in2 = in * in;                         // square inch
    constexpr float       ft2 = ft * ft;                         // square foot

    // Volume
    constexpr float       km3 = km * km * km;                    // cubic kilometer
    constexpr float        m3 = m * m * m;                       // cubic meter
    constexpr float       cm3 = cm * cm * cm;                    // cubic centimeter
    constexpr float       mm3 = mm * mm * mm;                    // cubic millimeter
    constexpr float       um3 = um * um * um;                    // cubic micrometer
    constexpr float       nm3 = nm * nm * nm;                    // cubic nanometer
    constexpr float angstrom3 = angstrom * angstrom * angstrom;  // cubic angstrom
    constexpr float       in3 = in * in * in;                    // cubic inch
    constexpr float       ft3 = ft * ft * ft;                    // cubic foot
    constexpr float         L = 1000 * cm3;                      // liter
    constexpr float        mL = 1e-3f * L;                       // milliliter
    constexpr float        uL = 1e-6f * L;                       // microliter

    // Energy
    constexpr float        eV = 1e-6f * MeV;                     // electron Volt
    constexpr float       keV = 1e-3f * MeV;                     // kilo electron Volt
    constexpr float       GeV = 1e3f * MeV;                      // giga electron Volt
    constexpr float       TeV = 1e6f * MeV;                      // tera electron Volt
    constexpr float         J = 6.241509e12f * MeV;              // Joule

    // Mass
    constexpr float        ug = 1e-6f * g;                       // microgram
    constexpr float        mg = 1e-3f * g;                       // milligram
    constexpr float        kg = 1000.f * g;                      // kilogram
    constexpr float        lb = 453.59237f * g;                  // pound

    // Time
    constexpr float    minute = 60.f * s;                        // minute
    constexpr float      hour = 60.f * minute;                   // hour
    constexpr float       day = 24.f * hour;                     // day
    constexpr float      year = 365.25f * day;                   // year

    // Frequency
    constexpr float        Hz = 1.f / s;                         // Hertz
    constexpr float       kHz = 1e3f * Hz;                       // kilo Hertz
    constexpr float       MHz = 1e6f * Hz;                       // mega Hertz
    constexpr float       GHz = 1e9f * Hz;                       // giga Hertz
    constexpr float       THz = 1e12f * Hz;                      // tera Hertz
    
    // Force
    constexpr float         N = kg * m / (s * s);                // Newton
    constexpr float       dyn = g * cm / (s * s);                // dyne
    constexpr float       lbf = 32.174049f * lb * ft / (s * s);   // pound-force

    // Pressure
    constexpr float        Pa = N / m2;                          // Pascal
    constexpr float       kPa = 1e3f * Pa;                       // kilo Pascal
    constexpr float       MPa = 1e6f * Pa;                       // mega Pascal
    constexpr float       GPa = 1e9f * Pa;                       // giga Pascal
    constexpr float       atm = 101325.f * Pa;                   // atmosphere
    constexpr float       bar = 1e5f * Pa;                       // bar
    constexpr float      mbar = 1e-3f * bar;                     // millibar
    constexpr float      torr = atm / 760.f;                     // torr
    constexpr float      mmHg = 133.322387415f * Pa;              // millimeter of mercury
    constexpr float       psi = lbf / in2;                       // pound per square inch
    constexpr float     baryn = dyn / cm2;                       // barye

    // Charge
    constexpr float         C = A * s;                           // Coulomb
    constexpr float        mC = 1e-3f * C;                       // milli Coulomb
    constexpr float        uC = 1e-6f * C;                       // micro Coulomb
    constexpr float        nC = 1e-9f * C;                       // nano Coulomb
    constexpr float        pC = 1e-12f * C;                      // pico Coulomb

    // Density
    constexpr float g_per_cm3 = g / cm3;                         // gram per cubic centimeter
    constexpr float kg_per_m3 = kg / m3;                         // kilogram per cubic meter

    // Dose
    constexpr float        Gy = J / kg;                          // Gray
    constexpr float       cGy = 1e-2f * Gy;                      // centi Gray
    constexpr float       rad = cGy;                             // radiation absorbed dose
    constexpr float        Sv = J / kg;                          // Sievert
    constexpr float       cSv = 1e-2f * Sv;                      // centi Sievert
    constexpr float       mSv = 1e-3f * Sv;                      // milli Sievert
    constexpr float       rem = cSv;                             // roentgen equivalent man

    // Angle
    constexpr float       deg = (PI / 180.f) * radian;         // degree

} // namespace ParticleZoo