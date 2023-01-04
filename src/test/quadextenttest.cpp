#include"test_pch.hpp"
#include"..\gis\QuadExtent.hpp"

namespace lapis {

	TEST(QuadExtentTest, overlaps) {
		CoordRef utm{ "32611" };
		CoordRef stateplane{ "2927" };

		QuadExtent base{ Extent(0,100,200,300, utm), stateplane };

		std::vector<Extent> test = {
			Extent{ 50,150,250,350, utm } //overlaps
			,Extent{ -50,150,150,350, utm } //contains
			,Extent{ 25,75,225,275, utm } //contained
			,Extent{ 200,300,400,500, utm } //no overlap
		};
		std::vector<bool> expected = {
			true
			,true
			,true
			,false
		};
		for (int i = 0; i < test.size(); ++i) {
			EXPECT_EQ(base.overlaps(test[i]), expected[i]);
		}
	}

	TEST(QuadExtentTest, contains) {
		CoordRef utm{ "32611" };
		CoordRef stateplane{ "2927" };

		QuadExtent base{ Extent(0,100,200,300, utm), stateplane };

		struct xy {
			coord_t x, y;
		};
		std::vector<xy> test = {
			{50,250}
			,{-50,250}
			,{50,150}
			,{50,350}
			,{150,250}
			,{99,201}
		};
		CoordTransform tr(utm, stateplane);
		tr.transformXY(test);
		std::vector<bool> expected = {
			true
			,false
			,false
			,false
			,false
			,true
		};

		for (int i = 0; i < test.size(); ++i) {
			EXPECT_EQ(base.contains(test[i].x, test[i].y), expected[i]);
		}

	}
}