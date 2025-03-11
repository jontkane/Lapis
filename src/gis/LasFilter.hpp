#pragma once
#ifndef lp_lasheader_h
#define lp_lasheader_h

#include"CurrentLasPoint.hpp"
#include"Vector.hpp"

namespace lapis {

	namespace classification {
		//I want implicit conversion to int so it's an enum in a namespace instead of an enum class
		enum : std::uint8_t {
			neverClassified = 0,
			unclassified = 1,
			ground = 2,
			lowVegetation = 3,
			mediumVegetation = 4,
			highVegetation = 5,
			building = 6,
			lowPointNoise = 7,
			water = 9,
			rail = 10,
			roadSurface = 11,
			wireGuard = 13,
			wireConductor = 14,
			transmissionTower = 15,
			wireStructureConnector = 16,
			bridgeDeck = 17,
			highNoise = 18
		};
	}

	enum class lasfilterpriority {
		low = 0, //filters which are slow and rarely failed
		mid = 1, //anything in between
		high = 2 //intended for filters which are fast to compute and often failed. They will be checked first, hopefully preventing some uses of slower filters
	};

	//This is a base class for filters that cause points in an las file to be skipped
	//Note that this class only has access to un-normalized Z values. It cannot filter by height, only by elevation.
	class LasFilter {
	public:
		LasFilter();
		virtual bool isFiltered(const CurrentLasPoint& p) = 0;
		lasfilterpriority priority;
	};

	class LasFilterFirstReturns : public LasFilter {
	public:
		LasFilterFirstReturns();
		bool isFiltered(const CurrentLasPoint& p) override;
	};

	class LasFilterOnlyReturns : public LasFilter {
	public:
		LasFilterOnlyReturns();
		bool isFiltered(const CurrentLasPoint& p) override;
	};

	class LasFilterClassWhitelist : public LasFilter {
	public:
		LasFilterClassWhitelist(const std::unordered_set<std::uint8_t>& whitelist);
		bool isFiltered(const CurrentLasPoint& p) override;

		const std::unordered_set<std::uint8_t>& getSet() const;

	private:
		std::unordered_set<std::uint8_t> whitelist;
	};

	class LasFilterClassBlacklist : public LasFilter {
	public:
		LasFilterClassBlacklist(const std::unordered_set<std::uint8_t>& blacklist);
		bool isFiltered(const CurrentLasPoint& p) override;

		const std::unordered_set<std::uint8_t>& getSet() const;

	private:
		std::unordered_set<std::uint8_t> blacklist;
	};

	class LasFilterWithheld : public LasFilter {
	public:
		LasFilterWithheld();
		bool isFiltered(const CurrentLasPoint& p) override;
	};

	class LasFilterMaxScanAngle : public LasFilter {
	public:
		LasFilterMaxScanAngle(double maxscan);
		bool isFiltered(const CurrentLasPoint& p) override;
	private:
		double maxscan;
	};

	class LasFilterAlwaysFail : public LasFilter {
	public:
		LasFilterAlwaysFail();
		bool isFiltered(const CurrentLasPoint& p) override;
	};

	//this filter assumes that the extent matches the LasReader's crs
	class LasFilterExtent : public LasFilter {
	public:
		LasFilterExtent(const Extent& e);
		bool isFiltered(const CurrentLasPoint& p) override;
	private:
		Extent _e;
	};

	class LasFilterPolygon : public LasFilter {
	public:
		LasFilterPolygon(const VectorsAndAttributes<Polygon>& polygons);
		LasFilterPolygon(const VectorsAndAttributes<MultiPolygon>& polygons);
		bool isFiltered(const CurrentLasPoint& p) override;
	private:
		std::vector<Polygon> _polygons;
		CoordRef _crs;

		std::shared_mutex _mut;
		std::unordered_map<const CurrentLasPoint*, PolygonOverlap> _overlaps;
		std::unordered_map<const CurrentLasPoint*, CoordTransform> _transforms;
		void _updateMaps(const CurrentLasPoint& p);
	};

}

#endif