#include"test_pch.hpp"
#include"..\gis\CoordRef.hpp"

namespace lapis {
	TEST(CoordRefTest, constructor) {
		std::string testfilefolder = std::string(LAPISTESTFILES);
		std::vector<std::string> sources = {
		""
		, "EPSG:2927"
		, "2927"
		, "+proj=utm +zone=11 +datum=NAD83 +units=m +no_defs +type=crs" //EPSG 26911
		, "+proj=utm +zone=11 +datum=NAD83 +units=m +no_defs" // EPSG 26911
		, testfilefolder + "testlaz10.laz" // EPSG 2927+Unknown
		, testfilefolder + "testlaz14.laz" // EPSG 6340+5703
		, testfilefolder + "testraster.img" // EPSG 26911
		, testfilefolder + "testvect.shp" // EPSG 6340
		, testfilefolder + "testvect.prj" // EPSG 6340
		};

		std::vector<std::string> expected = {
		"Unknown"
		, "2927"
		, "2927"
		, "26911"
		, "26911"
		, "32149+Unknown"
		, "6340+5703"
		, "26911"
		, "6340"
		, "6340"
		};

		for (int i = 0; i < sources.size(); ++i) {
			CoordRef crs{ sources[i] };
			EXPECT_EQ(crs.getEPSG(), expected[i]);
		}
	}

	TEST(CoordRefTest, xyUnits) {
		std::vector<CoordRef> source = {
			""
			, "2927"
			, "6340"
			, "2927+5703"
			, "4269"
		};
		std::vector<LinearUnit> expected = {
			linearUnitPresets::unknownLinear
			, LinearUnit{"US survey foot",0.30480060960122,unitType::linear, unitStatus::statedInCRS}
			, LinearUnit{"metre",1.,unitType::linear,unitStatus::statedInCRS }
			, LinearUnit{"US survey foot",0.30480060960122,unitType::linear, unitStatus::statedInCRS}
			, LinearUnit{"degree", 0.0174532925199433, unitType::angular, unitStatus::statedInCRS}
		};

		for (int i = 0; i < source.size(); ++i) {
			CoordRef crs{ source[i] };
			LinearUnit xy = crs.getXYUnits();
			EXPECT_EQ(xy.name, expected[i].name);
			EXPECT_NEAR(xy.convFactor, expected[i].convFactor, 0.0000001);
			EXPECT_EQ(xy.status, expected[i].status);
			EXPECT_EQ(xy.type, expected[i].type);
		}
	}

	TEST(CoordRefTest, zUnits) {
		std::vector<CoordRef> source = {
			""
			, CoordRef("",linearUnitPresets::internationalFoot)
			, "2927"
			, "2927+5703"
			, CoordRef("2927",linearUnitPresets::meter)
			, "4269"
		};
		std::vector<LinearUnit> expected = {
			linearUnitPresets::unknownLinear
			, linearUnitPresets::internationalFoot
			, LinearUnit{"US survey foot",0.30480060960122,unitType::linear, unitStatus::inferredFromCRS}
			, LinearUnit{ "metre",1.,unitType::linear,unitStatus::statedInCRS }
			, linearUnitPresets::meter
			, linearUnitPresets::unknownLinear
		};

		for (int i = 0; i < source.size(); ++i) {
			const LinearUnit& z = source[i].getZUnits();
			EXPECT_EQ(z.name, expected[i].name);
			EXPECT_NEAR(z.convFactor, expected[i].convFactor, 0.0000001);
			EXPECT_EQ(z.status, expected[i].status);
			EXPECT_EQ(z.type, expected[i].type);
		}
	}

	TEST(CoordRefTest, horizConsistent) {
		CoordRef against{ "6340+5703" };
		std::vector<CoordRef> source = {
			""
			, "6340"
			, "2927+5703"
			, "4269"
			, "6340+5702"
		};
		std::vector<bool> expected = {
			true
			, true
			, false
			, false
			, true
		};

		for (int i = 0; i < source.size(); ++i) {
			EXPECT_TRUE(against.isConsistentHoriz(source[i]) == expected[i]) << "Failed on test " + std::to_string(i);
			EXPECT_TRUE(source[i].isConsistentHoriz(against) == expected[i]) << "Failed on test " + std::to_string(i);
		}
	}

	TEST(CoordRefTest, vertConsistent) {
		CoordRef against{ "6340+5703" };
		std::vector<CoordRef> source = {
			""
			, "2927"
			, "2927+5703"
			, "4269"
			, "6340+5702"
			, CoordRef("6340",linearUnitPresets::meter)
			, CoordRef("6340+5703",linearUnitPresets::internationalFoot)
		};
		std::vector<bool> expected = {
			true
			, false
			, true
			, true
			, false
			, true
			, false
		};

		for (int i = 0; i < source.size(); ++i) {
			EXPECT_TRUE(against.isConsistentZUnits(source[i]) == expected[i]) << "Failed on test " + std::to_string(i);
			EXPECT_TRUE(source[i].isConsistentZUnits(against) == expected[i]) << "Failed on test " + std::to_string(i);
		}
	}

	TEST(CoordRefTest, isProjected) {
		CoordRef crs = "2927";
		EXPECT_TRUE(crs.isProjected());

		crs = "2927+5703";
		EXPECT_TRUE(crs.isProjected());

		crs = "4326";
		EXPECT_FALSE(crs.isProjected());

		crs = "4326+5703";
		EXPECT_FALSE(crs.isProjected());
	}
}