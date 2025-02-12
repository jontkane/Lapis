#pragma once
#ifndef lp_rasteralgos_h
#define lp_rasteralgos_h

#include"cropview.hpp"
#include"vector.hpp"

namespace lapis {

	//more optimized versions of aggregate for the calls which get made the most
	template<class T>
	inline Raster<T> aggregateSum(const Raster<T>& r, const Alignment& a) {
		Raster<T> out{ a };
		for (cell_t bigCell : CellIterator(a, r,SnapType::out)) {
			Extent e = a.extentFromCell(bigCell);
			for (cell_t smallCell : CellIterator(r, e, SnapType::near)) {
				if (r[smallCell].has_value()) {
					out[bigCell].value() += r[smallCell].value();
				}
			}
		}

		for (cell_t cell : CellIterator(a)) {
			if (out[cell].value() != 0) {
				out[cell].has_value() = true;
			}
		}

		return out;
	}

	template<class T>
	inline Raster<T> aggregateCount(const Raster<T>& r, const Alignment& a) {
		Raster<T> out{ a };
		for (cell_t bigCell : CellIterator(a, r, SnapType::out)) {
			Extent e = a.extentFromCell(bigCell);
			for (cell_t smallCell : CellIterator(r, e, SnapType::near)) {
				if (r[smallCell].has_value()) {
					out[bigCell].value()++;
				}
			}
		}

		for (cell_t cell : CellIterator(a)) {
			if (out[cell].value() != 0) {
				out[cell].has_value() = true;
			}
		}
		return out;
	}

	template<class T>
	inline Raster<T> aggregateMean(const Raster<T>& r, const Alignment& a) {
		Raster<T> out{ a };
		for (cell_t bigCell : CellIterator(a, r, SnapType::out)) {
			Extent e = a.extentFromCell(bigCell);
			T numerator = 0;
			T denominator = 0;
			for (cell_t smallCell : CellIterator(r, e, SnapType::near)) {
				if (r[smallCell].has_value()) {
					denominator++;
					numerator += r[smallCell].value();
				}
			}
			if (denominator != 0) {
				out[bigCell].has_value() = true;
				out[bigCell].value() = numerator / denominator;
			}
		}
		return out;
	}

	template<class OUTPUT, class INPUT>
	using ViewFunc = std::function<const xtl::xoptional<OUTPUT>(const CropView<INPUT>&)>;

	template<class OUTPUT, class INPUT>
	inline Raster<OUTPUT> aggregate(const Raster<INPUT>& r, const Alignment& a, ViewFunc<OUTPUT, INPUT> f) {
		Raster<OUTPUT> out{ a };
		for (cell_t cell : CellIterator(a, r, SnapType::out)) {
			Extent e = a.extentFromCell(cell);

			try {
				Raster<INPUT>* po = const_cast<Raster<INPUT>*>(&r); //I promise I'm not actually going to modify it; forgive this const_cast
				const CropView<INPUT> cv{ po, e, SnapType::near }; //see, I even made the view const

				auto v = f(cv);
				out[cell].has_value() = v.has_value();
				out[cell].value() = v.value();
			}
			catch (OutsideExtentException e) {
				continue; //This can happen due to floating point inaccuracies
			}
		}
		return out;
	}

	template<class OUTPUT, class INPUT>
	inline Raster<OUTPUT> focal(const Raster<INPUT>& r, int windowSize, ViewFunc<OUTPUT, INPUT> f) {
		if (windowSize < 0 || windowSize % 2 != 1) {
			throw std::invalid_argument("Invalid window size in focal");
		}

		Raster<OUTPUT> out{ (Alignment)r };
		int lookDist = (windowSize - 1) / 2;

		for (cell_t cell = 0; cell < r.ncell(); ++cell) {
			Extent e = r.extentFromCell(cell);
			e = Extent(e.xmin() - lookDist * r.xres(), e.xmax() + lookDist * r.xres(), e.ymin() - lookDist * r.yres(), e.ymax() + lookDist * r.yres());
			Raster<INPUT>* po = const_cast<Raster<INPUT>*>(&r);
			const CropView<INPUT> cv{ po,e,SnapType::near };

			auto v = f(cv);
			out[cell].has_value() = v.has_value();
			out[cell].value() = v.value();
		}
		return out;
	}

	template<class OUTPUT, class INPUT>
	xtl::xoptional<OUTPUT> viewMax(const CropView<INPUT>& in) {
		bool hasvalue = false;
		OUTPUT value = std::numeric_limits<OUTPUT>::lowest();

		for (cell_t cell = 0; cell < in.ncell(); ++cell) {
			hasvalue = hasvalue || in[cell].has_value();
			if (in[cell].has_value()) {
				value = std::max(value, (OUTPUT)in[cell].value());
			}
		}
		return xtl::xoptional<OUTPUT>(value, hasvalue);
	}

	template<class OUTPUT, class INPUT>
	xtl::xoptional<OUTPUT> viewMean(const CropView<INPUT>& in) {
		OUTPUT numerator = 0;
		OUTPUT denominator = 0;
		for (cell_t cell = 0; cell < in.ncell(); ++cell) {
			if (in[cell].has_value()) {
				denominator++;
				numerator += (OUTPUT)in[cell].value();
			}
		}
		if (denominator) {
			return xtl::xoptional<OUTPUT>(numerator / denominator);
		}
		return xtl::missing<OUTPUT>();
	}

	template<class OUTPUT, class INPUT>
	xtl::xoptional<OUTPUT> viewStdDev(const CropView<INPUT>& in) {
		//not calculating the mean twice is a potential optimization if it matters
		xtl::xoptional<OUTPUT> mean = viewMean<OUTPUT, INPUT>(in);
		OUTPUT denominator = 0;
		OUTPUT numerator = 0;
		if (!mean.has_value()) {
			return xtl::missing<OUTPUT>();
		}
		for (cell_t cell = 0; cell < in.ncell(); ++cell) {
			if (in[cell].has_value()) {
				denominator++;
				OUTPUT temp = (OUTPUT)in[cell].value() - mean.value();
				numerator += temp * temp;
			}
		}
		return xtl::xoptional<OUTPUT>(std::sqrt(numerator / denominator));
	}

	template<class OUTPUT, class INPUT>
	xtl::xoptional<OUTPUT> viewRumple(const CropView<INPUT>& in) {
		OUTPUT sum = 0;
		OUTPUT nTriangle = 0;

		//the idea here is to interpolate the height at the intersection between cells
		//then draw isoceles right triangles with the hypotenuse going between adjacent cell centers and the opposite point at a cell intersection
		

		for (rowcol_t row = 1; row < in.nrow(); ++row) {
			for (rowcol_t col = 1; col < in.ncol(); ++col) {
				auto lr = in.atRCUnsafe(row, col);
				auto ll = in.atRCUnsafe(row, col - 1);
				auto ur = in.atRCUnsafe(row - 1, col);
				auto ul = in.atRCUnsafe(row - 1, col - 1);

				OUTPUT numerator = 0;
				OUTPUT denominator = 0;
				auto runningSum = [&](auto x) {
					if (x.has_value()) {
						denominator++;
						numerator += (OUTPUT)x.value();
					}
				};

				runningSum(lr);
				runningSum(ll);
				runningSum(ur);
				runningSum(ul);

				if (denominator == 0) {
					continue;
				}
				OUTPUT mid = numerator / denominator;


				//a and b are the heights of two cells, and mid is the interpolated height of one of the center intersections adjacent to them
				//this calculates the ratio between the area of this triangle and its projection to the ground
				auto triangleAreaRatio = [&](xtl::xoptional<INPUT> a, xtl::xoptional<INPUT> b, OUTPUT mid) {
					if (!a.has_value() || !b.has_value()) {
						return;
					}
					OUTPUT longdiff = (OUTPUT)a.value() - (OUTPUT)b.value();
					OUTPUT middiff = (OUTPUT)a.value() - mid;

					//the CSMs produced by lapis will always have equal xres and yres
					//this formula doesn't look symmetric between a and b, but it actually is if you expand it out
					//this version has slightly fewer calculations than the clearer version
					//this was derived from the gram determinant, after a great deal of simplification
					nTriangle++;
					sum += (2.f / (OUTPUT)in.xres()) * std::sqrt(0.5f + middiff * middiff + longdiff * longdiff / 2.f - longdiff * middiff);
				};
				triangleAreaRatio(ll, ul, mid);
				triangleAreaRatio(ll, lr, mid);
				triangleAreaRatio(ul, ur, mid);
				triangleAreaRatio(ur, lr, mid);
			}
		}

		if (nTriangle == 0) {
			return xtl::missing<OUTPUT>();
		}
		return sum / nTriangle;
	}

	template<class OUTPUT, class INPUT>
	xtl::xoptional<OUTPUT> viewSum(const CropView<INPUT>& in) {
		OUTPUT value = 0;
		for (cell_t cell = 0; cell < in.ncell(); ++cell) {
			if (in[cell].has_value()) {
				value += (OUTPUT)in[cell].value();
			}
		}
		return xtl::xoptional<OUTPUT>(value);
	}

	template<class OUTPUT, class INPUT>
	xtl::xoptional<OUTPUT> viewCount(const CropView<INPUT>& in) {
		OUTPUT count = 0;
		for (cell_t cell = 0; cell < in.ncell(); ++cell) {
			if (in[cell].has_value()) {
				count++;
			}
		}
		return xtl::xoptional<OUTPUT>(count);
	}

	//this function will not have the expected behavior unless the CropView is 3x3
	template<class OUTPUT>
	struct slopeComponents {
		xtl::xoptional<OUTPUT> nsSlope, ewSlope;
	};
	template<class OUTPUT, class INPUT>
	inline slopeComponents<OUTPUT> getSlopeComponents(const CropView<INPUT>& in) {
		slopeComponents<OUTPUT> out;
		if (in.ncell() < 9) {
			out.nsSlope = xtl::missing<OUTPUT>();
			out.ewSlope = xtl::missing<OUTPUT>();
			return out;
		}
		for (cell_t cell = 0; cell < in.ncell(); ++cell) {
			if (!in[cell].has_value()) {
				out.nsSlope = xtl::missing<OUTPUT>();
				out.ewSlope = xtl::missing<OUTPUT>();
				return out;
			}
		}
		out.nsSlope = (OUTPUT)((in[6].value() + 2 * in[7].value() + in[8].value() - in[0].value() - 2 * in[1].value() - in[2].value()) / (8. * in.yres()));
		out.ewSlope = (OUTPUT)((in[0].value() + 2 * in[3].value() + in[6].value() - in[2].value() - 2 * in[5].value() - in[8].value()) / (8. * in.xres()));
		return out;
	}

	template<class OUTPUT, class INPUT>
	inline xtl::xoptional<OUTPUT> viewSlopeRadians(const CropView<INPUT>& in) {
		slopeComponents<OUTPUT> comp = getSlopeComponents<OUTPUT, INPUT>(in);
		if (!comp.nsSlope.has_value()) {
			return xtl::missing<OUTPUT>();
		}
		OUTPUT slopeProp = (OUTPUT)std::sqrt(comp.nsSlope.value() * comp.nsSlope.value() + comp.ewSlope.value() * comp.ewSlope.value());
		return xtl::xoptional<OUTPUT>((OUTPUT)std::atan(slopeProp));
	}
	template<class OUTPUT, class INPUT>
	inline xtl::xoptional<OUTPUT> viewAspectRadians(const CropView<INPUT>& in) {
		slopeComponents<OUTPUT> comp = getSlopeComponents<OUTPUT, INPUT>(in);
		if (!comp.nsSlope.has_value()) {
			return xtl::missing<OUTPUT>();
		}
		OUTPUT ns = comp.nsSlope.value();
		OUTPUT ew = comp.ewSlope.value();
		if (ns > 0) {
			if (ew > 0) {
				return (OUTPUT)std::atan(ew / ns);
			}
			else if (ew < 0) {
				return (OUTPUT)(2.f * M_PI + std::atan(ew / ns));
			}
			else {
				return (OUTPUT)0;
			}
		}
		else if (ns < 0) {
			return (OUTPUT)(M_PI + std::atan(ew / ns));
		}
		else {
			if (ew > 0) {
				return (OUTPUT)(M_PI / 2.f);
			}
			else if (ew < 0) {
				return (OUTPUT)(3.f * M_PI / 2.f);
			}
			else {
				return xtl::missing<OUTPUT>();
			}
		}
	}

	template<class OUTPUT, class INPUT>
	inline xtl::xoptional<OUTPUT> viewSlopeDegrees(const CropView<INPUT>& in) {
		constexpr OUTPUT toDegrees = (OUTPUT)(360. / 2. / M_PI);
		return viewSlopeRadians<OUTPUT, INPUT>(in) * toDegrees;
	}
	template<class OUTPUT, class INPUT>
	inline xtl::xoptional<OUTPUT> viewAspectDegrees(const CropView<INPUT>& in) {
		constexpr OUTPUT toDegrees = (OUTPUT)(360. / 2. / M_PI);
		return viewAspectRadians<OUTPUT, INPUT>(in) * toDegrees;
	}

	/*
	
	The curvature formulas are taken from:
	https://www.onestopgis.com/GIS-Theory-and-Techniques/Terrain-Mapping-and-Analysis/Terrain-Analysis-Surface-Curvature/2-Equation-for-Computing-Curvature.html
	Variable names match that page

	*/

	template<class T>
	struct CurvatureTempVariables {
		xtl::xoptional<T> D, E, F, G, H;
	};

	template<class OUTPUT, class INPUT>
	CurvatureTempVariables<OUTPUT> calcCurveTempVars(const CropView<INPUT>& in) {
		CurvatureTempVariables<OUTPUT> vars;
#pragma warning(push)
#pragma warning(disable : 4244)
		vars.D = ((in[3] + in[5]) / 2. - in[4]) / (in.xres() * in.xres());
		vars.E = ((in[1] + in[7]) / 2. - in[4]) / (in.yres() * in.yres());
		vars.F = (-in[0] + in[2] + in[6] - in[8]) / (4 * in.xres() * in.yres());
		vars.G = (-in[3] + in[5]) / (2 * in.xres());
		vars.H = (in[1] - in[7]) / (2 * in.yres());
#pragma warning(pop)
		return vars;
	}


	template<class OUTPUT, class INPUT>
	inline xtl::xoptional<OUTPUT> viewCurvature(const CropView<INPUT>& in) {
		if (in.ncell() < 9) {
			return xtl::missing<OUTPUT>();
		}
		auto vars = calcCurveTempVars<OUTPUT, INPUT>(in);
		return -200 * (vars.D + vars.E);
	}

	template<class OUTPUT, class INPUT>
	inline xtl::xoptional<OUTPUT> viewProfileCurvature(const CropView<INPUT>& in) {
		if (in.ncell() < 9) {
			return xtl::missing<OUTPUT>();
		}
		auto vars = calcCurveTempVars<OUTPUT, INPUT>(in);
		return -200 * (vars.D * vars.G * vars.G + vars.E * vars.H * vars.H + vars.F * vars.G * vars.H) / (vars.G * vars.G + vars.H * vars.H);
	}

	template<class OUTPUT, class INPUT>
	inline xtl::xoptional<OUTPUT> viewPlanCurvature(const CropView<INPUT>& in) {
		if (in.ncell() < 9) {
			return xtl::missing<OUTPUT>();
		}
		auto vars = calcCurveTempVars<OUTPUT, INPUT>(in);
		return 200 * (vars.D * vars.H * vars.H + vars.E * vars.G * vars.G - vars.F * vars.G * vars.H) / (vars.G * vars.G + vars.H * vars.H);
	}

	template<class OUTPUT, class INPUT>
	inline xtl::xoptional<OUTPUT> viewSRI(const CropView<INPUT>& in) {
		static CoordTransform toLonLat;
		static size_t prevRasterHash = 0;
		if (in.ncell() < 9) {
			return xtl::missing<OUTPUT>();
		}
		try {
			if (prevRasterHash != in.parentRasterHash()) {
				prevRasterHash = in.parentRasterHash();
				toLonLat = CoordTransform(in.crs(), "EPSG:4326");
			}
			coord_t latitude = toLonLat.transformSingleXY(in.xFromCell(4), in.yFromCell(4)).y;
			latitude = latitude / 180. * M_PI;
			xtl::xoptional<OUTPUT> slope = viewSlopeRadians<OUTPUT, INPUT>(in);
			xtl::xoptional<OUTPUT> aspect = viewAspectRadians<OUTPUT, INPUT>(in);
			if (!slope.has_value()) {
				return xtl::missing<OUTPUT>();
			}
			if (!aspect.has_value()) {
				return xtl::missing<OUTPUT>();
			}
			return xtl::xoptional<OUTPUT>(
				(OUTPUT)(1 + std::cos(latitude) * std::cos(slope.value()) + std::sin(latitude) * std::sin(slope.value()) * std::cos(M_PI - aspect.value())));
		}
		catch (...) {
			return xtl::missing<OUTPUT>();
		}
	}

	template<class OUTPUT, class INPUT>
	inline xtl::xoptional<OUTPUT> viewTRI(const CropView<INPUT>& in) {
		if (in.ncell() < 9) {
			return xtl::missing<OUTPUT>();
		}
		xtl::xoptional<OUTPUT> tri = (OUTPUT)0;
#pragma warning(push)
#pragma warning(disable : 4244)
		for (cell_t cell : CellIterator(in)) {
			tri += ((in[cell] - in[4]) * (in[cell] - in[4]));
		}
#pragma warning(pop)
		tri.value() = (OUTPUT)std::sqrt(tri.value());
		return tri;
	}

	struct RowColOffset {
		rowcol_t rowOffset, colOffset;
	};
	inline std::vector<RowColOffset> cellOffsetsFromRadius(const Alignment& a, coord_t radius) {
		//This algorithm is pretty inefficient but it was really easy to write and should be fast enough for the actual use case

		std::vector<RowColOffset> out;

		rowcol_t xRadius = (rowcol_t)(radius / a.xres());
		rowcol_t yRadius = (rowcol_t)(radius / a.yres());

		coord_t maxdistSq = radius + (a.xres() + a.yres()) / 2;
		maxdistSq *= maxdistSq;
		coord_t mindistSq = radius - (a.xres() + a.yres()) / 2.;
		mindistSq = std::max(mindistSq, 0.);
		mindistSq *= mindistSq;
		for (rowcol_t row = -yRadius; row <= yRadius; ++row) {
			for (rowcol_t col = -xRadius; col <= xRadius; ++col) {
				coord_t xdistSq = col * a.xres();
				xdistSq *= xdistSq;
				coord_t ydistSq = row * a.yres();
				ydistSq *= ydistSq;
				coord_t distSq = ydistSq + xdistSq;
				if (distSq >= mindistSq && distSq <= maxdistSq) {
					out.push_back(RowColOffset{ row,col });
				}
			}
		}
		return out;
	}

	inline Raster<metric_t> topoPosIndex(const Raster<coord_t>& bufferedElev, coord_t radius, const Extent& unbuffered) {
		Raster<metric_t> tpi{ (Alignment)bufferedElev };
		std::vector<RowColOffset> circle = cellOffsetsFromRadius(bufferedElev, radius);
		
		for (rowcol_t row = 0; row < tpi.nrow(); ++row) {
			for (rowcol_t col = 0; col < tpi.ncol(); ++col) {
				cell_t cell = bufferedElev.cellFromRowColUnsafe(row, col);
				if (!bufferedElev[cell].has_value()) {
					continue;
				}
				coord_t center = bufferedElev[cell].value();

				coord_t numerator = 0;
				coord_t denominator = 0;

				for (RowColOffset offset : circle) {
					rowcol_t otherrow = row + offset.rowOffset;
					rowcol_t othercol = col + offset.colOffset;
					if (otherrow < 0 || othercol < 0 || otherrow >= bufferedElev.nrow() || othercol >= bufferedElev.ncol()) {
						continue;
					}
					auto v = bufferedElev.atRCUnsafe(otherrow, othercol);
					if (!v.has_value()) {
						continue;
					}
					denominator++;
					numerator += v.value();
				}

				if (denominator == 0) {
					continue;
				}
				tpi[cell].has_value() = true;
				tpi[cell].value() = (metric_t)(center - (numerator / denominator));
			}
		}

		tpi = cropRaster(tpi, unbuffered, SnapType::near);
		return tpi;
	}


	//This function takes two rasters with the same origin and resolution. It modifies the first raster in the following ways:
	//If one of its cells is nodata, and the corresponding cell in the new raster isn't, the value is replaced by the value in the new raster
	//If both rasters have values in the same cell, the value is replaced by the value in the new raster *unless* it occurs on the outermost rows or columns
	//The purpose of this function is to merge calculations from different CSM tiles; the assumption is that the values will be identical except in the buffer zone
	//Where you want to use the value from the raster where it's more interior
	template<class T>
	inline void overlayInside(Raster<T>& base, const Raster<T>& over) {
		CropView<T> view{ &base,over,SnapType::near };
		if (view.ncell() != over.ncell()) {
			throw AlignmentMismatchException("Alignment mismatch in overlayInside");
		}

		for (rowcol_t row = 0; row < view.nrow(); ++row) {
			for (rowcol_t col = 0; col < view.ncol(); ++col) {
				auto baseV = view.atRCUnsafe(row, col);
				auto overV = over.atRCUnsafe(row, col);
				if (overV.has_value() && !baseV.has_value()) {
					baseV.has_value() = true;
					baseV.value() = overV.value();
					continue;
				}
				if (overV.has_value() && baseV.has_value() &&
					row > 0 && row < view.nrow() - 1 &&
					col > 0 && col < view.ncol() - 1) {
					baseV.value() = overV.value();
				}
			}
		}
	}

	template<class T>
	inline void overlaySum(Raster<T>& base, const Raster<T>& over) {
		CropView<T> view{ &base, over, SnapType::near };
		if (view.ncell() != over.ncell()) {
			throw AlignmentMismatchException("Alignment mismatch in overlaySum");
		}

		for (rowcol_t row = 0; row < view.nrow(); ++row) {
			for (rowcol_t col = 0; col < view.ncol(); ++col) {
				auto baseV = view.atRCUnsafe(row, col);
				auto overV = over.atRCUnsafe(row, col);

				if (overV.has_value() && !baseV.has_value()) {
					baseV.has_value() = true;
					baseV.value() = overV.value();
					continue;
				}
				if (overV.has_value() && baseV.has_value()) {
					baseV.value() += overV.value();
				}
			}
		}
	}

	//returns a polygonization of the input raster. All cells with the same value will be part of the same MultiPolygon
	//cells connected orthgonally (i.e., not diagonally) will be part of the same Polygon
	//if attributes is nullptr, then the output will have an attribute table with a single column:
	//ID, which contains the value associated with that MultiPolygon
	//it will be either a Real or an Int64, depending on the type of T
	// 
	//if attributes is specified, then it must contain "ID" as a numeric column.
	//all values which exist both in the input raster and in the ID column will be present in the output,
	//with the other columns preserved

	template<class T>
	inline VectorsAndAttributes<MultiPolygon> rasterToMultiPolygonForTaos(const Raster<T>& r,
		const AttributeTable* attributes = nullptr) {

		//this algorithm is based on https://www.tandfonline.com/doi/epdf/10.1080/10824000809480639?needAccess=true
		//though with a decent number of modifications because the paper is very unclear in multiple parts
		//it's likely that the algorithm they had in mind is more efficient than the modified version I'm using here
		struct Vertex {
			//these coordinates always refer to the cell intersection in the upper left of the named cell
			rowcol_t row;
			rowcol_t col;
			Vertex(rowcol_t row, rowcol_t col) : row(row), col(col) {}
			bool operator==(const Vertex&) const = default;
		};
		enum class Handedness {
			rightHand,
			leftHand
		};
		struct Arc {
			std::list<Vertex> vertices;
			std::weak_ptr<Arc> nextArc;
			Handedness handedness;
			Arc(Vertex firstVertex, Handedness handedness) : handedness(handedness) {
				vertices.push_back(firstVertex);
			}
		};

		struct FullPolygonId {
			T multiPolygonId; //the value in the raster; i.e, which multipolygon this will eventually belong to
			cell_t polygonId; //the specific polygon the cell belongs to. Only needs to be unique within a multiPolygonId, not globally unique
			bool operator==(const FullPolygonId&) const = default;
		};
		struct PolygonIdHasher {
			size_t operator()(const FullPolygonId& id) const {
				return std::hash<T>()(id.multiPolygonId) ^ (std::hash<cell_t>()(id.polygonId) << 1);
			}
		};
		struct TwoArms {
			bool horizontalIsSolid = false; //i.e., does the cell above this one belong to the same polygon
			bool verticalIsSolid = false; //i.e., does the cell to the left of this one belong to the same polygon

			std::shared_ptr<Arc> horizontalArcOuter;
			std::shared_ptr<Arc> horizontalArcInner;
			std::shared_ptr<Arc> verticalArcOuter;
			std::shared_ptr<Arc> verticalArcInner;

			std::optional<FullPolygonId> thisPoly;
			std::optional<FullPolygonId> leftPoly;
			std::optional<FullPolygonId> abovePoly;
		};

		struct InProgressPolygon {
			std::shared_ptr<Arc> firstArc; //important because the first arc encountered will always be part of the outer ring
			std::unordered_set<std::shared_ptr<Arc>> allArcs;
		};
		std::unordered_map<T, std::unordered_map<cell_t, InProgressPolygon>> allPolygons;
		auto addArcToPoly = [&allPolygons](std::shared_ptr<Arc> arc, FullPolygonId id) {
			//the behavior of operator[] to default-construct the value if necessary is desirable here
			InProgressPolygon& thisPoly = allPolygons[id.multiPolygonId][id.polygonId];
			if (thisPoly.firstArc == nullptr) {
				thisPoly.firstArc = arc;
			}
			thisPoly.allArcs.insert(arc);
		};

		using small_cell_t = uint32_t; //memory is a big issue in this function, and in practice, the rasters I'm feeding to it never need the additional space a full int64 provides
		Raster<small_cell_t> polygonIdRaster{ (Alignment)r }; //combined with the input raster, this is enough to construct a FullPolygonId from a cell
		std::vector<std::vector<T>> valueByFinalRow; //for each row, which multipolygons no longer need to be tracked once you've finished that row?

		//first pass gets all the cells to have the correct polygon id, using union find

		Raster<small_cell_t>& parents = polygonIdRaster; //a simple re-label to make it easier to follow the way the usage of this object changes as the algorithm progresses
		auto findAncestor = [&](cell_t current)->small_cell_t {
			std::vector<cell_t> toChange{};
			small_cell_t parent = (small_cell_t)parents.atCellUnsafe(current).value();
			small_cell_t grandparent = (small_cell_t)parents.atCellUnsafe(parent).value();

			while (parent != grandparent) {
				toChange.push_back(current);
				current = parent;
				parent = grandparent;
				grandparent = parents.atCellUnsafe(grandparent).value();
			}
			for (cell_t id : toChange) {
				parents.atCellUnsafe(id).value() = grandparent;
			}
			return grandparent;
		};
		for (rowcol_t row = 0; row < r.nrow(); ++row) {
			for (rowcol_t col = 0; col < r.ncol(); ++col) {
				auto v = r.atRCUnsafe(row, col);
				if (!v.has_value()) {
					continue;
				}
				auto thisParent = parents.atRCUnsafe(row, col);
				thisParent.has_value() = true;
				bool leftMatch = false;
				bool aboveMatch = false;
				if (col > 0) {
					auto vLeft = r.atRCUnsafe(row, col - 1);
					leftMatch = v.value() == vLeft.value();
				}
				if (row > 0) {
					auto vAbove = r.atRCUnsafe(row - 1, col);
					aboveMatch = v.value() == vAbove.value();
				}

				if (!leftMatch && !aboveMatch) {
					parents.atRCUnsafe(row, col).value() = (small_cell_t)parents.cellFromRowColUnsafe(row, col);
				}
				else if (leftMatch) {
					if (aboveMatch) { //both match
						small_cell_t leftParent = findAncestor(parents.cellFromRowColUnsafe(row, col - 1));
						small_cell_t aboveParent = findAncestor(parents.cellFromRowColUnsafe(row - 1, col));
						thisParent.value() = leftParent;
						parents.atCellUnsafe(aboveParent).value() = leftParent;
					}
					else {
						small_cell_t leftParent = findAncestor(parents.cellFromRowColUnsafe(row, col - 1));
						thisParent.value() = leftParent;
					}

				}
				else { //only above matches
					small_cell_t aboveParent = findAncestor(parents.cellFromRowColUnsafe(row - 1, col));
					thisParent.value() = aboveParent;
				}
			}
		}

		//re-label everything based on aliases to ensure there's a single id per polygon, and populate valueByFinalRow
		//it should be fine to reuse the same raster used for holding parents for the final labels
		std::unordered_map<T, rowcol_t> finalRowByValue;
		for (cell_t cell : CellIterator(parents)) {
			auto v = r.atCellUnsafe(cell);
			if (!v.has_value()) {
				continue;
			}
			auto polygonId = polygonIdRaster.atCellUnsafe(cell);
			polygonId.has_value() = true;
			polygonId.value() = findAncestor(cell);
			finalRowByValue[v.value()] = polygonIdRaster.rowFromCellUnsafe(cell);
		}
		valueByFinalRow.resize(polygonIdRaster.nrow());
		for (const auto& keyValue : finalRowByValue) {
			valueByFinalRow[keyValue.second].push_back(keyValue.first);
		}

		auto formRing = [](std::shared_ptr<Arc> startArc,
			InProgressPolygon& inProgressPoly, const Alignment& a)->std::vector<CoordXY> {

				std::shared_ptr<Arc> currentArc = startArc;
				std::list<Vertex> ringRowCol;
				do {
					assert(currentArc->nextArc != nullptr);
					if (currentArc->handedness != Handedness::leftHand) {
						currentArc->vertices.reverse();
					}
					currentArc->vertices.pop_front();
					inProgressPoly.allArcs.erase(currentArc);
					ringRowCol.splice(ringRowCol.end(), currentArc->vertices);
					currentArc = currentArc->nextArc.lock();
				} while (currentArc != startArc);

				std::vector<CoordXY> ringXY;
				ringXY.reserve(ringRowCol.size());
				coord_t xAdj = a.xres() / 2.;
				coord_t yAdj = a.yres() / 2.;
				for (const Vertex& v : ringRowCol) {
					//it's important to keep these calls as the unsafe version;
					//we need the property that they produce a sensible answer even outside the bounds of the alignment
					coord_t x = a.xFromColUnsafe(v.col) - xAdj;
					coord_t y = a.yFromRowUnsafe(v.row) + yAdj;
					ringXY.push_back(CoordXY(x, y));
				}
				return ringXY;
			};
		auto formMultiPoly = [&](T id)->MultiPolygon {
			MultiPolygon thisMultiPoly{};
			for (auto& polyKeyValue : allPolygons.at(id)) {
				Polygon thisPoly{};
				InProgressPolygon& inProgressPoly = polyKeyValue.second;
				while (inProgressPoly.allArcs.size()) {
					if (inProgressPoly.firstArc) {
						std::vector<CoordXY> outerRing = formRing(inProgressPoly.firstArc, inProgressPoly, r);
						inProgressPoly.firstArc = nullptr;
						thisPoly = Polygon(outerRing);
					}
					else {
						std::vector<CoordXY> innerRing = formRing(*inProgressPoly.allArcs.begin(), inProgressPoly, r);
						thisPoly.addInnerRing(innerRing);
					}
				}
				thisMultiPoly.addPolygon(thisPoly);
			}
			return thisMultiPoly;
			};
		VectorsAndAttributes<MultiPolygon> outShp{ r.crs() };
		std::unordered_map<T, size_t> attributeRows;
		if (!attributes) {
			outShp.addNumericField<T>("ID");
		}
		else {
			const std::vector<std::string>& allFieldNames = attributes->getAllFieldNames();
			for (const std::string& name : allFieldNames) {
				switch (attributes->getFieldType(name)) {
				case FieldType::String:
					outShp.addStringField(name, attributes->getStringFieldWidth(name));
					break;
				case FieldType::Real:
					outShp.addRealField(name);
					break;
				case FieldType::Integer:
					outShp.addIntegerField(name);
					break;
				}
			}
			for (size_t i = 0; i < attributes->nrow(); ++i) {
				attributeRows.emplace((T)attributes->getIntegerField(i, "ID"), i);
			}
		}

		//finally, do some edge tracing
		//with this algorithm, edges are identified to the left and above the current cell, so we need to go to the "virtual" cells below and to the right of the real data
		std::vector<TwoArms> prevRow = std::vector<TwoArms>(polygonIdRaster.ncol() + 1);
		for (rowcol_t row = 0; row < polygonIdRaster.nrow() + 1; ++row) {
			std::vector<TwoArms> thisRow = std::vector<TwoArms>(polygonIdRaster.ncol() + 1);

			for (rowcol_t col = 0; col < thisRow.size(); ++col) {
				TwoArms& thisArms = thisRow[col];
				if (col < polygonIdRaster.ncol() && row < polygonIdRaster.nrow()) {
					if (polygonIdRaster.atRCUnsafe(row, col).has_value()) {
						thisArms.thisPoly = FullPolygonId{
							r.atRCUnsafe(row,col).value(),
							polygonIdRaster.atRCUnsafe(row, col).value()
						};
					}
				}

				bool leftHorizontalSolid = false;
				bool aboveVerticalSolid = false;
				TwoArms* leftArms = nullptr;
				TwoArms* aboveArms = nullptr;
				if (col == 0) {
					thisArms.verticalIsSolid = true;
				}
				else {
					leftArms = &thisRow[col - 1];
					thisArms.leftPoly = leftArms->thisPoly;
					leftHorizontalSolid = leftArms->horizontalIsSolid;
					if (col == thisRow.size() - 1) {
						thisArms.verticalIsSolid = true;
					}
					else {
						thisArms.verticalIsSolid = (thisArms.leftPoly != thisArms.thisPoly);
					}
				}


				if (row == 0) {
					thisArms.horizontalIsSolid = true;
				}
				else {
					aboveArms = &prevRow[col];
					thisArms.abovePoly = aboveArms->thisPoly;
					aboveVerticalSolid = aboveArms->verticalIsSolid;
					if (row == polygonIdRaster.nrow()) {
						thisArms.horizontalIsSolid = true;
					}
					else {
						thisArms.horizontalIsSolid = (thisArms.abovePoly != thisArms.thisPoly);
					}
				}


				constexpr uint8_t THIS_VERT_SOLID = 1 << 0;
				constexpr uint8_t THIS_HORIZ_SOLID = 1 << 1;
				constexpr uint8_t UPPER_VERT_SOLID = 1 << 2;
				constexpr uint8_t LEFT_HORIZ_SOLID = 1 << 3;
				uint8_t thisCase =
					(uint8_t)(thisArms.verticalIsSolid) << 0
					| (uint8_t)(thisArms.horizontalIsSolid) << 1
					| (uint8_t)(aboveVerticalSolid) << 2
					| (uint8_t)(leftHorizontalSolid) << 3;

				Vertex currentVertex = Vertex(row, col);

				//these are helper functions for common tasks in the below cases
				auto connectArcsLeftFirst = [](std::shared_ptr<Arc> one, std::shared_ptr<Arc> two) {
					assert(one->handedness != two->handedness);
					std::shared_ptr<Arc> left = one->handedness == Handedness::leftHand ? one : two;
					std::shared_ptr<Arc> right = one->handedness == Handedness::rightHand ? one : two;
					assert(left->nextArc == nullptr);
					left->nextArc = right;
					};
				auto connectArcsRightFirst = [](std::shared_ptr<Arc> one, std::shared_ptr<Arc> two) {
					assert(one->handedness != two->handedness);
					std::shared_ptr<Arc> left = one->handedness == Handedness::leftHand ? one : two;
					std::shared_ptr<Arc> right = one->handedness == Handedness::rightHand ? one : two;
					assert(right->nextArc == nullptr);
					right->nextArc = left;
					};
				auto closeUpperLeftPolyInner = [&]() {
					aboveArms->verticalArcOuter->vertices.push_back(currentVertex);
					leftArms->horizontalArcOuter->vertices.push_back(currentVertex);
					connectArcsLeftFirst(aboveArms->verticalArcOuter, leftArms->horizontalArcOuter);
					};
				auto upperVertContinueCornerInner = [&]() {
					aboveArms->verticalArcInner->vertices.push_back(currentVertex);
					thisArms.horizontalArcOuter = aboveArms->verticalArcInner;
					};
				auto leftHorizContinueCornerInner = [&]() {
					leftArms->horizontalArcInner->vertices.push_back(currentVertex);
					thisArms.verticalArcOuter = leftArms->horizontalArcInner;
					};
				auto makeNewArcsForInnerPoly = [&]() {
					thisArms.verticalArcInner = std::make_shared<Arc>(currentVertex, Handedness::rightHand);
					thisArms.horizontalArcInner = std::make_shared<Arc>(currentVertex, Handedness::leftHand);
					connectArcsRightFirst(thisArms.verticalArcInner, thisArms.horizontalArcInner);
					if (thisArms.thisPoly) {
						addArcToPoly(thisArms.verticalArcInner, *thisArms.thisPoly);
						addArcToPoly(thisArms.horizontalArcInner, *thisArms.thisPoly);
					}
					};
				auto continueOuterHoriz = [&]() {
					thisArms.horizontalArcOuter = leftArms->horizontalArcOuter;
					};
				auto continueOuterVert = [&]() {
					thisArms.verticalArcOuter = aboveArms->verticalArcOuter;
					};
				auto makeNewArcsForOuterPoly = [&]() {
					thisArms.verticalArcOuter = std::make_shared<Arc>(currentVertex, Handedness::leftHand);
					thisArms.horizontalArcOuter = std::make_shared<Arc>(currentVertex, Handedness::rightHand);
					connectArcsRightFirst(thisArms.verticalArcOuter, thisArms.horizontalArcOuter);
					if (thisArms.leftPoly) {
						addArcToPoly(thisArms.verticalArcOuter, *thisArms.leftPoly);
					}
					if (thisArms.abovePoly) {
						addArcToPoly(thisArms.horizontalArcOuter, *thisArms.leftPoly);
					}
					};
				auto continueInnerVert = [&]() {
					thisArms.verticalArcInner = aboveArms->verticalArcInner;
					};
				auto leftHorizContinueCornerOuter = [&]() {
					leftArms->horizontalArcOuter->vertices.push_back(currentVertex);
					thisArms.verticalArcInner = leftArms->horizontalArcOuter;
					};
				auto continueInnerHoriz = [&]() {
					thisArms.horizontalArcInner = leftArms->horizontalArcInner;
					};
				auto upperVertContinueCornerOuter = [&]() {
					aboveArms->verticalArcOuter->vertices.push_back(currentVertex);
					thisArms.horizontalArcInner = aboveArms->verticalArcOuter;
					};
				auto closeUpperLeftPolyOuter = [&]() {
					aboveArms->verticalArcInner->vertices.push_back(currentVertex);
					leftArms->horizontalArcInner->vertices.push_back(currentVertex);
					connectArcsLeftFirst(aboveArms->verticalArcInner, leftArms->horizontalArcInner);
					};

				//these cases are pulled from the paper cited above
				//when the upper vertical is solid, aboveArms should never be nullptr
				//when the left horizontal is solid, leftArms should never be nullptr
				switch (thisCase) {
				case (THIS_VERT_SOLID | THIS_HORIZ_SOLID | UPPER_VERT_SOLID | LEFT_HORIZ_SOLID): //case a
					closeUpperLeftPolyInner();
					upperVertContinueCornerInner();
					leftHorizContinueCornerInner();
					makeNewArcsForInnerPoly();
					break;
				case (THIS_VERT_SOLID | THIS_HORIZ_SOLID | LEFT_HORIZ_SOLID): //case b
					continueOuterHoriz();
					leftHorizContinueCornerInner();
					makeNewArcsForInnerPoly();
					break;
				case (THIS_VERT_SOLID | THIS_HORIZ_SOLID | UPPER_VERT_SOLID): //case c
					continueOuterVert();
					upperVertContinueCornerInner();
					makeNewArcsForInnerPoly();
					break;
				case (THIS_VERT_SOLID | THIS_HORIZ_SOLID): //case d
					makeNewArcsForInnerPoly();
					makeNewArcsForOuterPoly();
					break;
				case (THIS_VERT_SOLID | UPPER_VERT_SOLID | LEFT_HORIZ_SOLID): //case e
					closeUpperLeftPolyInner();
					leftHorizContinueCornerInner();
					continueInnerVert();
					break;
				case (THIS_VERT_SOLID | LEFT_HORIZ_SOLID): //case f
					leftHorizContinueCornerInner();
					leftHorizContinueCornerOuter();
					break;
				case (THIS_VERT_SOLID | UPPER_VERT_SOLID): //case g
					continueOuterVert();
					continueInnerVert();
					break;
				case (THIS_VERT_SOLID): //case h
					assert(false);
					throw std::runtime_error("impossible case in polygonization");
					break;
				case (THIS_HORIZ_SOLID | UPPER_VERT_SOLID | LEFT_HORIZ_SOLID): //case i
					closeUpperLeftPolyInner();
					continueInnerHoriz();
					upperVertContinueCornerInner();
					break;
				case (THIS_HORIZ_SOLID | LEFT_HORIZ_SOLID): //case j
					continueInnerHoriz();
					continueOuterHoriz();
					break;
				case (THIS_HORIZ_SOLID | UPPER_VERT_SOLID): //case k
					upperVertContinueCornerInner();
					upperVertContinueCornerOuter();
					break;
				case (THIS_HORIZ_SOLID): //case l
					assert(false);
					throw std::runtime_error("impossible case in polygonization");
					break;
				case (UPPER_VERT_SOLID | LEFT_HORIZ_SOLID): //case m
					closeUpperLeftPolyInner();
					closeUpperLeftPolyOuter();
					break;
				case (LEFT_HORIZ_SOLID): //case n
					assert(false);
					throw std::runtime_error("impossible case in polygonization");
					break;
				case (UPPER_VERT_SOLID): //case o
					assert(false);
					throw std::runtime_error("impossible case in polygonization");
					break;
				case (0): //case p
					//no action necessary
					break;
				}
			}
			prevRow = std::move(thisRow);
			if (row == 0) {
				continue;
			}
			for (T value : valueByFinalRow[row - 1]) {
				if (!attributes || attributeRows.contains(value)) {
					MultiPolygon thisMultiPoly = formMultiPoly(value);
					outShp.addGeometry(thisMultiPoly);
					if (!attributes) {
						outShp.back().setNumericField<T>("ID", value);
					}
					else {
						size_t attributeRow = attributeRows.at(value);
						for (const std::string& name : attributes->getAllFieldNames()) {
							switch (outShp.getFieldType(name)) {
							case FieldType::String:
								outShp.back().setStringField(name, attributes->getStringField(attributeRow, name));
								break;
							case FieldType::Real:
								outShp.back().setRealField(name, attributes->getRealField(attributeRow, name));
								break;
							case FieldType::Integer:
								outShp.back().setIntegerField(name, attributes->getIntegerField(attributeRow, name));
								break;
							}
						}
					}
				}
				allPolygons.erase(value);
			}
		}

		return outShp;
	}
}

#endif