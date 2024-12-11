#include"vector.hpp"

namespace lapis {
    Polygon::Polygon(const std::list<CoordXY>& outerRing) :_outerRing(outerRing) {}
    void Polygon::addInnerRing(const std::list<CoordXY>& innerRing)
    {
        _innerRings.push_back(innerRing);
    }
    void Polygon::_appendToWkbNoHeader(std::vector<uint8_t>& wkb) const
    {
        uint32_t nRings = 1 + _innerRings.size();
        _appendToWkb<uint32_t>(wkb, nRings);

        _appendRingToWkb(wkb, _outerRing);
        for (const auto& innerRing : _innerRings) {
            _appendRingToWkb(wkb, innerRing);
        }
    }
    void Polygon::_appendRingToWkb(std::vector<uint8_t>& wkb, const std::list<CoordXY>& ring)
    {
        auto appendPointToWkb = [&](CoordXY xy) {
            _appendToWkb(wkb, (double)xy.x);
            _appendToWkb(wkb, (double)xy.y);
        };
        for (const CoordXY& xy : ring) {
            appendPointToWkb(xy);
        }
        appendPointToWkb(ring.front()); //closing the ring
    }
    UniqueOGRGeometry MultiPolygon::asGdal() const
    {
        std::vector<uint8_t> wkb;
        _firstBytesOfWkb(wkb, _wkbFormatNumber);
        uint32_t nPolygon = _polygons.size();
        _appendToWkb<uint32_t>(wkb, nPolygon);
        for (const auto& polygon : _polygons) {
            polygon._appendToWkbNoHeader(wkb);
        }
        return createGeometryWrapperFromWkb((void*)wkb.data(), _crs);
    }
    void MultiPolygon::addPolygon(const Polygon& polygon)
    {
        _polygons.push_back(polygon);
    }
    void AttributeTable::setStringField(size_t index, const std::string& name, const std::string& value)
    {
        if (_fields.at(name).type != FieldType::String) {
            throw std::runtime_error("Not a string field");
        }
        FixedWidthString& fws = std::get<FixedWidthString>(_fields.at(name).values.at(index));
        fws.set(value, _fields.at(name).width);
    }
    void AttributeTable::setIntegerField(size_t index, const std::string& name, int64_t value)
    {
        if (_fields.at(name).type != FieldType::Integer) {
            throw std::runtime_error("Not an integer field");
        }
        _fields.at(name).values.at(index) = value;
    }
    void AttributeTable::setRealField(size_t index, const std::string& name, double value)
    {
        if (_fields.at(name).type != FieldType::Real) {
            throw std::runtime_error("Not a floating point field");
        }
        _fields.at(name).values.at(index) = value;
    }
    void AttributeTable::addStringField(const std::string& name, size_t width)
    {
        std::vector<Variant> values = std::vector<Variant>(_nrow, Variant(FixedWidthString()));
        Field newField{ FieldType::String, width, std::move(values) };
        _fields.emplace(name, std::move(newField));
        _fieldNamesInOrder.push_back(name);
    }
    void AttributeTable::addIntegerField(const std::string& name)
    {
        std::vector<Variant> values = std::vector<Variant>(_nrow, Variant((int64_t)0));
        Field newField{ FieldType::Integer, 0, std::move(values) };
        _fields.emplace(name, std::move(newField));
        _fieldNamesInOrder.push_back(name);
    }
    void AttributeTable::addRealField(const std::string& name)
    {
        std::vector<Variant> values = std::vector<Variant>(_nrow, Variant((double)0));
        Field newField{ FieldType::Real, 0, std::move(values) };
        _fields.emplace(name, std::move(newField));
        _fieldNamesInOrder.push_back(name);
    }
    void AttributeTable::resize(size_t nrow)
    {
        _nrow = nrow;
        for (auto& keyValue : _fields) {
            keyValue.second.values.resize(_nrow);
        }
    }
    void AttributeTable::addRow()
    {
        _nrow++;
        resize(_nrow);
    }
    size_t AttributeTable::nrow()
    {
        return _nrow;
    }
    std::vector<std::string> AttributeTable::getAllFieldNames() const
    {
        return _fieldNamesInOrder;
    }
    FieldType AttributeTable::getFieldType(const std::string& name) const
    {
        return _fields.at(name).type;
    }
    size_t AttributeTable::getStringFieldWidth(const std::string& name) const
    {
        if (_fields.at(name).type != FieldType::String) {
            throw std::runtime_error("Not a string field");
        }
        return _fields.at(name).width;
    }
    const std::string& AttributeTable::getStringField(size_t index, const std::string& name) const
    {
        if (_fields.at(name).type != FieldType::String) {
            throw std::runtime_error("Not a string field");
        }
        return std::get<FixedWidthString>(_fields.at(name).values.at(index)).get();
    }
    int64_t AttributeTable::getIntegerField(size_t index, const std::string& name) const
    {
        if (_fields.at(name).type != FieldType::Integer) {
            throw std::runtime_error("Not an integer field");
        }
        return std::get<int64_t>(_fields.at(name).values.at(index));
    }
    double AttributeTable::getRealField(size_t index, const std::string& name) const
    {
        if (_fields.at(name).type != FieldType::Real) {
            throw std::runtime_error("Not a floating point field");
        }
        return std::get<double>(_fields.at(name).values.at(index));
    }
    CoordRef& GisVector::crs()
    {
        return _crs;
    }
    const CoordRef& GisVector::crs() const
    {
        return _crs;
    }
    constexpr bool GisVector::isNativeLittleEndian()
    {
        return std::endian::native == std::endian::little;
    }
    void GisVector::_firstBytesOfWkb(std::vector<uint8_t>& wkb, uint32_t wkbFormatNumber)
    {
        _appendToWkb<uint8_t>(wkb, isNativeLittleEndian() ? (uint8_t)1 : (uint8_t)0);
        _appendToWkb<uint32_t>(wkb, wkbFormatNumber);
    }
}