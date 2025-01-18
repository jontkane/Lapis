#include"vector.hpp"

namespace lapis {

    OGRPolygon Polygon::asGdal() const
    {
        OGRPolygon out{};

        OGRLinearRing gdalOuterRing = _gdalCurveFromRing(_outerRing);
        out.addRing(&gdalOuterRing);
        for (const auto& innerRing : _innerRings) {
            OGRLinearRing gdalInnerRing = _gdalCurveFromRing(innerRing);
            out.addRing(&gdalInnerRing);
        }
        return out;
    }
    Polygon::Polygon(OGRGeometry* gdalGeometry)
    {
        OGRPolygon* gdalPolygon = dynamic_cast<OGRPolygon*>(gdalGeometry);
        if (!gdalPolygon) {
            throw std::invalid_argument("geometry is not a polygon");
        }

        auto stdVectorFromOGRLinearRing = [](const OGRLinearRing* ogr)->std::vector<CoordXY> {
            std::vector<CoordXY> out;
            out.reserve(ogr->getNumPoints());
            for (const OGRPoint& point : *ogr) {
                out.emplace_back(point.getX(), point.getY());
            }
            out.pop_back(); //gdal rings close themselves by duplicating the first point at the end
            return out;
        };

        OGRLinearRing* outerRing = gdalPolygon->getExteriorRing();
        _outerRing = stdVectorFromOGRLinearRing(outerRing);
        int nInnerRing = gdalPolygon->getNumInteriorRings();
        _innerRings.reserve(nInnerRing);
        for (int i = 0; i < nInnerRing; ++i) {
            _innerRings.emplace_back(stdVectorFromOGRLinearRing(gdalPolygon->getInteriorRing(i)));
        }
    }
    Polygon::Polygon(const Extent& e)
    {
        _outerRing.emplace_back(e.xmin(), e.ymax());
        _outerRing.emplace_back(e.xmin(), e.ymin());
        _outerRing.emplace_back(e.xmax(), e.ymin());
        _outerRing.emplace_back(e.xmax(), e.ymax());
    }
    Polygon::Polygon(const QuadExtent& q)
    {
        //this list is clockwise, so we need to reverse it
        const CoordXYVector& coords = q.coords();

        for (int i = (int)(coords.size() - 1); i >= 0; --i) {
            _outerRing.emplace_back(coords[i]);
        }
    }
    OGRLinearRing Polygon::_gdalCurveFromRing(const std::vector<CoordXY>& ring) const
    {
        
        OGRLinearRing out{};
        
        auto addPoint = [&](const CoordXY& xy) {
            OGRPoint point;
            point.setX(xy.x);
            point.setY(xy.y);
            out.addPoint(&point);
        };
        for (const CoordXY& xy : ring) {
            addPoint(xy);
        }
        addPoint(ring.front());
        return out;
    }
    Polygon::Polygon(const std::vector<CoordXY>& outerRing) :_outerRing(outerRing) {}
    void Polygon::addInnerRing(const std::vector<CoordXY>& innerRing)
    {
        _innerRings.push_back(innerRing);
    }
    OGRMultiPolygon MultiPolygon::asGdal() const
    {
        OGRMultiPolygon out{};
        for (const auto& polygon : _polygons) {
            OGRPolygon poly = polygon.asGdal();
            out.addGeometry(&poly);
        }
        return out;
    }
    MultiPolygon::MultiPolygon(OGRGeometry* gdalGeometry)
    {
        OGRMultiPolygon* gdalMultiPolygon = dynamic_cast<OGRMultiPolygon*>(gdalGeometry);
        if (!gdalMultiPolygon) {
            throw std::invalid_argument("geometry is not a multipolygon");
        }
        for (OGRPolygon* gdalPolygon : *gdalMultiPolygon) {
            _polygons.emplace_back(Polygon(gdalPolygon));
        }
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
            switch (keyValue.second.type) {
            case FieldType::Integer:
                keyValue.second.values.resize(_nrow, Variant((int64_t)0));
                break;
            case FieldType::Real:
                keyValue.second.values.resize(_nrow, Variant((double)0.));
                break;
            case FieldType::String:
                keyValue.second.values.resize(_nrow, Variant(FixedWidthString()));
                break;
            }
        }
    }
    void AttributeTable::addRow()
    {
        _nrow++;
        resize(_nrow);
    }
    size_t AttributeTable::nrow() const
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
    OGRPoint Point::asGdal() const
    {
        return OGRPoint(_x,_y);
    }
    Point::Point(OGRGeometry* gdalGeometry)
    {
        OGRPoint* gdalPoint = dynamic_cast<OGRPoint*>(gdalGeometry);
        if (!gdalPoint) {
            throw std::invalid_argument("geometry is not a point");
        }
        _x = gdalPoint->getX();
        _y = gdalPoint->getY();
    }
    Point::Point(coord_t x, coord_t y) : _x(x), _y(y) {}
    Point::Point(CoordXY xy) : _x(xy.x), _y(xy.y) {}
}