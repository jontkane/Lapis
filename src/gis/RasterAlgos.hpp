#pragma once
#ifndef lp_rasteralgos_h
#define lp_rasteralgos_h

#include"cropview.hpp"

namespace lapis {

	template<class T, class RETURN>
	using ViewFunc = std::function<const xtl::xoptional<RETURN>(const CropView<T>&)>;

	template<class T, class RETURN>
	inline Raster<RETURN> aggregate(const Raster<T>& r, const Alignment& a, ViewFunc<T, RETURN> f) {
		Raster<RETURN> out{ a };
		for (cell_t cell = 0; cell < a.ncell(); ++cell) {
			Extent e = a.extentFromCell(cell);
			if (!e.overlaps(r)) {
				continue;
			}
			Raster<T>* po = const_cast<Raster<T>*>(&r); //I promise I'm not actually going to modify it; forgive this const_cast
			const CropView<T> cv{ po, e, SnapType::near }; //see, I even made the view const

			auto v = f(cv);
			out[cell].has_value() = v.has_value();
			out[cell].value() = v.value();
		}
		return out;
	}

	template<class T, class RETURN>
	inline Raster<RETURN> focal(const Raster<T>& r, int windowSize, ViewFunc<T, RETURN> f) {
		if (windowSize < 0 || windowSize % 2 != 1) {
			throw std::invalid_argument("");
		}

		Raster<RETURN> out{ r };
		int lookDist = (windowSize - 1) / 2;

		for (cell_t cell = 0; cell < r.ncell(); ++cell) {
			Extent e = r.extentFromCell(cell);
			e = Extent(e.xmin() - lookDist * r.xres(), e.xmax() + lookDist * r.xres(), e.ymin() - lookDist * r.yres(), e.ymax() + lookDist * r.yres());
			Raster<T>* po = const_cast<Raster<T>*>(&r);
			const CropView<T> cv{ po,e,SnapType::near };

			auto v = f(cv);
			out[cell].has_value() = v.has_value();
			out[cell].value() = v.value();
		}
		return out;
	}

	template<class T>
	xtl::xoptional<T> viewMax(const CropView<T>& in) {
		bool hasvalue = false;
		T value = std::numeric_limits<T>::lowest();

		for (cell_t cell = 0; cell < in.ncell(); ++cell) {
			hasvalue = hasvalue || in[cell].has_value();
			if (in[cell].has_value()) {
				value = std::max(value, in[cell].value());
			}
		}
		return xtl::xoptional<T>(value, hasvalue);
	}

	template<class T>
	xtl::xoptional<T> viewMean(const CropView<T>& in) {
		T numerator = 0;
		T denominator = 0;
		for (cell_t cell = 0; cell < in.ncell(); ++cell) {
			if (in[cell].has_value()) {
				denominator++;
				numerator += in[cell].value();
			}
		}
		if (denominator) {
			return xtl::xoptional<T>(numerator / denominator);
		}
		return xtl::missing<T>();
	}

	template<class T>
	xtl::xoptional<T> viewStdDev(const CropView<T>& in) {
		//not calculating the mean twice is a potential optimization if it matters
		xtl::xoptional<T> mean = viewMean(in);
		T denominator = 0;
		T numerator = 0;
		if (!mean.has_value()) {
			return xtl::missing<T>();
		}
		for (cell_t cell = 0; cell < in.ncell(); ++cell) {
			if (in[cell].has_value()) {
				denominator++;
				T temp = in[cell].value() - mean.value();
				numerator += temp * temp;
			}
		}
		return xtl::xoptional<T>(std::sqrt(numerator / denominator));
	}

	template<class T>
	xtl::xoptional<T> viewRumple(const CropView<T>& in) {
		T sum = 0;
		T nTriangle = 0;

		//the idea here is to interpolate the height at the intersection between cells
		//then draw isoceles right triangles with the hypotenuse going between adjacent cell centers and the opposite point at a cell intersection
		

		for (rowcol_t row = 1; row < in.nrow(); ++row) {
			for (rowcol_t col = 1; col < in.ncol(); ++col) {
				auto lr = in.atRCUnsafe(row, col);
				auto ll = in.atRCUnsafe(row, col - 1);
				auto ur = in.atRCUnsafe(row - 1, col);
				auto ul = in.atRCUnsafe(row - 1, col - 1);

				T numerator = 0;
				T denominator = 0;
				auto runningSum = [&](auto x) {
					if (x.has_value()) {
						denominator++;
						numerator += x.value();
					}
				};

				runningSum(lr);
				runningSum(ll);
				runningSum(ur);
				runningSum(ul);

				if (denominator == 0) {
					continue;
				}
				T mid = numerator / denominator;


				//a and b are the heights of two cells, and mid is the interpolated height of one of the center intersections adjacent to them
				//this calculates the ratio between the area of this triangle and its projection to the ground
				auto triangleAreaRatio = [&](xtl::xoptional<T> a, xtl::xoptional<T> b, T mid) {
					if (!a.has_value() || !b.has_value()) {
						return;
					}
					T longdiff = a.value() - b.value();
					T middiff = a.value() - mid;

					//the CSMs produced by lapis will always have equal xres and yres
					//this formula doesn't look symmetric between a and b, but it actually is if you expand it out
					//this version has slightly fewer calculations than the clearer version
					//this was derived from the gram determinant, after a great deal of simplification
					nTriangle++;
					sum += (2. / in.xres()) * std::sqrt(0.5 + middiff * middiff + longdiff * longdiff / 2. - longdiff * middiff);
				};
				triangleAreaRatio(ll, ul, mid);
				triangleAreaRatio(ll, lr, mid);
				triangleAreaRatio(ul, ur, mid);
				triangleAreaRatio(ur, lr, mid);
			}
		}

		if (nTriangle == 0) {
			return xtl::missing<T>();
		}
		return sum / nTriangle;
	}

	template<class T>
	xtl::xoptional<T> viewSum(const CropView<T>& in) {
		T value = 0;
		for (cell_t cell = 0; cell < in.ncell(); ++cell) {
			if (in[cell].has_value()) {
				value += in[cell].value();
			}
		}
		return xtl::xoptional<T>(value);
	}

	template<class T>
	xtl::xoptional<T> viewCount(const CropView<T>& in) {
		T count = 0;
		for (cell_t cell = 0; cell < in.ncell(); ++cell) {
			if (in[cell].has_value()) {
				count++;
			}
		}
		return xtl::xoptional<T>(count);
	}

	//this function will not have the expected behavior unless the CropView is 3x3
	template<class RETURN>
	struct slopeComponents {
		xtl::xoptional<RETURN> nsSlope, ewSlope;
	};
	template<class T, class RETURN>
	inline slopeComponents<RETURN> getSlopeComponents(const CropView<T>& in) {
		slopeComponents<RETURN> out;
		if (in.ncell() < 9) {
			out.nsSlope = xtl::missing<RETURN>();
			out.ewSlope = xtl::missing<RETURN>();
			return out;
		}
		for (cell_t cell = 0; cell < in.ncell(); ++cell) {
			if (!in[cell].has_value()) {
				out.nsSlope = xtl::missing<RETURN>();
				out.ewSlope = xtl::missing<RETURN>();
				return out;
			}
		}
		out.nsSlope = (in[6].value() + 2 * in[7].value() + in[8].value() - in[0].value() - 2 * in[1].value() - in[2].value()) / (8. * in.yres());
		out.ewSlope = (in[0].value() + 2 * in[3].value() + in[6].value() - in[2].value() - 2 * in[5].value() - in[8].value()) / (8. * in.xres());
		return out;
	}

	template<class T, class RETURN>
	inline xtl::xoptional<RETURN> viewSlope(const CropView<T>& in) {
		slopeComponents<RETURN> comp = getSlopeComponents<T, RETURN>(in);
		if (!comp.nsSlope.has_value()) {
			return xtl::missing<RETURN>();
		}
		RETURN slopeProp = std::sqrt(comp.nsSlope.value() * comp.nsSlope.value() + comp.ewSlope.value() * comp.ewSlope.value());
		return xtl::xoptional<RETURN>(std::atan(slopeProp));
	}
	template<class T, class RETURN>
	inline xtl::xoptional<RETURN> viewAspect(const CropView<T>& in) {
		slopeComponents<RETURN> comp = getSlopeComponents<T, RETURN>(in);
		if (!comp.nsSlope.has_value()) {
			return xtl::missing<RETURN>();
		}
		RETURN ns = comp.nsSlope.value();
		RETURN ew = comp.ewSlope.value();
		if (ns > 0) {
			if (ew > 0) {
				return std::atan(ew / ns);
			}
			else if (ew < 0) {
				return 2 * M_PI + std::atan(ew / ns);
			}
			else {
				return 0;
			}
		}
		else if (ns < 0) {
			return M_PI + std::atan(ew / ns);
		}
		else {
			if (ew > 0) {
				return M_PI / 2.;
			}
			else if (ew < 0) {
				return 3. * M_PI / 2.;
			}
			else {
				return xtl::missing<RETURN>();
			}
		}
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
			throw AlignmentMismatchException();
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
			throw AlignmentMismatchException();
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