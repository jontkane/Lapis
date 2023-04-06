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
	coord_t LinearUnit::convertOneFromThis(coord_t value, const std::optional<LinearUnit>& destUnits) const
	{
		if (!destUnits.has_value()) {
			return value;
		}
		return convertOneFromThis(value, destUnits.value());
	}
	coord_t LinearUnit::convertOneToThis(coord_t value, const LinearUnit& sourceUnits) const
	{
		coord_t fullConv = sourceUnits._convFactor / _convFactor;
		if (_isUnknown || sourceUnits._isUnknown) {
			fullConv = 1.;
		}

		return value * fullConv;
	}
	coord_t LinearUnit::convertOneToThis(coord_t value, const std::optional<LinearUnit>& sourceUnits) const
	{
		if (!sourceUnits.has_value()) {
			return value;
		}
		return convertOneToThis(value,sourceUnits.value());
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
	bool LinearUnit::operator==(const LinearUnit& other) const
	{
		return (_isUnknown == other._isUnknown) && (std::abs(_convFactor - other._convFactor) < LAPIS_EPSILON);
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
	LinearUnitConverter::LinearUnitConverter(const std::optional<LinearUnit>& source, const std::optional<LinearUnit>& dest)
	{
		if (!source.has_value() || !dest.has_value()) {
			_conv = 1.;
		}
		else if (source.value().isUnknown() || dest.value().isUnknown()) {
			_conv = 1.;
		}
		else {
			_conv = source.value()._convFactor / dest.value()._convFactor;
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
		union {
			char* forArithmetic;
			coord_t* forWork;
		} pointer;
		pointer.forWork = po;
		while (count) {
			*pointer.forWork = convertOne(*pointer.forWork);
			pointer.forArithmetic += span;
			--count;
		}
	}
}