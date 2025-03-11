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
        OGRPolygon* gdalPolygon = gdalGeometry->toPolygon();
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
    bool Polygon::_pointInRing(coord_t x, coord_t y, const std::vector<CoordXY>& ring) const
    {
        //the code in this function is a minimal modification of the code here: https://wrfranklin.org/Research/Short_Notes/pnpoly.html
        //that code has the following license:

        /*
        Copyright (c) 1970-2003, Wm. Randolph Franklin

        Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

        Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimers.
        Redistributions in binary form must reproduce the above copyright notice in the documentation and/or other materials provided with the distribution.
        The name of W. Randolph Franklin may not be used to endorse or promote products derived from this Software without specific prior written permission. 

        THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
        */

        bool contains = false;
        for (size_t i = 0, j = ring.size() - 1; i < ring.size(); j = i++) {
            if (((ring[i].y > y) != (ring[j].y > y)) &&
                (x < (ring[j].x - ring[i].x) * (y - ring[i].y) / (ring[j].y - ring[i].y) + ring[i].x)) {
                contains = !contains;
            }
        }
        return contains;
    }
    PolygonOverlap Polygon::_extentOverlapsRing(const Extent& e, const std::vector<CoordXY>& ring) const
    {
        //The strategy here is to check line segment intersections between the polygon and extent (i.e. quadrilateral, after projecting).
        //If there are any intersections at all, it's a partial overlap.
        //If there are no intersections, then:
        //If a point of the polygon falls within the extent, the polygon is contained
        //If a point of the extent falls within the polygon, the extent is contained
        //If neither is true, there's no overlap

        const CoordXYVector& extCoords = QuadExtent(e).coords();

        auto lineSegmentBeginningAtIndex = [](const auto& coords, size_t index, CoordXY& begin, CoordXY& end) {
            size_t secondIndex = index + 1;
            if (index == coords.size() - 1) {
                secondIndex = 0;
            }
            begin.x = coords[index].x;
            begin.y = coords[index].y;
            end.x = coords[secondIndex].x;
            end.y = coords[secondIndex].y;
            };

        //first, a quick and dirty check to see if the bounding boxes overlap. If they don't, we can just quickly return no overlap
        coord_t xmin = std::numeric_limits<coord_t>::max();
        coord_t ymin = std::numeric_limits<coord_t>::max();
        coord_t xmax = std::numeric_limits<coord_t>::lowest();
        coord_t ymax = std::numeric_limits<coord_t>::lowest();
        for (size_t i = 0; i < ring.size(); ++i) {
            xmin = std::min(xmin, ring[i].x);
            xmax = std::max(xmax, ring[i].x);
            ymin = std::min(ymin, ring[i].y);
            ymax = std::max(ymax, ring[i].y);
        }
        Extent boundingBox{ xmin,xmax,ymin,ymax };
        if (!boundingBox.overlapsUnsafe(e)) {
            return PolygonOverlap::NoOverlap;
        }

        bool anyCoincidentSegments = false;

        for (size_t extentVertexIndex = 0; extentVertexIndex < 4; ++extentVertexIndex) {
            //naming these variables according to the formula in https://en.wikipedia.org/wiki/Line%E2%80%93line_intersection
            CoordXY x1y1; //begin of the segment
            CoordXY x2y2; //end of the segment
            lineSegmentBeginningAtIndex(extCoords, extentVertexIndex, x1y1, x2y2); //fill in the values
            coord_t& x1 = x1y1.x;
            coord_t& y1 = x1y1.y;
            coord_t& x2 = x2y2.x;
            coord_t& y2 = x2y2.y;
            for (size_t polyVertexIndex = 0; polyVertexIndex < ring.size(); ++polyVertexIndex) {
                CoordXY x3y3;
                CoordXY x4y4;
                lineSegmentBeginningAtIndex(ring, polyVertexIndex, x3y3, x4y4);
                coord_t& x3 = x3y3.x;
                coord_t& y3 = x3y3.y;
                coord_t& x4 = x4y4.x;
                coord_t& y4 = x4y4.y;

                coord_t tNumerator = (x1 - x3) * (y3 - y4) - (y1 - y3) * (x3 - x4);
                coord_t uNumerator = -((x1 - x2) * (y1 - y3) - (y1 - y2) * (x1 - x3));
                coord_t denominator = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 * x4);

                //the inequalities reverse if the denominator is negative, but negating everything sorts it out
                if (denominator < 0) {
                    denominator = -denominator;
                    uNumerator = -uNumerator;
                    tNumerator = -tNumerator;
                }

                //the lines are parallel or coincident. We need to distinguish which.
                //they're coincident if three of the points are colinear
                //test for this comes from https://math.stackexchange.com/questions/405966/if-i-have-three-points-is-there-an-easy-way-to-tell-if-they-are-collinear
                if (denominator == 0) {
                    if ((y1 - y2) * (x3 - x2) == (y3 - y2) * (x2 - x1)) {
                        //coincident
                        //technically, this only proves the *lines* are coincident; the line *segments* may or may not actually overlap
                        auto valueIsBetween = [](coord_t test, coord_t first, coord_t second) {
                            return (test <= first && test >= second) || (test <= second && test >= first);
                            };
                        auto pointIsBetween = [&](const CoordXY& test, const CoordXY& begin, const CoordXY& end) {
                            return valueIsBetween(test.x, begin.x, end.x) && valueIsBetween(test.y, begin.y, end.y);
                            };
                        bool coincidentOverlap = false;
                        coincidentOverlap = coincidentOverlap || pointIsBetween(x1y1, x3y3, x4y4);
                        coincidentOverlap = coincidentOverlap || pointIsBetween(x2y2, x3y3, x4y4);
                        coincidentOverlap = coincidentOverlap || pointIsBetween(x3y3, x1y1, x2y2);
                        coincidentOverlap = coincidentOverlap || pointIsBetween(x4y4, x1y1, x2y2);
                        if (coincidentOverlap) {
                            anyCoincidentSegments = true;
                            continue;
                        }
                    }
                    else {
                        //parallel, so no intersection
                        continue;
                    }
                }

                bool overlap = true;
                overlap = overlap && tNumerator >= 0;
                overlap = overlap && uNumerator >= 0;
                overlap = overlap && tNumerator <= denominator;
                overlap = overlap && uNumerator <= denominator;
                if (overlap) {
                    return PolygonOverlap::PartialOverlap;
                }
            }
        }

        //if we make it this far, there is no non-coincident overlap between segments of the ring and the extent
        
        if (!anyCoincidentSegments) {
            //without the confounding factor of coincidence, we can conclude that we are not in a partial overlap situation
            //thus, either all of the extent vertices are contained by the ring, or none; and similarly for vice versa
            //a single check suffices in both cases
            if (_pointInRing(extCoords[0].x, extCoords[0].y, ring)) {
                return PolygonOverlap::InputContainedByThis;
            }
            if (e.contains(ring[0].x, ring[0].y)) {
                return PolygonOverlap::InputContainsThis;
            }
            return PolygonOverlap::NoOverlap;
        }

        //and finally, the annoying case where there's at least one coincident intersection, but no non-coincident intersections.
        //we can no longer assume that one vertex being inside means they all are, so the tests above are replaced with more comprehensive ones
        //in this situation, A is fully inside B if all vertices of A are inside B. If neither A nor B are fully inside the other, then it's partial overlap
        bool polyFullyInsideExt = true;
        for (size_t i = 0; i < ring.size(); ++i) {
            polyFullyInsideExt = polyFullyInsideExt && e.contains(ring[i].x, ring[i].y);
            if (!polyFullyInsideExt) {
                break;
            }
        }
        if (polyFullyInsideExt) {
            return PolygonOverlap::InputContainsThis;
        }
        bool extFullyInsidePoly = true;
        for (size_t i = 0; i < extCoords.size(); ++i) {
            extFullyInsidePoly = extFullyInsidePoly && _pointInRing(extCoords[i].x, extCoords[i].y, ring);
            if (!extFullyInsidePoly) {
                break;
            }
        }
        if (extFullyInsidePoly) {
            return PolygonOverlap::InputContainedByThis;
        }
        return PolygonOverlap::PartialOverlap;

    }
    Polygon::Polygon(const std::vector<CoordXY>& outerRing) :_outerRing(outerRing) {}
    void Polygon::addInnerRing(const std::vector<CoordXY>& innerRing)
    {
        _innerRings.push_back(innerRing);
    }
    bool Polygon::pointInPolygon(coord_t x, coord_t y) const
    {
        for (const auto& innerRing : _innerRings) {
            if (_pointInRing(x, y, innerRing)) {
                return false;
            }
        }
        return _pointInRing(x, y, _outerRing);
    }
    PolygonOverlap Polygon::extentOverlaps(const Extent& e) const
    {
        for (const auto& innerRing : _innerRings) {
            PolygonOverlap result = _extentOverlapsRing(e, innerRing);
            if (result == PolygonOverlap::InputContainedByThis) {
                return PolygonOverlap::NoOverlap;
            }
            if (result == PolygonOverlap::InputContainsThis) {
                return PolygonOverlap::PartialOverlap;
            }
            if (result == PolygonOverlap::PartialOverlap) {
                return PolygonOverlap::PartialOverlap;
            }
            //NoOverlap is a result that doesn't allow you to make any broader conclusions
        }
        return _extentOverlapsRing(e, _outerRing);
    }
    const std::unordered_set<OGRwkbGeometryType>& Polygon::gdalGeometryTypes()
    {
        auto init = []() {
            std::unordered_set<OGRwkbGeometryType> set;
            set.insert(OGRwkbGeometryType::wkbPolygon);
            set.insert(OGRwkbGeometryType::wkbPolygonM);
            set.insert(OGRwkbGeometryType::wkbPolygonZM);
            set.insert(OGRwkbGeometryType::wkbPolygon25D);
            return set;
            };
        static std::unordered_set<OGRwkbGeometryType> set = init();
        return set;
    }
    OGRwkbGeometryType Polygon::primaryGdalGeometryType()
    {
        return wkbPolygon;
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
        OGRMultiPolygon* gdalMultiPolygon = gdalGeometry->toMultiPolygon();
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
    const std::unordered_set<OGRwkbGeometryType>& MultiPolygon::gdalGeometryTypes()
    {
        auto init = []() {
            std::unordered_set<OGRwkbGeometryType> set;
            set.insert(OGRwkbGeometryType::wkbMultiPolygon);
            set.insert(OGRwkbGeometryType::wkbMultiPolygonM);
            set.insert(OGRwkbGeometryType::wkbMultiPolygonZM);
            set.insert(OGRwkbGeometryType::wkbMultiPolygon25D);
            return set;
            };
        static std::unordered_set<OGRwkbGeometryType> set = init();
        return set;
    }
    OGRwkbGeometryType MultiPolygon::primaryGdalGeometryType()
    {
        return wkbMultiPolygon;
    }
    const std::vector<Polygon>& MultiPolygon::polygons() const
    {
        return _polygons;
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
        OGRPoint* gdalPoint = gdalGeometry->toPoint();
        if (!gdalPoint) {
            throw std::invalid_argument("geometry is not a point");
        }
        _x = gdalPoint->getX();
        _y = gdalPoint->getY();
    }
    Point::Point(coord_t x, coord_t y) : _x(x), _y(y) {}
    Point::Point(CoordXY xy) : _x(xy.x), _y(xy.y) {}
    const std::unordered_set<OGRwkbGeometryType>& Point::gdalGeometryTypes()
    {
        auto init = []() {
            std::unordered_set<OGRwkbGeometryType> set;
            set.insert(OGRwkbGeometryType::wkbPoint);
            set.insert(OGRwkbGeometryType::wkbPointM);
            set.insert(OGRwkbGeometryType::wkbPointZM);
            set.insert(OGRwkbGeometryType::wkbPoint25D);
            return set;
            };
        static std::unordered_set<OGRwkbGeometryType> set = init();
        return set;
    }
    OGRwkbGeometryType Point::primaryGdalGeometryType()
    {
        return wkbPoint;
    }
}