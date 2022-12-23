#include<gtest/gtest.h>
#include"..\ProductHandler.hpp"
#include"ParameterSpoofer.hpp"

namespace lapis {

	void setReasonableFineIntDefaults(FineIntParameterSpoofer& spoof) {
		setReasonableSharedDefaults(spoof);
		spoof.setFineIntAlign(*spoof.metricAlign());
	}

	TEST(FineIntTest, handlepointstest) {
		FineIntParameterSpoofer spoof;
		setReasonableFineIntDefaults(spoof);

		FineIntHandler fih{ &spoof };


		std::filesystem::remove_all(spoof.outFolder());

		LidarPointVector points;
		points.push_back({ 0.5,0.5,3,2,0 });
		points.push_back({ 0.5,0.5,1,2,0 }); //below cutoff
		points.push_back({ 1.5,1.5,3,2,0 });
		points.push_back({ 1.5,1.5,3,2,0 });

		fih.handlePoints(points, *spoof.fineIntAlign(), 0);

		std::filesystem::path numName = fih.fineIntTempDir() / "0_num.tif";
		std::filesystem::path denomName = fih.fineIntTempDir() / "0_denom.tif";
		ASSERT_TRUE(std::filesystem::exists(numName));
		ASSERT_TRUE(std::filesystem::exists(denomName));

		Raster<intensity_t> expectedNum{ *spoof.fineIntAlign() };
		expectedNum.atXY(0.5, 0.5).has_value() = true;
		expectedNum.atXY(0.5, 0.5).value() = 2;
		expectedNum.atXY(1.5, 1.5).has_value() = true;
		expectedNum.atXY(1.5, 1.5).value() = 4;
		Raster<intensity_t> expectedDenom = expectedNum / (intensity_t)2.;

		Raster<intensity_t> actualNum{ numName.string() };
		Raster<intensity_t> actualDenom{ denomName.string() };

		ASSERT_TRUE(actualNum.isSameAlignment(actualDenom));
		ASSERT_TRUE(actualNum.isSameAlignment(expectedNum));

		for (cell_t cell = 0; cell < actualNum.ncell(); ++cell) {
			EXPECT_EQ(actualNum[cell].has_value(), expectedNum[cell].has_value());
			EXPECT_EQ(actualDenom[cell].has_value(), expectedDenom[cell].has_value());
			if (actualNum[cell].has_value()) {
				EXPECT_EQ(actualNum[cell].value(), expectedNum[cell].value());
				EXPECT_EQ(actualDenom[cell].value(), expectedDenom[cell].value());
			}
		}


		std::filesystem::remove_all(spoof.outFolder());
	}

	TEST(FineIntTest, handletiletest) {
		FineIntParameterSpoofer spoof;
		setReasonableFineIntDefaults(spoof);
		spoof.addLasExtent(*spoof.fineIntAlign());
		spoof.addLasExtent(*spoof.fineIntAlign());

		FineIntHandler fih{ &spoof };

		std::filesystem::remove_all(spoof.outFolder());

		cell_t testTile = 2;

		LidarPointVector points;
		points.push_back({ 0.5,0.5,3,5,0 });
		points.push_back({ 0.5,0.5,3,7,0 });
		points.push_back({ 2.5,0.5,3,9,0 });
		points.push_back({ 1.5,1.5,3,3,0 });

		fih.handlePoints(points, *spoof.fineIntAlign(), 0);
		
		points.clear();
		points.push_back({ 1.5,1.5,3,5,0 });
		points.push_back({ 1.5,1.5,1,1,0 });

		fih.handlePoints(points, *spoof.fineIntAlign(), 1);

		Raster<intensity_t> expected{ cropAlignment(*spoof.fineIntAlign(),spoof.layout()->extentFromCell(testTile), SnapType::near) };
		expected.atXY(0.5, 0.5).has_value() = true;
		expected.atXY(0.5, 0.5).value() = 6;
		expected.atXY(1.5, 1.5).has_value() = true;
		expected.atXY(1.5, 1.5).value() = 4;

		fih.handleCsmTile(Raster<csm_t>(),testTile);

		std::filesystem::path name = fih.fineIntDir() / (spoof.name() + "_MeanCanopyIntensity_" + nameFromLayout(*spoof.layout(), testTile) + ".tif");
		ASSERT_TRUE(std::filesystem::exists(name));
		Raster<intensity_t> actual{ name.string() };

		ASSERT_TRUE(actual.isSameAlignment(expected));
		for (cell_t cell = 0; cell < expected.ncell(); ++cell) {
			EXPECT_EQ(expected[cell].has_value(), actual[cell].has_value());
			if (expected[cell].has_value()) {
				EXPECT_EQ(expected[cell].value(), actual[cell].value());
			}
		}


		std::filesystem::remove_all(spoof.outFolder());
	}

}