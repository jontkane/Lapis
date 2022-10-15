#pragma once

#ifndef lp_lasextent_h
#define lp_lasextent_h

#include"gis_pch.hpp"
#include"Extent.hpp"

namespace lapis {

	//this class is a simple extension of Extent which also stores the number of points in the las file that it was derived from
	class LasExtent : public Extent {
	public:

		LasExtent() : Extent(), _nPoints(0) {}
		LasExtent(const std::string& s);
		LasExtent(const LasIO& las);
		LasExtent(const Extent& e, std::uint64_t nPoints) : Extent(e), _nPoints(nPoints) {}

		virtual ~LasExtent() = default;

		std::uint64_t nPoints() const {
			return _nPoints;
		}

	protected:
		std::uint64_t _nPoints;
	};

}

#endif