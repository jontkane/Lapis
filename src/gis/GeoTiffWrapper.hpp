#pragma once
#ifndef lp_geotiffwrapper_h
#define lp_geotiffwrapper_h

#include"gis_pch.hpp"

namespace lapis {

#pragma pack(push, 1)
	union gtifKey {
		struct gtifKeyHeader {
			unsigned short keyDirectoryVersion;
			unsigned short keyRevision;
			unsigned short minorRevision;
			unsigned short numberOfKeys;
		} header;
		struct gtifKeyEntry {
			unsigned short keyID;
			unsigned short tiffTagLocation;
			unsigned short count;
			unsigned short value_offset;
		} entry;
	};
#pragma pack(pop)

	const int gtifKeysCode = 34735;
	const int gtifDoublesCode = 34736;
	const int gtifAsciiCode = 34737;

	class tiffWrapper {
	public:
		ST_TIFF* ptr;
		tiffWrapper() : ptr(ST_Create()) {}
		~tiffWrapper() {
			ST_Destroy(ptr);
		}

		tiffWrapper(const tiffWrapper& other) = delete;
	};

	class gtifWrapper {
	public:
		GTIF* ptr;
		gtifWrapper(const tiffWrapper& tiff, TIFFMethod* method) : ptr(GTIFNewWithMethods(tiff.ptr, method)) {}
		~gtifWrapper() {
			if (ptr != nullptr) {
				GTIFFree(ptr);
			}
		}

		gtifWrapper(const gtifWrapper& other) = delete;
	};

}

#endif