#pragma once
#ifndef lapis_coordref_h
#define lapis_coordref_h

#include"projwrappers.hpp"
#include"GDALWrappers.hpp"
#include"lasio.hpp"
#include"Unit.hpp"


namespace lapis {

	class CoordRef {
	public:
		CoordRef() = default;

		//s should be able to be any string proj can read (which is generally very permissive), a raster file, a vector file, or a .prj file
		CoordRef(const std::string& s);
		CoordRef(const std::string& s, LinearUnit zUnits);
		CoordRef(const char* s);
		CoordRef(const char* s, LinearUnit zUnits);
		CoordRef(const SharedPJ& pj);
		CoordRef(const LasIO& las);

		const std::string getPrettyWKT() const;

		const std::string getCompleteWKT() const;

		const std::string getSingleLineWKT() const;

		const std::string getShortName() const;

		//returns whether or not this projection is defined
		bool isEmpty() const;

		//returns whether a transformation is needed to go from one crs to the other--a bit looser than true equality
		bool isSame(const CoordRef& other)const;

		//returns true if either crs is empty, or if their horizontal components are the same
		bool isConsistentHoriz(const CoordRef& other) const;

		//returns true if the z units of the two coordrefs are the same, or if one or both are unknown
		bool isConsistentZUnits(const CoordRef& other) const;

		//return true if isConsistentCRSOnly is true, and the two CoordRefs haven't had their Z units set to different values
		bool isConsistent(const CoordRef& other) const;

		//returns true if the horizontal units are linear and not angular
		bool isProjected() const;

		//returns the XY units of the CRS.
		//A return without a value indicates that the CRS is angular, not linear
		std::optional<LinearUnit> getXYLinearUnits() const;

		//returns the Z units of the CRS
		//If there's no vertical datum, it returns a Unit object which is unknown, but has the same convfactor as the horizontal units
		const LinearUnit& getZUnits() const;

		//This function will set the zUnits of the crs
		void setZUnits(const LinearUnit& zUnits);

		//returns true if the vertical datum (and units) are defined
		bool hasVertDatum() const;

		//returns a string which is EPSG code of this CRS, if one can be deduced
		//compound CRSes will have both halves returned with a + in between
		//a CRS that can't be found in the EPSG database is reported as "Unknown"
		std::string getEPSG() const;

		//returns the CoordRef defined by the epsg code getEPSG identifies, or a copy of this coordref if that doesn't exist
		CoordRef getCleanEPSG() const;

		PJ* getPtr();
		const PJ* getPtr() const;

		SharedPJ& getSharedPtr();
		const SharedPJ& getSharedPtr() const;

	private:
		SharedPJ _p;
		LinearUnit _zUnits;

		void _crsFromString(const std::string& s);
		void _crsFromPrj(const std::string& s);
		void _crsFromRaster(const std::string& s);
		void _crsFromVector(const std::string& s);
		void _crsFromLasIO(const LasIO& las);
		LinearUnit _inferZUnits();
	};

	class CoordRefComparator {
	public:
		bool operator()(const CoordRef& a, const CoordRef& b) const;
	};
	class CoordRefHasher {
	public:
		size_t operator()(const CoordRef& a) const;
	};
}

#endif