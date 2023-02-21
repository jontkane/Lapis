#include"test_pch.hpp"
#include"..\gis\RasterAlgos.hpp"

namespace lapis {

	TEST(TopoTest, tpiTest) {
		Raster<coord_t> r{ Alignment(Extent(0,25,0,25),25,25) };
		
		//the testing strategy here is to make a raster whose values are distance from the center; the tpi at the center should thus be approximately equal to the radius used
		coord_t xCenter = r.xFromCol(25/2);
		coord_t yCenter = r.yFromRow(25/2);
		for (cell_t cell = 0; cell < r.ncell(); ++cell) {
			r[cell].has_value() = true;
			coord_t x = r.xFromCell(cell);
			coord_t y = r.yFromCell(cell);
			r[cell].value() = std::sqrt((x - xCenter) * (x - xCenter) + (y - yCenter) * (y - yCenter));
		}


		Extent unbuffered = Extent(2, 23, 2, 23);
		std::vector<coord_t> radii = { 3.,5.,10. };
		for (coord_t radius : radii) {
			Raster<metric_t> tpi = topoPosIndex(r, radius, unbuffered);
			EXPECT_EQ(unbuffered, (Extent)tpi);
			auto v = tpi.atXY(xCenter, yCenter);
			EXPECT_TRUE(v.has_value());
			EXPECT_NEAR(v.value(), -radius, 1);
		}
	}
}