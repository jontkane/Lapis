#pragma once
#ifndef lp_unit_h
#define lp_unit_h

#include"..\LapisTypeDefs.hpp"
#include<regex>

namespace lapis {

	class LinearUnitConverter;

	class LinearUnit {
		friend class LinearUnitConverter;

	public:

		//Produces an unknown linear unit. An unknown unit has a conversion factor of 1 with all other units
		LinearUnit();

		//Following existing convention, the conversion factor is the value that you multiply by to get meters
		LinearUnit(const std::string& name, coord_t convFactor);

		const std::string& name() const;

		coord_t convertOneFromThis(coord_t value, const LinearUnit& destUnits) const;
		coord_t convertOneFromThis(coord_t value, const std::optional<LinearUnit>& destUnits) const;
		coord_t convertOneToThis(coord_t value, const LinearUnit& sourceUnits) const;
		coord_t convertOneToThis(coord_t value, const std::optional<LinearUnit>& sourceUnits) const;

		//returns true if either this or other are unknown, or if their conv factors are equal to within floating point imprecision
		bool isConsistent(const LinearUnit& other) const;

		bool isUnknown() const;

		//this tests not for full equality of all members, but just that either both or neither are unknown, and also that the conv factors are within a floating point error
		bool operator==(const LinearUnit& other) const;

	private:
		std::string _name;
		coord_t _convFactor;
		bool _isUnknown;

	};

	class LinearUnitConverter {
	public:

		LinearUnitConverter();
		LinearUnitConverter(const LinearUnit& source, const LinearUnit& dest);
		LinearUnitConverter(const std::optional<LinearUnit>& source, const std::optional<LinearUnit>& dest);

		//these are aliases for each other
		coord_t convertOne(coord_t value) const;
		coord_t operator()(coord_t value) const;

		//This function converts values in a contiguous container, which is not necessarily an array of coord_t
		//The span variable indicates the distance between successive values to convert
		//For example, if we have: struct XYZ {coord_t x, y, z;};
		//and a std::vector<XYZ> of size 1000 named v, and we want to convert the z values only, the call would be:
		//convertManyInPlace(&v[0].z, 1000, sizeof(XYZ));
		//if span is unspecified, then the assumption is that po is a pointer to an array of coord_t
		void convertManyInPlace(coord_t* po, size_t count, size_t span = sizeof(coord_t)) const;

	private:
		coord_t _conv;
	};

	namespace linearUnitPresets {

		//Spelled 'metre' to match the convention in WKT strings
		const LinearUnit meter{ "metre", 1. };

		const LinearUnit internationalFoot{ "foot",0.3048 };

		const LinearUnit usSurveyFoot{ "US survey foot", 0.30480060960122 };

		const LinearUnit unknownLinear{};
	}
}

#endif