
#include "particlezoo/MCNP/MCNPphspFile.h"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <stdexcept>

#include "particlezoo/ByteBuffer.h"
#include "particlezoo/utilities/version.h"

namespace ParticleZoo::MCNPphspFile
{

    CLICommand MCNPTitleCommand{ WRITER, "", "MCNP-title", "Specify the title card written to the MCNP surface source file header", { CLI_STRING } };
    CLICommand MCNPSurfaceCommand{ WRITER, "", "MCNP-surface", "Specify the surface number recorded on MCNP surface source particle records that do not already carry one", { CLI_UINT }, { 99999u } };
    CLICommand MCNPFormatCommand{ WRITER, "", "MCNP-format", "Specify the MCNP surface source format to write (MCNP6, MCNP5 or MCNPX)", { CLI_STRING }, { std::string("MCNP6") } };
    CLICommand MCNPLegacyHeaderCommand{ WRITER, "", "MCNP-legacy-header", "Write the MCNP 6.1.1 surface source header layout (80-character title card and short third record) instead of the MCNP 6.2/6.3 layout", { CLI_VALUELESS } };

    namespace {
        // Writes a string as a fixed-width Fortran character field, truncating or
        // padding with spaces as necessary.
        void writeFixedWidthString(ByteBuffer & buffer, const std::string & value, std::size_t width) {
            std::string padded = value.substr(0, width);
            padded.resize(width, ' ');
            buffer.writeString(padded);
        }
    }


    // Reader class implementation

    Reader::Reader(const std::string & fileName, const UserOptions & options)
    : PhaseSpaceFileReader("MCNP", fileName, options)
    {
        readHeader();
    }

    std::vector<CLICommand> Reader::getFormatSpecificCLICommands() {
        return {};
    }

    bool Reader::hasMoreParticles()
    {
        resyncHistoryStateAfterSeek();
        return PhaseSpaceFileReader::hasMoreParticles();
    }

    void Reader::resyncHistoryStateAfterSeek()
    {
        if (resyncingHistoryState_) return;

        // Sequential reading only ever parses the record last parsed (a peek followed
        // by a read) or the one after it; any other position means the base class
        // seeked via moveToParticle() and the history baseline is stale.
        std::uint64_t recordIndex = getParticlesRead(true);
        if (lastParsedRecordIndex_ != std::numeric_limits<std::uint64_t>::max()
            && (recordIndex == lastParsedRecordIndex_ || recordIndex == lastParsedRecordIndex_ + 1)) {
            return;
        }

        if (recordIndex == 0) {
            // (Back) at the start of the file: reset to the initial state; the base
            // class's first-particle rule provides the new-history flag
            lastParsedRecordIndex_ = std::numeric_limits<std::uint64_t>::max();
            previousHistoryNumber_ = 0;
            currentHistoryNumber_ = 0;
            return;
        }

        // Read the record before the seek target (without counting it) so that the
        // next particle's new-history comparison uses the correct baseline, then
        // restore the position and read statistics with a second seek
        resyncingHistoryState_ = true;
        try {
            PhaseSpaceFileReader::moveToParticle(recordIndex - 1);
            getNextParticle(false);
            PhaseSpaceFileReader::moveToParticle(recordIndex);
        } catch (...) {
            resyncingHistoryState_ = false;
            throw;
        }
        resyncingHistoryState_ = false;
    }

    void Reader::detectLayout()
    {
        // The supported file layouts begin as follows:
        //   MCNP6:  [4B or 8B marker: 8] "SF_00001" [marker: 8] [marker: 143|191] kods...
        //   MCNPX:  [4B or 8B marker: 163|167] kods...
        //   MCNP5:  [4B or 8B marker: 143] kods...
        // where kods must start with a printable ASCII character. The patterns are
        // probed in native byte order first and then in swapped byte order.
        constexpr std::size_t PROBE_SIZE = 36;
        if (getFileSize() < PROBE_SIZE) {
            throw std::runtime_error("Invalid MCNP surface source file (file is too small): " + getFileName());
        }

        for (int attempt = 0; ; ++attempt) {
            ByteBuffer probe = getHeaderData(PROBE_SIZE);
            auto u32at = [&probe](std::size_t off) { probe.moveTo(off); return static_cast<std::uint64_t>(probe.read<std::uint32_t>()); };
            auto u64at = [&probe](std::size_t off) { probe.moveTo(off); return probe.read<std::uint64_t>(); };
            auto printableAt = [&probe](std::size_t off) { probe.moveTo(off); byte ch = probe.read<byte>(); return ch >= 32 && ch < 127; };

            if (u32at(0) == 8 && u32at(12) == 8 && (u32at(16) == 143 || u32at(16) == 191) && printableAt(20)) {
                variant_ = MCNPVariant::MCNP6; markerWidth_ = 4; return;
            }
            if (u64at(0) == 8 && u64at(16) == 8 && (u64at(24) == 143 || u64at(24) == 191) && printableAt(32)) {
                variant_ = MCNPVariant::MCNP6; markerWidth_ = 8; return;
            }
            if ((u32at(0) == 163 || u32at(0) == 167) && printableAt(4)) {
                variant_ = MCNPVariant::MCNPX; markerWidth_ = 4; return;
            }
            if ((u64at(0) == 163 || u64at(0) == 167) && printableAt(8)) {
                variant_ = MCNPVariant::MCNPX; markerWidth_ = 8; return;
            }
            if (u32at(0) == 143 && printableAt(4)) {
                variant_ = MCNPVariant::MCNP5; markerWidth_ = 4; return;
            }
            if (u64at(0) == 143 && printableAt(8)) {
                variant_ = MCNPVariant::MCNP5; markerWidth_ = 8; return;
            }

            if (attempt == 1) {
                throw std::runtime_error("Invalid MCNP surface source file (unrecognized header layout): " + getFileName());
            }
            ByteOrder swappedOrder = (HOST_BYTE_ORDER == ByteOrder::LittleEndian) ? ByteOrder::BigEndian : ByteOrder::LittleEndian;
            setByteOrder(swappedOrder);
        }
    }

    void Reader::readHeader()
    {
        detectLayout();

        const std::uint64_t fileSize = getFileSize();
        std::size_t bufferedSize = static_cast<std::size_t>(std::min<std::uint64_t>(fileSize, 4096));
        ByteBuffer header = getHeaderData(bufferedSize);
        std::size_t offset = 0;

        // Makes sure that at least bytesNeeded more bytes of header data are buffered,
        // re-reading a larger portion of the file if necessary.
        auto ensureBuffered = [&](std::size_t bytesNeeded) {
            if (offset + bytesNeeded > bufferedSize) {
                if (offset + bytesNeeded > fileSize) {
                    throw std::runtime_error("Invalid MCNP surface source file (unexpected end of file while reading the header): " + getFileName());
                }
                bufferedSize = offset + bytesNeeded;
                header = getHeaderData(bufferedSize);
                header.moveTo(offset);
            }
        };

        // Reads one Fortran sequential record and returns its payload, validating
        // the leading and trailing record length markers (4 or 8 bytes wide).
        auto readMarker = [&]() -> std::uint64_t {
            ensureBuffered(markerWidth_);
            std::uint64_t value;
            if (markerWidth_ == 4) {
                value = header.read<std::uint32_t>();
            } else {
                value = header.read<std::uint64_t>();
            }
            offset += markerWidth_;
            return value;
        };
        auto readRecord = [&](const char * recordName) -> ByteBuffer {
            std::uint64_t recordLength = readMarker();
            if (recordLength > fileSize) {
                throw std::runtime_error("Invalid MCNP surface source file (oversized record in the " + std::string(recordName) + "): " + getFileName());
            }
            std::size_t payloadLength = static_cast<std::size_t>(recordLength);
            ensureBuffered(payloadLength + markerWidth_);
            ByteBuffer payload(header.readBytes(payloadLength), header.getByteOrder());
            offset += payloadLength;
            std::uint64_t trailingMarker = readMarker();
            if (trailingMarker != recordLength) {
                throw std::runtime_error("Invalid MCNP surface source file (mismatched record markers in the " + std::string(recordName) + "): " + getFileName());
            }
            return payload;
        };

        // Format record (MCNP6 only): id
        if (variant_ == MCNPVariant::MCNP6) {
            ByteBuffer formatRecord = readRecord("format record");
            std::string formatId = formatRecord.readString(formatRecord.length());
            if (formatId != MCNP_FORMAT_ID) {
                std::cerr << "Warning: unexpected MCNP surface source format identifier \"" << formatId
                          << "\" (expected " << MCNP_FORMAT_ID << "); attempting to read anyway: " << getFileName() << std::endl;
            }
        }

        // First record: kods, vers, lods, idtms, probs, aids, knods (informational only).
        // The field widths differ between the MCNP families, and the title card (aids)
        // is 80 characters in MCNP 6.1.1 and 128 characters in MCNP 6.2/6.3, so its
        // width is inferred from the record length.
        ByteBuffer firstRecord = readRecord("first record");
        if (variant_ == MCNPVariant::MCNPX) {
            // kods(8), vers(5), lods(28), idtms(19), probs(19), aids, knods
            constexpr std::size_t MCNPX_FIXED_FIELDS_LENGTH = 8 + 5 + 28 + 19 + 19 + 4;
            if (firstRecord.length() > MCNPX_FIXED_FIELDS_LENGTH) {
                std::size_t aidsLength = firstRecord.length() - MCNPX_FIXED_FIELDS_LENGTH;
                kods_ = firstRecord.readString(8);
                vers_ = firstRecord.readString(5);
                firstRecord.readString(28); // lods
                firstRecord.readString(19); // idtms
                firstRecord.readString(19); // probs
                aids_ = firstRecord.readString(aidsLength);
            }
        } else if (firstRecord.length() >= FIRST_RECORD_DATA_LENGTH_LEGACY) {
            // MCNP6 and MCNP5: kods(8), vers(5), lods(8), idtms(19), probs(19), aids, knods
            std::size_t aidsLength = firstRecord.length() - FIRST_RECORD_FIXED_FIELDS_LENGTH;
            kods_ = firstRecord.readString(8);
            vers_ = firstRecord.readString(5);
            firstRecord.readString(8);  // lods
            firstRecord.readString(19); // idtms
            firstRecord.readString(19); // probs
            aids_ = firstRecord.readString(aidsLength);
        }

        // Second record: np1, nrss, nrcd, njsw, niss. MCNP6 stores np1/nrss/niss as
        // 64-bit integers, MCNP5 stores np1/nrss as 64-bit and niss as 32-bit, and
        // MCNPX stores everything as 32-bit integers.
        ByteBuffer secondRecord = readRecord("second record");
        std::int64_t np1, nrss, niss;
        std::int32_t nrcd, njsw;
        if (variant_ == MCNPVariant::MCNPX) {
            if (secondRecord.length() != 20) {
                throw std::runtime_error("Unsupported MCNP surface source file (unexpected second record length): " + getFileName());
            }
            np1  = secondRecord.read<std::int32_t>();
            nrss = secondRecord.read<std::int32_t>();
            nrcd = secondRecord.read<std::int32_t>();
            njsw = secondRecord.read<std::int32_t>();
            niss = secondRecord.read<std::int32_t>();
        } else if (variant_ == MCNPVariant::MCNP5) {
            if (secondRecord.length() != SECOND_RECORD_DATA_LENGTH) {
                throw std::runtime_error("Unsupported MCNP surface source file (unexpected second record length): " + getFileName());
            }
            np1  = secondRecord.read<std::int64_t>();
            nrss = secondRecord.read<std::int64_t>();
            nrcd = secondRecord.read<std::int32_t>();
            njsw = secondRecord.read<std::int32_t>();
            niss = secondRecord.read<std::int32_t>();
        } else {
            if (secondRecord.length() != SECOND_RECORD_DATA_LENGTH) {
                throw std::runtime_error("Unsupported MCNP surface source file (unexpected second record length): " + getFileName());
            }
            np1  = secondRecord.read<std::int64_t>();
            nrss = secondRecord.read<std::int64_t>();
            nrcd = secondRecord.read<std::int32_t>();
            njsw = secondRecord.read<std::int32_t>();
            niss = secondRecord.read<std::int64_t>();
        }

        std::int32_t absNrcd = nrcd < 0 ? -nrcd : nrcd;
        if (absNrcd == 6) {
            throw std::runtime_error("Unsupported MCNP surface source file (spherically symmetric surface sources are not supported): " + getFileName());
        }
        if (absNrcd < 10 || absNrcd > 11 || (variant_ == MCNPVariant::MCNP6 && absNrcd == 10)) {
            throw std::runtime_error("Unsupported MCNP surface source file (nrcd = " + std::to_string(nrcd) + "; only the standard 10- or 11-value particle records are supported): " + getFileName());
        }
        particleRecordDataLength_ = static_cast<std::size_t>(absNrcd) * sizeof(double);
        if (nrss < 0 || niss < 0 || njsw < 0) {
            throw std::runtime_error("Invalid MCNP surface source file (negative track, history or surface count): " + getFileName());
        }

        // Third record: niwr, mipts, kjaq (only present when np1 is negative, which it
        // always should be). MCNP 6.1.1 writes just the three integers while MCNP 6.2/6.3
        // pad the record with 17 additional zero-valued integers.
        std::int32_t niwr = 0;
        std::int32_t mipts = 1;
        if (np1 < 0) {
            ByteBuffer thirdRecord = readRecord("third record");
            if (thirdRecord.length() >= sizeof(std::int32_t))     niwr  = thirdRecord.read<std::int32_t>();
            if (thirdRecord.length() >= 2 * sizeof(std::int32_t)) mipts = thirdRecord.read<std::int32_t>();
            // kjaq (macrobody facet flag) and any padding are not needed to read the particle records
        }
        if (niwr < 0 || mipts < 1) {
            throw std::runtime_error("Invalid MCNP surface source file (invalid cell or particle type count): " + getFileName());
        }

        // Fourth record: one surface definition record per surface/cell (contents not
        // needed). The count is computed in 64 bits since njsw and niwr both come from
        // the file and their int32 sum could otherwise overflow on a corrupt file; each
        // record occupies at least 8 bytes, which bounds any legitimate count.
        std::int64_t surfaceRecordCount = static_cast<std::int64_t>(njsw) + static_cast<std::int64_t>(niwr);
        if (surfaceRecordCount > static_cast<std::int64_t>(fileSize / 8)) {
            throw std::runtime_error("Invalid MCNP surface source file (surface and cell count exceeds file size): " + getFileName());
        }
        for (std::int64_t j = 0; j < surfaceRecordCount; ++j) {
            readRecord("surface record");
        }

        // Summary record: zero, nsl(:,:) (informational only; its length is checked
        // against the expected surface layout but variants are tolerated since the
        // particle records are located by their own length below)
        ByteBuffer summaryRecord = readRecord("summary record");
        std::size_t expectedSummaryLength = sizeof(double) + static_cast<std::size_t>(surfaceRecordCount) * (2 + 4 * static_cast<std::size_t>(mipts)) * sizeof(std::int32_t);
        if (summaryRecord.length() != expectedSummaryLength) {
            std::cerr << "Warning: MCNP surface source file has an unexpected summary record length ("
                      << summaryRecord.length() << " bytes, expected " << expectedSummaryLength
                      << "): " << getFileName() << std::endl;
        }

        // Some MCNP6 files carry additional records at the end of the header; skip a
        // few of these if the next record does not have the particle record length
        for (int extraRecords = 0; extraRecords < 3; ++extraRecords) {
            if (offset + markerWidth_ > fileSize) break; // end of file (no particle records)
            ensureBuffered(markerWidth_);
            std::uint64_t nextRecordLength = (markerWidth_ == 4)
                                           ? header.read<std::uint32_t>()
                                           : header.read<std::uint64_t>();
            header.moveTo(offset); // peek only
            if (nextRecordLength == particleRecordDataLength_) break;
            readRecord("trailing header record");
            std::cerr << "Warning: skipping an unexpected " << nextRecordLength
                      << "-byte record at the end of the MCNP surface source header: " << getFileName() << std::endl;
        }

        headerSize_ = offset;
        numberOfParticles_ = static_cast<std::uint64_t>(nrss);
        numberOfOriginalHistories_ = static_cast<std::uint64_t>(np1 < 0 ? -np1 : np1);
        numberOfRepresentedHistories_ = static_cast<std::uint64_t>(niss);
    }

    Particle Reader::readBinaryParticle(ByteBuffer & buffer)
    {
        std::uint64_t leadingMarker = (markerWidth_ == 4) ? buffer.read<std::uint32_t>() : buffer.read<std::uint64_t>();
        if (leadingMarker != particleRecordDataLength_) {
            throw std::runtime_error("Invalid MCNP surface source file (unexpected particle record marker).");
        }

        double a   = buffer.read<double>();
        double b   = buffer.read<double>();
        double wgt = buffer.read<double>();
        double erg = buffer.read<double>();
        double tme = buffer.read<double>();
        double x   = buffer.read<double>();
        double y   = buffer.read<double>();
        double z   = buffer.read<double>();
        double u   = buffer.read<double>();
        double v   = buffer.read<double>();
        double c   = (particleRecordDataLength_ == PARTICLE_RECORD_DATA_LENGTH) ? buffer.read<double>() : 0.0;

        std::uint64_t trailingMarker = (markerWidth_ == 4) ? buffer.read<std::uint32_t>() : buffer.read<std::uint64_t>();
        if (trailingMarker != leadingMarker) {
            throw std::runtime_error("Invalid MCNP surface source file (mismatched particle record markers).");
        }

        // b packs the particle type (and, for MCNPX/MCNP5, the surface number) and
        // carries the sign of the third direction cosine
        long long bPacked = std::llround(std::fabs(b));
        ParticleType type = ParticleType::Unsupported;
        long long packedSurface = -1; // MCNPX/MCNP5 pack the surface into b instead of c
        switch (variant_) {
            case MCNPVariant::MCNP6:
                type = getParticleTypeFromSSWType(bPacked);
                break;
            case MCNPVariant::MCNPX:
                packedSurface = bPacked % 1000000;
                type = getParticleTypeFromMCNPXType(bPacked / 1000000);
                break;
            case MCNPVariant::MCNP5: {
                // |b| = 8 * (ipt * 1e6 + surface); some variants scale the type by a
                // further factor of 100, which is folded away when present
                long long quot = bPacked / 8;
                packedSurface = quot % 1000000;
                long long mcnp5Type = quot / 1000000;
                if (mcnp5Type >= 100) mcnp5Type /= 100;
                type = (mcnp5Type == 1) ? ParticleType::Neutron
                     : (mcnp5Type == 2) ? ParticleType::Photon
                     : (mcnp5Type == 3) ? ParticleType::Electron
                     : ParticleType::Unsupported; // MCNP5 transports only neutrons, photons and electrons
                break;
            }
        }
        if (type == ParticleType::Unsupported) {
            throw std::runtime_error("MCNP particle type field " + std::to_string(bPacked) + " could not be decoded to a supported particle type.");
        }

        double w = calcThirdUnitComponent(u, v);
        if (b < 0) w = -w;

        // a holds the history number of the particle (negative for uncollided particles).
        // The history bookkeeping is keyed to the record index so that peeking at a
        // particle does not disturb the new-history detection.
        std::int64_t historyNumber = std::llround(std::fabs(a));
        std::uint64_t recordIndex = getParticlesRead(true);
        if (recordIndex != lastParsedRecordIndex_) {
            previousHistoryNumber_ = currentHistoryNumber_;
            lastParsedRecordIndex_ = recordIndex;
        }
        currentHistoryNumber_ = historyNumber;
        std::int64_t deltaHistories = historyNumber - previousHistoryNumber_;
        bool isNewHistory = deltaHistories != 0;

        Particle particle(type, erg * MeV, x * cm, y * cm, z * cm, u, v, w, isNewHistory, wgt);

        if (deltaHistories > 0) {
            std::int64_t incrementalHistories = std::min<std::int64_t>(deltaHistories, std::numeric_limits<std::int32_t>::max());
            particle.setIncrementalHistories(static_cast<std::uint32_t>(incrementalHistories));
        }

        particle.setFloatProperty(FloatPropertyType::TIME, static_cast<float>(tme) * shake);

        if (variant_ == MCNPVariant::MCNP6) {
            // c holds the problem name of the recording surface; for a macrobody the
            // facet number is encoded as the first decimal (e.g. 4.2 is facet 2 of
            // macrobody 4). They are stored as two exact integer properties.
            double cAbs = std::fabs(c);
            std::int64_t surfaceNumber = static_cast<std::int64_t>(cAbs);
            std::int32_t facet = static_cast<std::int32_t>(std::llround((cAbs - static_cast<double>(surfaceNumber)) * 10.0));
            if (facet >= 10) { surfaceNumber++; facet = 0; } // guard against representation noise just below a whole number
            particle.setIntProperty(IntPropertyType::MCNP_SURFACE_ID, static_cast<std::int32_t>(surfaceNumber));
            if (facet != 0) {
                particle.setIntProperty(IntPropertyType::MCNP_MACROBODY_FACET, facet);
            }
        } else {
            // MCNPX and MCNP5 pack the surface number into the b value instead
            particle.setIntProperty(IntPropertyType::MCNP_SURFACE_ID, static_cast<std::int32_t>(packedSurface));
        }

        particle.setBoolProperty(BoolPropertyType::IS_UNCOLLIDED, a < 0);

        return particle;
    }


    // Writer class implementation

    namespace {
        // Resolves the format to write and rejects invalid or conflicting options.
        // Called before the base class is constructed (in a delegating constructor
        // argument) so that an option error cannot leave a partially constructed
        // writer behind.
        MCNPVariant parseWriteVariant(const UserOptions & options) {
            MCNPVariant variant = MCNPVariant::MCNP6;
            if (options.contains(MCNPFormatCommand)) {
                std::string formatName = std::get<std::string>(options.at(MCNPFormatCommand)[0]);
                if (formatName == "MCNP6")      variant = MCNPVariant::MCNP6;
                else if (formatName == "MCNP5") variant = MCNPVariant::MCNP5;
                else if (formatName == "MCNPX") variant = MCNPVariant::MCNPX;
                else throw std::runtime_error("Invalid MCNP surface source format specified: " + formatName + " (use MCNP6, MCNP5 or MCNPX)");
            }
            if (variant != MCNPVariant::MCNP6 && options.contains(MCNPLegacyHeaderCommand)) {
                throw std::runtime_error("The MCNP-legacy-header option only applies to the MCNP6 format.");
            }
            return variant;
        }
    }

    Writer::Writer(const std::string & fileName, const UserOptions & options)
    : Writer(fileName, options, parseWriteVariant(options))
    {}

    Writer::Writer(const std::string & fileName, const UserOptions & options, MCNPVariant writeVariant)
    : PhaseSpaceFileWriter("MCNP", fileName, options),
      title_("Surface source written by " + Version::GetVersionString()),
      surfaceNumber_(99999),
      writeVariant_(writeVariant)
    {
        if (options.contains(MCNPTitleCommand)) {
            title_ = std::get<std::string>(options.at(MCNPTitleCommand)[0]);
        }
        surfaceNumber_ = options.extractUIntOption(MCNPSurfaceCommand, 99999u);
        legacyHeader_ = options.contains(MCNPLegacyHeaderCommand);
    }

    std::vector<CLICommand> Writer::getFormatSpecificCLICommands() {
        return { MCNPTitleCommand, MCNPSurfaceCommand, MCNPFormatCommand, MCNPLegacyHeaderCommand };
    }

    void Writer::writeHeaderData(ByteBuffer & buffer)
    {
        std::int64_t nrss = static_cast<std::int64_t>(getParticlesWritten());
        std::int64_t niss = static_cast<std::int64_t>(representedHistories_);
        if (nrss > 0 && niss == 0) niss = 1; // particles written without new-history flags still represent one history
        std::int64_t np1  = -std::max<std::int64_t>(static_cast<std::int64_t>(getHistoriesWritten()), 1); // must be negative so that the third record is read

        // Format record (MCNP6 only): id
        if (writeVariant_ == MCNPVariant::MCNP6) {
            buffer.write<std::uint32_t>(static_cast<std::uint32_t>(FORMAT_RECORD_DATA_LENGTH));
            buffer.writeString(std::string(MCNP_FORMAT_ID));
            buffer.write<std::uint32_t>(static_cast<std::uint32_t>(FORMAT_RECORD_DATA_LENGTH));
        }

        // First record: kods, vers, lods, idtms, probs, aids, knods.
        // MCNP6 and MCNP5 use an 8-character lods field with an 80-character title
        // card (128 characters in the MCNP 6.2/6.3 layout); MCNPX uses a 28-character
        // lods field with an 80-character title card.
        std::string version = std::to_string(Version::MAJOR_VERSION) + "." + std::to_string(Version::MINOR_VERSION) + "." + std::to_string(Version::PATCH_VERSION);
        char dateTime[20] = {};
        std::time_t now = std::time(nullptr);
        std::tm * localNow = std::localtime(&now);
        if (localNow == nullptr || std::strftime(dateTime, sizeof(dateTime), "%m/%d/%y %H:%M:%S", localNow) == 0) {
            dateTime[0] = '\0';
        }
        std::string probs = std::filesystem::path(getFileName()).stem().string();

        std::uint32_t firstRecordLength;
        std::size_t lodsLength = 8;
        switch (writeVariant_) {
            case MCNPVariant::MCNP5: firstRecordLength = static_cast<std::uint32_t>(FIRST_RECORD_DATA_LENGTH_LEGACY); break;
            case MCNPVariant::MCNPX: firstRecordLength = static_cast<std::uint32_t>(FIRST_RECORD_DATA_LENGTH_MCNPX); lodsLength = 28; break;
            default:                 firstRecordLength = static_cast<std::uint32_t>(legacyHeader_ ? FIRST_RECORD_DATA_LENGTH_LEGACY : FIRST_RECORD_DATA_LENGTH_MODERN); break;
        }
        std::size_t aidsLength = firstRecordLength - lodsLength - (8 + 5 + 19 + 19 + 4);
        buffer.write<std::uint32_t>(firstRecordLength);
        writeFixedWidthString(buffer, "PartZoo", 8);            // kods
        writeFixedWidthString(buffer, version, 5);              // vers
        writeFixedWidthString(buffer, std::string(dateTime).substr(0, 8), lodsLength); // lods
        writeFixedWidthString(buffer, dateTime, 19);            // idtms
        writeFixedWidthString(buffer, probs, 19);               // probs
        writeFixedWidthString(buffer, title_, aidsLength);      // aids
        buffer.write<std::int32_t>(1);                          // knods
        buffer.write<std::uint32_t>(firstRecordLength);

        // Second record: np1, nrss, nrcd, njsw, niss. MCNP6 stores np1/nrss/niss as
        // 64-bit integers, MCNP5 stores np1/nrss as 64-bit and niss as 32-bit, and
        // MCNPX stores everything as 32-bit integers.
        auto clampToInt32 = [](std::int64_t value) {
            return static_cast<std::int32_t>(std::max<std::int64_t>(std::numeric_limits<std::int32_t>::min(),
                                             std::min<std::int64_t>(value, std::numeric_limits<std::int32_t>::max())));
        };
        if (writeVariant_ == MCNPVariant::MCNPX) {
            buffer.write<std::uint32_t>(static_cast<std::uint32_t>(SECOND_RECORD_DATA_LENGTH_MCNPX));
            buffer.write<std::int32_t>(clampToInt32(np1));
            buffer.write<std::int32_t>(clampToInt32(nrss));
            buffer.write<std::int32_t>(11);                     // nrcd
            buffer.write<std::int32_t>(1);                      // njsw
            buffer.write<std::int32_t>(clampToInt32(niss));
            buffer.write<std::uint32_t>(static_cast<std::uint32_t>(SECOND_RECORD_DATA_LENGTH_MCNPX));
        } else if (writeVariant_ == MCNPVariant::MCNP5) {
            buffer.write<std::uint32_t>(static_cast<std::uint32_t>(SECOND_RECORD_DATA_LENGTH));
            buffer.write<std::int64_t>(np1);
            buffer.write<std::int64_t>(nrss);
            buffer.write<std::int32_t>(11);                     // nrcd
            buffer.write<std::int32_t>(1);                      // njsw
            buffer.write<std::int32_t>(clampToInt32(niss));
            buffer.write<std::int32_t>(0);                      // padding
            buffer.write<std::uint32_t>(static_cast<std::uint32_t>(SECOND_RECORD_DATA_LENGTH));
        } else {
            buffer.write<std::uint32_t>(static_cast<std::uint32_t>(SECOND_RECORD_DATA_LENGTH));
            buffer.write<std::int64_t>(np1);
            buffer.write<std::int64_t>(nrss);
            buffer.write<std::int32_t>(-11);                    // nrcd
            buffer.write<std::int32_t>(1);                      // njsw
            buffer.write<std::int64_t>(niss);
            buffer.write<std::uint32_t>(static_cast<std::uint32_t>(SECOND_RECORD_DATA_LENGTH));
        }

        // Third record: niwr, mipts, kjaq (padded with 17 zero-valued integers in the
        // MCNP 6.2/6.3 layout)
        std::uint32_t thirdRecordLength = static_cast<std::uint32_t>((writeVariant_ == MCNPVariant::MCNP6 && !legacyHeader_) ? THIRD_RECORD_DATA_LENGTH_MODERN : THIRD_RECORD_DATA_LENGTH_LEGACY);
        buffer.write<std::uint32_t>(thirdRecordLength);
        buffer.write<std::int32_t>(0);                          // niwr
        buffer.write<std::int32_t>(1);                          // mipts
        buffer.write<std::int32_t>(0);                          // kjaq
        for (std::size_t i = 3 * sizeof(std::int32_t); i < thirdRecordLength; i += sizeof(std::int32_t)) {
            buffer.write<std::int32_t>(0);                      // padding
        }
        buffer.write<std::uint32_t>(thirdRecordLength);

        // Fourth record: a single placeholder SO surface (sphere at the origin)
        buffer.write<std::uint32_t>(static_cast<std::uint32_t>(FOURTH_RECORD_DATA_LENGTH));
        buffer.write<std::int32_t>(static_cast<std::int32_t>(surfaceNumber_)); // jss
        buffer.write<std::int32_t>(5);                          // kst (SO surface)
        buffer.write<std::int32_t>(1);                          // n (number of coefficients)
        buffer.write<double>(-1.0);                             // scf (placeholder radius)
        buffer.write<std::uint32_t>(static_cast<std::uint32_t>(FOURTH_RECORD_DATA_LENGTH));

        // Summary record: zero, nsl(:,:) (informational only)
        std::int32_t trackCount = static_cast<std::int32_t>(std::min<std::int64_t>(nrss, std::numeric_limits<std::int32_t>::max()));
        buffer.write<std::uint32_t>(static_cast<std::uint32_t>(SUMMARY_RECORD_DATA_LENGTH));
        buffer.write<double>(0.0);
        buffer.write<std::int32_t>(trackCount);
        buffer.write<std::int32_t>(-999);
        buffer.write<std::int32_t>(trackCount);
        buffer.write<std::int32_t>(-999);
        buffer.write<std::int32_t>(-999);
        buffer.write<std::int32_t>(-999);
        buffer.write<std::uint32_t>(static_cast<std::uint32_t>(SUMMARY_RECORD_DATA_LENGTH));
    }

    void Writer::writeBinaryParticle(ByteBuffer & buffer, Particle & particle)
    {
        // Resolve the surface number (used for c and, in the MCNP5/MCNPX formats,
        // packed into the type field)
        long long surface = particle.hasIntProperty(IntPropertyType::MCNP_SURFACE_ID)
                          ? static_cast<long long>(particle.getIntProperty(IntPropertyType::MCNP_SURFACE_ID))
                          : static_cast<long long>(surfaceNumber_);
        if (surface < 0) surface = -surface;

        // Encode the particle type (and, for MCNP5/MCNPX, the surface) into |b|
        ParticleType type = particle.getType();
        long long bPacked = 0;
        switch (writeVariant_) {
            case MCNPVariant::MCNP6:
                bPacked = getSSWTypeFromParticleType(type);
                break;
            case MCNPVariant::MCNPX: {
                long long mcnpxType = getMCNPXTypeFromParticleType(type);
                if (mcnpxType != 0) {
                    if (surface >= 1000000) {
                        throw std::runtime_error("Surface number " + std::to_string(surface) + " is too large for the MCNPX surface source format (maximum 999999).");
                    }
                    bPacked = surface + 1000000 * mcnpxType;
                }
                break;
            }
            case MCNPVariant::MCNP5: {
                long long mcnp5Type = (type == ParticleType::Neutron)  ? 1
                                    : (type == ParticleType::Photon)   ? 2
                                    : (type == ParticleType::Electron) ? 3
                                    : 0; // MCNP5 transports only neutrons, photons and electrons
                if (mcnp5Type != 0) {
                    if (surface >= 1000000) {
                        throw std::runtime_error("Surface number " + std::to_string(surface) + " is too large for the MCNP5 surface source format (maximum 999999).");
                    }
                    // |b| = 8 * (ipt * 1e6 + surface)
                    bPacked = 8 * (surface + 1000000 * mcnp5Type);
                }
                break;
            }
        }
        if (bPacked == 0) {
            throw std::runtime_error("Particle type " + std::string(getParticleTypeName(particle.getType())) + " is not supported by the selected MCNP surface source format.");
        }

        if (particle.isNewHistory()) {
            currentHistoryNumber_ += particle.getIncrementalHistories();
            representedHistories_++;
        }
        if (currentHistoryNumber_ == 0) currentHistoryNumber_ = 1;

        double a = static_cast<double>(currentHistoryNumber_);
        if (particle.hasBoolProperty(BoolPropertyType::IS_UNCOLLIDED) && particle.getBoolProperty(BoolPropertyType::IS_UNCOLLIDED)) {
            a = -a;
        }

        double u = particle.getDirectionalCosineX();
        double v = particle.getDirectionalCosineY();
        double w = particle.getDirectionalCosineZ();

        // Guard against float rounding pushing u^2 + v^2 above 1 in double precision,
        // which would make the reconstruction of w fail on the MCNP side
        double uuvv = u * u + v * v;
        if (uuvv > 1.0) {
            double normFactor = 1.0 / std::sqrt(uuvv);
            u *= normFactor;
            v *= normFactor;
        }

        // std::signbit is used instead of a < 0 comparison so that the sign of an
        // exactly tangential direction (w = -0.0) survives a read/write round trip
        double b = static_cast<double>(bPacked);
        if (std::signbit(w)) b = -b;

        constexpr float inv_cm = 1.0f / cm;
        constexpr float inv_MeV = 1.0f / MeV;
        constexpr float inv_shake = 1.0f / shake;

        double tme = particle.hasFloatProperty(FloatPropertyType::TIME)
                   ? static_cast<double>(particle.getFloatProperty(FloatPropertyType::TIME) * inv_shake)
                   : 0.0;

        double c = static_cast<double>(surface);
        if (writeVariant_ == MCNPVariant::MCNP6 && particle.hasIntProperty(IntPropertyType::MCNP_MACROBODY_FACET)) {
            c += static_cast<double>(particle.getIntProperty(IntPropertyType::MCNP_MACROBODY_FACET)) / 10.0;
        }

        buffer.write<std::uint32_t>(static_cast<std::uint32_t>(PARTICLE_RECORD_DATA_LENGTH));
        buffer.write<double>(a);
        buffer.write<double>(b);
        buffer.write<double>(particle.getWeight());
        buffer.write<double>(particle.getKineticEnergy() * inv_MeV);
        buffer.write<double>(tme);
        buffer.write<double>(particle.getX() * inv_cm);
        buffer.write<double>(particle.getY() * inv_cm);
        buffer.write<double>(particle.getZ() * inv_cm);
        buffer.write<double>(u);
        buffer.write<double>(v);
        buffer.write<double>(c);
        buffer.write<std::uint32_t>(static_cast<std::uint32_t>(PARTICLE_RECORD_DATA_LENGTH));
    }

}
