#pragma once
#ifndef lp_unit_h
#define lp_unit_h

#include"..\LapisTypeDefs.hpp"
#include<regex>

namespace lapis {
	enum class unitStatus {
		statedInCRS, //the CRS explicitly states the units
		inferredFromCRS, //the CRS isn't explicit but a plausible guess can be made; e.g., assuming that the vertical and horizontal units are the same
		unknown, //the unit status is completely unknown
		setByUser //the units were set by the user and not derived from the CRS
	};
	enum class unitType {
		linear,
		angular,
		area,
		unknown
	};

	struct Unit {
		std::string name; //the name as stored in the wkt, or "unknown"
		coord_t convFactor; //the number to multiple by to get to SI units (or to radians)
		unitType type; //true if it's a linear unit that can be converted to meters
		unitStatus status;

		Unit() : name("unknown"), convFactor(1), type(unitType::unknown), status(unitStatus::unknown) {}
		Unit(const std::string& name, coord_t convFactor, unitType type, unitStatus status) : name(name), convFactor(convFactor), type(type), status(status) {}
		Unit(const std::string& name);

		bool isUnknown() const {
			return status == unitStatus::unknown;
		}
		bool isLinear() const {
			return type == unitType::linear;
		}
	};
	namespace LinearUnitDefs {
		const Unit meter{ "metre",1.,unitType::linear,unitStatus::setByUser };
		const Unit foot{ "foot",0.3048,unitType::linear,unitStatus::setByUser };
		const Unit surveyFoot{ "US survey foot",0.30480060960122,unitType::linear,unitStatus::setByUser };
		const Unit unkLinear{ "unknown",1.,unitType::linear,unitStatus::unknown };
	}

	inline bool operator==(const Unit& lhs, const Unit& rhs) {
		//Not checking that the name strings are identical because I don't care about the difference between 'meter' and 'metre'
		return (lhs.convFactor == rhs.convFactor) && (lhs.type == rhs.type) && (lhs.isUnknown() == rhs.isUnknown());
	}

	inline coord_t convertUnits(coord_t value, const Unit& src, const Unit& dst) {
		if (src.isUnknown() || dst.isUnknown()) {
			return value;
		}
		value *= src.convFactor;
		value /= dst.convFactor;
		return value;
	}

	inline Unit::Unit(const std::string& name) {
		std::regex meter{ ".*m.*" };
		std::regex surveyfoot{ ".*us.*" };
		std::regex intlfoot{ ".*f.*" };
		if (std::regex_match(name, meter)) {
			*this = LinearUnitDefs::meter;
		}
		else if (std::regex_match(name, surveyfoot)) {
			*this = LinearUnitDefs::surveyFoot;
		}
		else if (std::regex_match(name, intlfoot)) {
			*this = LinearUnitDefs::foot;
		}
		else {
			*this = LinearUnitDefs::unkLinear;
		}
	}
}

#endif