#include"..\gis\CropView.hpp"
#include<gtest/gtest.h>

namespace lapis {
	TEST(CropViewTest, valueAccess) {
		Raster<int> r{ Alignment(Extent(0,3,0,3),3,3) };
		for (int i = 0; i < 9; ++i) {
			r[i].value() = i;
			r[i].has_value() = true;
		}

		Extent cropE{ 1,3,1,3 };
		CropView<int> cv{ &r,cropE }; //should be {{1,2},{4,5}}

		EXPECT_EQ(1, cv[0].value());
		EXPECT_EQ(5, cv.atRC(1, 1).value());
		EXPECT_EQ(2, cv.atXY(2.5, 2.5).value());

		EXPECT_EQ(2, cv.nrow());
		EXPECT_EQ(2, cv.ncol());

		cv[0].value() = 10;
		EXPECT_EQ(10, r[1].value());
	}
}