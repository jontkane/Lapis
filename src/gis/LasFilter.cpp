#include"LasFilter.hpp"

namespace lapis {
	LasFilter::LasFilter() {
		priority = lasfilterpriority::mid;
	}
	LasFilterFirstReturns::LasFilterFirstReturns() {
		priority = lasfilterpriority::high;
	}
	bool LasFilterFirstReturns::isFiltered(const CurrentLasPoint& p) {
		return p.returnNumber() != 1;
	}
	LasFilterOnlyReturns::LasFilterOnlyReturns() {
		priority = lasfilterpriority::high;
	}
	bool LasFilterOnlyReturns::isFiltered(const CurrentLasPoint& p) {
		return p.numberOfReturns() != 1;
	}
	LasFilterClassWhitelist::LasFilterClassWhitelist(const std::unordered_set<std::uint8_t>& whitelist) : whitelist(whitelist) {
		priority = lasfilterpriority::mid;
	}
	bool LasFilterClassWhitelist::isFiltered(const CurrentLasPoint& p) {
		return !whitelist.contains(p.classification());
	}
	const std::unordered_set<std::uint8_t>& LasFilterClassWhitelist::getSet() const {
		return whitelist;
	}
	LasFilterClassBlacklist::LasFilterClassBlacklist(const std::unordered_set<std::uint8_t>& blacklist) : blacklist(blacklist) {
		priority = lasfilterpriority::low;
	}
	bool LasFilterClassBlacklist::isFiltered(const CurrentLasPoint& p) {
		return blacklist.contains(p.classification());
	}
	const std::unordered_set<std::uint8_t>& LasFilterClassBlacklist::getSet() const {
		return blacklist;
	}
	LasFilterWithheld::LasFilterWithheld() {
		priority = lasfilterpriority::high;
	}
	bool LasFilterWithheld::isFiltered(const CurrentLasPoint& p) {
		return p.withheld();
	}
	LasFilterMaxScanAngle::LasFilterMaxScanAngle(double maxscan) : maxscan(maxscan) {
		priority = lasfilterpriority::mid;
	}
	bool LasFilterMaxScanAngle::isFiltered(const CurrentLasPoint& p) {
		return std::abs(p.scanAngle()) > maxscan;
	}
	LasFilterAlwaysFail::LasFilterAlwaysFail() {
		priority = lasfilterpriority::high;
	}
	bool LasFilterAlwaysFail::isFiltered(const CurrentLasPoint& p) {
		return true;
	}
	LasFilterExtent::LasFilterExtent(const Extent& e) : _e(e) {
		priority = lasfilterpriority::mid;
	}
	bool LasFilterExtent::isFiltered(const CurrentLasPoint& p) {
		return !(_e.contains(p.x(), p.y()));
	}
	LasFilterPolygon::LasFilterPolygon(const VectorsAndAttributes<Polygon>& polygons)
	{
		priority = lasfilterpriority::low;
		for (size_t i = 0; i < polygons.nFeatures(); ++i) {
			_polygons.push_back(polygons.getGeometry(i));
		}
	}
	LasFilterPolygon::LasFilterPolygon(const VectorsAndAttributes<MultiPolygon>& polygons)
	{
		priority = lasfilterpriority::low;
		for (size_t i = 0; i < polygons.nFeatures(); ++i) {
			const MultiPolygon& multipoly = polygons.getGeometry(i);
			for (const Polygon& poly : multipoly.polygons()) {
				_polygons.push_back(poly);
			}
		}
	}
	bool LasFilterPolygon::isFiltered(const CurrentLasPoint& p)
	{
		CoordXY xy;
		_updateMaps(p);
		{
			std::shared_lock<std::shared_mutex> lock{ _mut };
			if (_overlaps.at(&p) == PolygonOverlap::NoOverlap) {
				return true;
			}
			if (_overlaps.at(&p) == PolygonOverlap::InputContainedByThis) {
				return false;
			}
			xy = _transforms.at(&p).transformSingleXY(p.x(), p.y());
		}
		for (const Polygon& poly : _polygons) {
			if (poly.pointInPolygon(xy.x, xy.y)) {
				return false;
			}
		}
		return true;
	}
	void LasFilterPolygon::_updateMaps(const CurrentLasPoint& p)
	{
		if (_overlaps.contains(&p)) {
			return;
		}

		PolygonOverlap current = PolygonOverlap::NoOverlap;
		for (const Polygon& poly : _polygons) {
			PolygonOverlap newOverlap = poly.extentOverlaps(p);
			if (newOverlap == PolygonOverlap::InputContainedByThis) {
				current = PolygonOverlap::InputContainedByThis;
				break;
			}
			if (newOverlap == PolygonOverlap::InputContainsThis || newOverlap == PolygonOverlap::PartialOverlap) {
				current = PolygonOverlap::PartialOverlap;
			}
		}

		std::unique_lock<std::shared_mutex> lock{ _mut };
		_overlaps.emplace(&p, current);

		_transforms.emplace(&p, CoordTransform(p.crs(), _crs));
	}
}