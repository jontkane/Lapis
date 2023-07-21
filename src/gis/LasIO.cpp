#include"gis_pch.hpp"
#include"GisExceptions.hpp"
#include"LasIO.hpp"

namespace lapis {
	LasIO::LasIO(const std::string& filename) : _ifs(std::make_unique<std::ifstream>(filename, std::ios::binary)) {
		
		std::ifstream& ifs = *_ifs;
		if (!ifs) {
			throw lapis::InvalidLasFileException("Unable to open file " + filename);
		}

		_readHeader();

		

		ifs.seekg(header.HeaderSize, std::ios_base::beg);

		_readVLRs<std::uint16_t>(header.NumberOfVLRs, header.OffsetToPointData);


		if (header.NumberOfEVLRs != 0) {
			ifs.seekg(header.StartOfEVLRs, std::ios_base::beg);
			_readVLRs<std::uint64_t>(header.NumberOfEVLRs);
		}

		if (header.isCompressed()) {
			_initChunks();
		}

		std::streampos offset = header.isCompressed() ? header.OffsetToPointData + sizeof(uint64_t) : header.OffsetToPointData;
		_ifs->seekg(offset, std::ios_base::beg);

		_pointInChunk = 0;
		_currentChunk = nullptr;
	}

	void LasIO::_readHeader() {
		std::ifstream& ifs = *_ifs;
		std::array<char, 4> filesignature{};
		ifs.read(filesignature.data(), 4); //File Signature ("LASF")
		std::string filesig_str{ filesignature.data(), 4 };
		if (filesig_str != "LASF") {
			throw lapis::InvalidLasFileException("Not a LAS/LAZ file");
		}

		const std::streamsize skipToGlobalEncoding = 2ll; //File Source ID
		ifs.seekg(skipToGlobalEncoding, std::ios_base::cur);
		_readBytes(&header.GlobalEncoding);

		constexpr std::streamsize skipToVersion = 4ll //Project ID - GUID data 1
			+ 2ll //Project ID - GUID data 2
			+ 2ll //Project ID - GUID data 3
			+ 8ll //Project IS - GUID data 4
			;
		ifs.seekg(skipToVersion, std::ios_base::cur);
		_readBytes(&header.VersionMajor); //Version Major
		_readBytes(&header.VersionMinor); //Version Minor

		if (header.VersionMajor > 1 || header.VersionMinor > 4) { //neither 2.0 nor 1.5 exist yet, better to fail gracefully if they come out than trip on our faces
			throw lapis::InvalidLasFileException("Unsupported LAS format");
		}

		constexpr std::streamsize skipToYear = 32ll //System identifier
			+ 32ll //Generating software
			+ 2ll; //File creation day of year
		
		ifs.seekg(skipToYear, std::ios_base::cur);
		_readBytes(&header.FileCreationYear); //File creation year
		_readBytes(&header.HeaderSize); //Header Size
		_readBytes(&header.OffsetToPointData); //Offset to point data
		_readBytes(&header.NumberOfVLRs); //Number of Variable Length Records
		_readBytes(&header.ModifiedPointFormat); //point data record format
		_readBytes(&header.PointLength); //point data record length
		_readBytes(&header.LegacyNumberOfPoints); //Legacy Number of point records

		constexpr std::streamsize skipToXScale = 20ll; //Legacy number of points by return
		ifs.seekg(skipToXScale, std::ios::cur);
		_readBytes(&header.ScaleFactor.x); //X scale factor
		_readBytes(&header.ScaleFactor.y); //Y scale factor
		_readBytes(&header.ScaleFactor.z); //Z scale factor
		_readBytes(&header.Offset.x); //X offset
		_readBytes(&header.Offset.y); //Y offset
		_readBytes(&header.Offset.z); //Z offset
		_readBytes(&header.xmax); //Max X
		_readBytes(&header.xmin); //Min X
		_readBytes(&header.ymax); //Max Y
		_readBytes(&header.ymin); //Min y
		_readBytes(&header.zmax); //Max Z
		_readBytes(&header.zmin); //Min Z

		if (header.VersionMinor >= 4) { //technically 1.3 has the start of waveform entry but who cares
			const std::streamsize skipToEVLRStart = 8ll; //Start of waveform data packet record

			ifs.seekg(skipToEVLRStart, std::ios_base::cur);
			_readBytes(&header.StartOfEVLRs); //Start of first Extended Variable Length Record
			_readBytes(&header.NumberOfEVLRs); //Number of Extended Variable Length Records
			_readBytes(&header.NewNumberOfPoints); //Number of point records
		}
	}

	void LasIO::_initChunks() {

		//this function is copied with minimal edits from PDAL's LazPerfVlrCompression.cpp file

		std::ifstream& ifs = *_ifs;

		ifs.seekg(header.OffsetToPointData, std::ios_base::beg);
		uint64_t chunkTablePos;
		_readBytes(&chunkTablePos);

		ifs.seekg(chunkTablePos, std::ios_base::beg);
		uint32_t version;
		uint32_t numChunks;
		_readBytes(&version);
		_readBytes(&numChunks);

		if (version != 0) {
			throw InvalidLasFileException("Unsupported laz chunk type");
		}

		bool variable = vlrs.compressionInfo.chunk_size == lazperf::VariableChunkSize;
		if (numChunks) {
			lazperf::InputCb cb{ std::bind(&LasIO::getBytes,this, std::placeholders::_1,std::placeholders::_2) };
			_chunks = lazperf::decompress_chunk_table(cb, numChunks, variable);
		}

		if (!variable)
		{
			uint64_t remaining = header.NumberOfPoints();
			for (lazperf::chunk& chunk : _chunks)
			{
				chunk.count = (std::min)((uint64_t)vlrs.compressionInfo.chunk_size, remaining);
				remaining -= chunk.count;
			}
		}
	}

	template<class T>
	void LasIO::_readVLRs(std::uint32_t nVLR, std::uint32_t pointOffset)
	{
		std::ifstream& ifs = *_ifs;
		for (std::uint32_t i = 0; i < nVLR; ++i) {
			if (ifs.eof() || ifs.tellg() >= pointOffset) { //if the number of VLRs lies
				break;
			}
			const std::streamsize skipToUserID = 2ll; //Reserved
			ifs.seekg(skipToUserID, std::ios_base::cur);
			std::array<char, 16> userID{};
			ifs.read(userID.data(), 16); //User ID

			uint16_t recordID;
			T recordLength = 0;
			_readBytes(&recordID);
			_readBytes(&recordLength);

			const std::streamsize skipToVLR = 32ll; //Description
			ifs.seekg(skipToVLR, std::ios_base::cur);

			const std::string projectionUserID = "LASF_Projection";
			if (!std::strcmp(projectionUserID.data(), userID.data())) {
				if (header.isWKT()) {
					if (recordID == 2111 || recordID == 2112) { //VLR is WKT format
						std::vector<char> vlr = std::vector<char>(recordLength);
						ifs.read(vlr.data(), recordLength);
						vlrs.wkt = std::string(vlr.data(), recordLength);
						continue;
					}
				}
				else {
					if (recordID == gtifKeysCode) {
						if (recordLength % sizeof(gtifKey) != 0) {
							throw InvalidLasFileException("Issue with GeoTiff CRS in LAS header");
						}
						vlrs.gtifKeys.resize(recordLength / sizeof(gtifKey));
						ifs.read((char*)vlrs.gtifKeys.data(), recordLength);

						//if (vlrs.gtifKeys[0].header.numberOfKeys * sizeof(gtifKey) + sizeof(gtifKey) != recordLength) {
						//	throw InvalidLasFileException("Issue with GeoTiff CRS in LAS header");
						//}
						continue;
					}
					if (recordID == gtifDoublesCode) {
						vlrs.gtifDoubleParams.resize(recordLength);
						ifs.read(vlrs.gtifDoubleParams.data(), recordLength);
						continue;
					}
					if (recordID == gtifAsciiCode) {
						vlrs.gtifAsciiParams.resize(recordLength);
						ifs.read(vlrs.gtifAsciiParams.data(), recordLength);
						continue;
					}
				}
			}

			const std::string lazUserID = "laszip encoded";
			if (!std::strcmp(lazUserID.data(), userID.data()) && recordID == 22204) {
				vlrs.compressionInfo.read(ifs);
				if ((header.PointFormat() <= 5 && vlrs.compressionInfo.compressor != 2) ||
					(header.PointFormat() > 5 && vlrs.compressionInfo.compressor != 3)) {
					throw InvalidLasFileException("Unsupported compression format");
				}
				continue;
			}

			ifs.seekg(recordLength, std::ios_base::cur);
		}
	}

	template void LasIO::_readVLRs<std::uint16_t>(std::uint32_t nVLR, std::uint32_t pointOffset);
	template void LasIO::_readVLRs<std::uint64_t>(std::uint32_t nVLR, std::uint32_t pointOffset);

	void LasIO::getBytes(unsigned char* buffer, size_t count)
	{
		_ifs->read((char*)buffer, count);
	}

	void LasIO::readPoint(char* buffer)
	{

		//this implementation is copied with minimal modification from lazperf in readers.cpp

		if (!header.isCompressed()) {
			_ifs->read(buffer, header.PointLength);
		} else {
			if (!_decompressor || _pointInChunk == _currentChunk->count)
			{
				lazperf::InputCb cb{ std::bind(&LasIO::getBytes,this, std::placeholders::_1,std::placeholders::_2) };
				_decompressor = lazperf::build_las_decompressor(cb, header.PointFormat(), header.extraBytes());

				// reset chunk state
				if (_currentChunk == nullptr)
					_currentChunk = _chunks.data();
				else
					_currentChunk++;
				_pointInChunk = 0;
			}

			_decompressor->decompress(buffer);
			_pointInChunk++;
		}
	}
}