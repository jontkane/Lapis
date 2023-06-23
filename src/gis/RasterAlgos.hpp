#pragma once
#ifndef lp_rasteralgos_h
#define lp_rasteralgos_h

#include"cropview.hpp"

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

}

#endif