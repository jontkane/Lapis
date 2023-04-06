#include"gis_pch.hpp"
#include"Unit.hpp"

namespace lapis {
	LinearUnit::LinearUnit() : _name("unknown linear unit"), _convFactor(1.), _isUnknown(true)
	{
	}
	LinearUnit::LinearUnit(const std::string& name, coord_t convFactor) :
		_name(name), _convFactor(convFactor), _isUnknown(false)
	{
	}
	const std::string& LinearUnit::name() const
	{
		return _name;
	}
	coord_t LinearUnit::convertOneFromThis(coord_t value, const LinearUnit& destUnits) const
	{
		coord_t fullConv = _convFactor / destUnits._convFactor;
		if (_isUnknown || destUnits._isUnknown) {
			fullConv = 1.;
		}

		return value * fullConv;

	}
	coord_t LinearUnit::convertOneToThis(coord_t value, const LinearUnit& sourceUnits) const
	{
		coord_t fullConv = sourceUnits._convFactor / _convFactor;
		if (_isUnknown || sourceUnits._isUnknown) {
			fullConv = 1.;
		}

		return value * fullConv;
	}
	bool LinearUnit::isConsistent(const LinearUnit& other) const
	{
		if (_isUnknown || other._isUnknown) {
			return true;
		}
		if (std::abs(_convFactor - other._convFactor) < LAPIS_EPSILON) {
			return true;
		}
		return false;
	}
	bool LinearUnit::isUnknown() const
	{
		return _isUnknown;
	}
	LinearUnitConverter::LinearUnitConverter() : _conv(1.)
	{
	}
	LinearUnitConverter::LinearUnitConverter(const LinearUnit& source, const LinearUnit& dest)
	{
		if (source._isUnknown || dest._isUnknown) {
			_conv = 1.;
		}
		else {
			_conv = source._convFactor / dest._convFactor;
		}
	}
	coord_t LinearUnitConverter::convertOne(coord_t value) const
	{
		return value * _conv;
	}
	coord_t LinearUnitConverter::operator()(coord_t value) const
	{
		return convertOne(value);
	}
	void LinearUnitConverter::convertManyInPlace(coord_t* po, size_t count, size_t span) const
	{
		while (count) {
			*po = convertOne(*po);
			po += span;
			--count;
		}
	}
}