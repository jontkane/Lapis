#pragma once
#ifndef lapis_lasheader_h
#define lapis_lasheader_h

#include"..\LapisTypeDefs.hpp"
#include"GeoTiffWrapper.hpp"

//This class implements the reading of the header of a las or laz file without the full overhead of using laszip
//If you just need the extent or projection or something like that, this should be a lot faster (hopefully)
//Not all elements of the header are read, in the interest of speed

namespace lapis {


	struct LasHeader {
		//char FileSignature[4];
		//uint16_t FileSourceID;
		uint16_t GlobalEncoding = 0;
		//uint32_t GUIDData1;
		//uint16_t GUIDData2;
		//uint16_t GUIDData3;
		//char GUIDData4[8];
		uint8_t VersionMajor = 0;
		uint8_t VersionMinor = 0;
		//char SystemIdentifier[32];
		//char GeneratingSoftware[32];
		//uint16_t FileCreationDayOfYear;
		uint16_t FileCreationYear = 0;
		uint16_t HeaderSize = 0;
		uint32_t OffsetToPointData = 0;
		uint32_t NumberOfVLRs = 0;
		uint8_t ModifiedPointFormat = 0;
		uint16_t PointLength = 0;
		uint32_t LegacyNumberOfPoints = 0;
		//uint32_t LegacyNumberOfPointsByReturn[5];
		struct {
			double x = 0;
			double y = 0;
			double z = 0;
		} ScaleFactor;
		struct {
			double x = 0;
			double y = 0;
			double z = 0;
		} Offset;
		double xmax = 0;
		double xmin = 0;
		double ymax = 0;
		double ymin = 0;
		double zmax = 0;
		double zmin = 0;
		//uint64_t StartOfWaveform;
		uint64_t StartOfEVLRs = 0;
		uint32_t NumberOfEVLRs = 0;
		uint64_t NewNumberOfPoints = 0;
		//uint64_t NewNumberOfPointsByReturn[15];

		uint64_t NumberOfPoints() const {
			return NewNumberOfPoints ? NewNumberOfPoints : LegacyNumberOfPoints;
		}

		bool isWKT() const {
			constexpr std::uint16_t wktbit = 0b0000000000010000;
			return GlobalEncoding & wktbit;
		}

		uint8_t PointFormat() const {
			//compressed points are indicated by adding 128 to the point format
			constexpr std::uint8_t removecompressedbit = 0b01111111;
			return ModifiedPointFormat & removecompressedbit;
		}

		bool isCompressed() const {
			//compressed points are indicated by adding 128 to the point format
			constexpr std::uint8_t compressedbit = 0b10000000;
			return ModifiedPointFormat & compressedbit;
		}

		inline static constexpr uint16_t point10size = 20;
		inline static constexpr uint16_t gpstimesize = 8;
		inline static constexpr uint16_t rgbsize = 6;
		inline static constexpr uint16_t waveformsize = 29;
		inline static constexpr uint16_t point14size = 30;
		inline static constexpr uint16_t nirsize = 2;

		inline static const std::vector<uint16_t> defaultPointSizes = {
			point10size, //type 0
			point10size + gpstimesize, //type 1
			point10size + rgbsize, //type 2
			point10size + gpstimesize + rgbsize, //type 3
			point10size + gpstimesize + waveformsize, //type 4
			point10size + gpstimesize + rgbsize + waveformsize, //type 5
			point14size, //type 6
			point14size + rgbsize, //type 7
			point14size + rgbsize + nirsize, //type 8
			point14size + waveformsize, //type 9
			point14size + waveformsize + rgbsize + nirsize //type 10
		};

		uint16_t extraBytes() const {
			return PointLength - defaultPointSizes[PointFormat()];
		}
	};

	//for now, none of the stuff from point types 1-5 are recorded
#pragma pack(push)
#pragma pack(1)
	struct LasPoint10 {
		int32_t x, y, z;
		uint16_t intensity;
		uint8_t middleBits;
		uint8_t classAndFlags;
		int8_t rawScanAngle;
		uint8_t userData;
		uint16_t pointSourceID;

		uint8_t returnNumber() const {
			//bits 0 through 2 of middleBits
			return middleBits & 0b00000111;
		}
		uint8_t numberOfReturns() const {
			//bits 3 through 5 of middleBits
			return (middleBits >> 3) & 0b00000111;
		}
		bool scanDirectionFlag() const {
			//bit 6 of middle bits
			return middleBits & 0b01000000;
		}
		bool edgeOfFlightline() const {
			//bit 7 of middle bits
			return middleBits & 0b10000000;
		}

		//this is provided for compatibility with point14
		double scanAngle() const {
			return rawScanAngle;
		}

		uint8_t classification() const {
			//bits 0 through 4 of classAndFlags
			return classAndFlags  & 0b00011111;
		}
		bool synthetic() const {
			//bit 5 of classAndFlags
			return classAndFlags & 0b00100000;
		}
		bool keypoint() const {
			//bit 6 of classAndFlags
			return classAndFlags & 0b01000000;
		}
		bool withheld() const {
			//bit 7 of classAndFlags
			return classAndFlags & 0b10000000;
		}
	};
#pragma pack(pop)

#pragma pack(push)
#pragma pack(1)
	struct LasPoint14 {
		int32_t x, y, z;
		uint16_t intensity;
		uint8_t returnByte;
		uint8_t flagByte;
		uint8_t rawClassification;
		uint8_t userData;
		int16_t rawScanAngle;
		uint16_t pointSourceID;
		double gpsTime;

		uint8_t returnNumber() const {
			return returnByte & 0b00001111;
		}
		uint8_t numberOfReturns() const {
			return returnByte >> 4;
		}

		bool synthetic() const {
			return flagByte & 0b00000001;
		}
		bool keypoint() const {
			return flagByte & 0b00000010;
		}
		bool withheld() const {
			return flagByte & 0b00000100;
		}
		bool overlap() const {
			return flagByte & 0b00001000;
		}
		uint8_t scannerChannel() const {
			return (flagByte >> 4) & 0b00000011;
		}
		bool scanDirectionFlag() const {
			return flagByte & 0b01000000;
		}
		bool edgeOfFlightline() const {
			return flagByte & 0b10000000;
		}

		//this function isn't strictly necessary, but is provided for compatibility with point10
		uint8_t classification() const {
			return rawClassification;
		}

		double scanAngle() const {
			return (double)rawScanAngle * 0.006;
		}
	};
#pragma pack(pop)

	//Storage for the VLRs we care about
	struct LasVLRs {

		std::string wkt;
		std::vector<gtifKey> gtifKeys;
		std::vector<char> gtifDoubleParams;
		std::vector<char> gtifAsciiParams;
		lazperf::laz_vlr compressionInfo;
	};

	class LasIO {
	public:

		LasIO() = default;
		LasIO(const std::string& filename);

		LasHeader header;
		LasVLRs vlrs;

		void readPoint(char* buffer);

	private:

		void _readHeader();

		//templated so that VLRs and EVLRs can share code despite having different int types for record length
		template<class T>
		void _readVLRs(std::uint32_t nVLR, std::uint32_t pointOffset = std::numeric_limits<std::uint32_t>::max());

		void _initChunks();

		template<class T>
		void _readBytes(T* ptr);

		//lazperf wants a function with this signature
		void getBytes(unsigned char* buffer, size_t count);

		std::unique_ptr<std::ifstream> _ifs;
		lazperf::las_decompressor::ptr _decompressor;
		lazperf::chunk* _currentChunk = nullptr;
		std::vector<lazperf::chunk> _chunks;
		uint64_t _pointInChunk = 0;

	};

	template<class T>
	void lapis::LasIO::_readBytes(T* ptr)
	{
		_ifs->read((char*)ptr, sizeof(T));
	}
}
#endif