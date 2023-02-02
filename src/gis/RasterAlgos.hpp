#pragma once
#ifndef lp_rasteralgos_h
#define lp_rasteralgos_h

#include"cropview.hpp"

namespace lapis {

	template<class OUTPUT, class INPUT>
	using ViewFunc = std::function<const xtl::xoptional<OUTPUT>(const CropView<INPUT>&)>;

	template<class OUTPUT, class INPUT>
	inline Raster<OUTPUT> aggregate(const Raster<INPUT>& r, const Alignment& a, ViewFunc<OUTPUT, INPUT> f) {
		Raster<OUTPUT> out{ a };
		for (cell_t cell = 0; cell < a.ncell(); ++cell) {
			Extent e = a.extentFromCell(cell);
			if (!e.overlaps(r)) {
				continue;
			}
			Raster<INPUT>* po = const_cast<Raster<INPUT>*>(&r); //I promise I'm not actually going to modify it; forgive this const_cast
			const CropView<INPUT> cv{ po, e, SnapType::near }; //see, I even made the view const

			auto v = f(cv);
			out[cell].has_value() = v.has_value();
			out[cell].value() = v.value();
		}
		return out;
	}

	template<class OUTPUT, class INPUT>
	inline Raster<OUTPUT> focal(const Raster<INPUT>& r, int windowSize, ViewFunc<OUTPUT, INPUT> f) {
		if (windowSize < 0 || windowSize % 2 != 1) {
			throw std::invalid_argument("");
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
	inline xtl::xoptional<OUTPUT> viewSlope(const CropView<INPUT>& in) {
		slopeComponents<OUTPUT> comp = getSlopeComponents<OUTPUT, INPUT>(in);
		if (!comp.nsSlope.has_value()) {
			return xtl::missing<OUTPUT>();
		}
		OUTPUT slopeProp = (OUTPUT)std::sqrt(comp.nsSlope.value() * comp.nsSlope.value() + comp.ewSlope.value() * comp.ewSlope.value());
		return xtl::xoptional<OUTPUT>((OUTPUT)std::atan(slopeProp));
	}
	template<class OUTPUT, class INPUT>
	inline xtl::xoptional<OUTPUT> viewAspect(const CropView<INPUT>& in) {
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