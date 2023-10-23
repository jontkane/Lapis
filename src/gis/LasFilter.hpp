#pragma once
#ifndef lp_lasheader_h
#define lp_lasheader_h

#include"CurrentLasPoint.hpp"

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
		LasFilter() {
			priority = lasfilterpriority::mid;
		}
		virtual bool isFiltered(const CurrentLasPoint& p) = 0;
		lasfilterpriority priority;
	};

	class LasFilterFirstReturns : public LasFilter {
	public:
		LasFilterFirstReturns() {
			priority = lasfilterpriority::high;
		}
		bool isFiltered(const CurrentLasPoint& p) override {
			return p.returnNumber() != 1;
		}
	};

	class LasFilterOnlyReturns : public LasFilter {
	public:
		LasFilterOnlyReturns() {
			priority = lasfilterpriority::high;
		}
		bool isFiltered(const CurrentLasPoint& p) override {
			return p.numberOfReturns() != 1;
		}
	};

	class LasFilterClassWhitelist : public LasFilter {
	public:
		LasFilterClassWhitelist(const std::unordered_set<std::uint8_t>& whitelist) : whitelist(whitelist) {
			priority = lasfilterpriority::mid;
		}
		bool isFiltered(const CurrentLasPoint& p) override {
			return !whitelist.contains(p.classification());
		}

		const std::unordered_set<std::uint8_t>& getSet() const {
			return whitelist;
		}

	private:
		std::unordered_set<std::uint8_t> whitelist;
	};

	class LasFilterClassBlacklist : public LasFilter {
	public:
		LasFilterClassBlacklist(const std::unordered_set<std::uint8_t>& blacklist) : blacklist(blacklist) {
			priority = lasfilterpriority::low;
		}
		bool isFiltered(const CurrentLasPoint& p) override {
			return blacklist.contains(p.classification());
		}

		const std::unordered_set<std::uint8_t>& getSet() const {
			return blacklist;
		}

	private:
		std::unordered_set<std::uint8_t> blacklist;
	};

	class LasFilterWithheld : public LasFilter {
	public:
		LasFilterWithheld() {
			priority = lasfilterpriority::high;
		}
		bool isFiltered(const CurrentLasPoint& p) override {
			return p.withheld();
		}
	};

	class LasFilterMaxScanAngle : public LasFilter {
	public:
		LasFilterMaxScanAngle(double maxscan) : maxscan(maxscan) {
			priority = lasfilterpriority::mid;
		}
		bool isFiltered(const CurrentLasPoint& p) override {
			return std::abs(p.scanAngle()) > maxscan;
		}
	private:
		double maxscan;
	};

	class LasFilterAlwaysFail : public LasFilter {
	public:
		LasFilterAlwaysFail() {
			priority = lasfilterpriority::high;
		}
		bool isFiltered(const CurrentLasPoint& p) override {
			return true;
		}
	};

	//this filter assumes that the extent matches the LasReader's crs
	class LasFilterExtent : public LasFilter {
	public:
		LasFilterExtent(const Extent& e): _e(e) {
			priority = lasfilterpriority::mid;
		}
		bool isFiltered(const CurrentLasPoint& p) override {
			return !(_e.contains(p.x(), p.y()));
		}
	private:
		Extent _e;
	};
}

#endif