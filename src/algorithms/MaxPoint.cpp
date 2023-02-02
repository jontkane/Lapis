#include"MaxPoint.hpp"
#include"..\utils\MetadataPdf.hpp"
#include"..\parameters\ParameterGetter.hpp"

namespace lapis {
	MaxPoint::MaxPoint(coord_t footprintDiameter)
		:_footprintRadius(footprintDiameter/2.)
	{
	}
	Raster<csm_t> MaxPoint::createCsm(const LidarPointVector& points, const Alignment& a)
	{
		Raster<csm_t> csm{ Alignment(bufferExtent(a,_footprintRadius),a.xOrigin(),a.yOrigin(),a.xres(),a.yres()) };

		const coord_t diagonal = _footprintRadius / std::sqrt(2);
		const coord_t epsilon = -0.000001; //the purpose of this value is to avoid ties caused by the footprint algorithm messing with the high points TAO algorithm
		struct XYEpsilon {
			coord_t x, y, epsilon;
		};
		std::vector<XYEpsilon> circle;
		if (_footprintRadius == 0) {
			circle = { {0.,0.,0.} };
		}
		else {
			circle = { {0,0,0},
				{_footprintRadius,0,epsilon},
				{-_footprintRadius,0,epsilon},
				{0,_footprintRadius,epsilon},
				{0,-_footprintRadius,epsilon},
				{diagonal,diagonal,2 * epsilon},
				{diagonal,-diagonal,2 * epsilon},
				{-diagonal,diagonal,2 * epsilon},
				{-diagonal,-diagonal,2 * epsilon} };
		}

		//there are generally far more points than cells, so it's worth it to do some goofy stuff to reduce the amount of work in the core loop
		//in particular, this escapes the need to handle has_value() inside the point loop
		for (cell_t cell = 0; cell < csm.ncell(); ++cell) {
			csm[cell].value() = std::numeric_limits<csm_t>::lowest();
		}

		for (const LasPoint& p : points) {
			for (const XYEpsilon& direction : circle) {
				coord_t x = p.x + direction.x;
				coord_t y = p.y + direction.y;
				csm_t z = p.z + direction.epsilon;
				cell_t cell = csm.cellFromXYUnsafe(x, y);

				csm[cell].value() = std::max(csm[cell].value(), z);
			}
		}

		for (cell_t cell = 0; cell < csm.ncell(); ++cell) {
			if (csm[cell].value() > std::numeric_limits<csm_t>::lowest()) {
				csm[cell].has_value() = true;
			}
		}

		return csm;
	}
	csm_t MaxPoint::combineCells(csm_t a, csm_t b)
	{
		return std::max(a, b);
	}
	void MaxPoint::describeInPdf(MetadataPdf& pdf, CsmParameterGetter* getter)
	{
		pdf.writeSubsectionTitle("CSM Creation Algorithm");
		std::stringstream desc;
		desc << "The CSM for this run was produced with the Max Point algorithm. In this algorithm, the value of a cell "
			"in the output raster is equal to the maximum height of lidar returns that fall in that cell. ";
		if (_footprintRadius > 0) {
			desc << "Returns were treated as circles with radius " <<
				pdf.numberWithUnits(_footprintRadius, getter->unitSingular(), getter->unitPlural()) << ", ";
			desc << "in order to account for the width of a lidar pulse.";
		}
		pdf.writeTextBlockWithWrap(desc.str());
	}
	coord_t MaxPoint::footprintRadius()
	{
		return _footprintRadius;
	}
}