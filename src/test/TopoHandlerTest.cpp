#include"test_pch.hpp"
#include"ParameterSpoofer.hpp"
#include"..\run\TopoHandler.hpp"

namespace lapis {
	class TopoHandlerProtectedAccess : public TopoHandler {
	public:
		TopoHandlerProtectedAccess(ParamGetter* getter) : TopoHandler(getter) {}

		std::vector<TopoMetric>& topoMetrics() {
			return _topoMetrics;
		}

		Raster<coord_t> meanElev() {
			return merger->meanElev();
		}
	};

	TEST(TopoHandlerTest, constructortest) {
		TopoParameterSpoofer spoof;
		setReasonableSharedDefaults(spoof);

		TopoHandlerProtectedAccess th(&spoof);
		th.prepareForRun();

		EXPECT_GT(th.topoMetrics().size(), 0);
	}

	TEST(TopoHandlerTest, handledemtest) {
		TopoParameterSpoofer spoof;
		setReasonableSharedDefaults(spoof);

		TopoHandlerProtectedAccess th(&spoof);
		th.prepareForRun();

		Raster<coord_t> expectedNum{ *spoof.metricAlign() };
		Raster<coord_t> expectedDenom{ *spoof.metricAlign() };
		
		Raster<coord_t> sampleDem{ Alignment(Extent(0,3,0,3),7,7) };

		for (cell_t cell = 0; cell < sampleDem.ncell(); ++cell) {
			if (cell % 2 != 0) {
				continue;
			}
			sampleDem[cell].has_value() = true;
			sampleDem[cell].value() = (coord_t)cell;

			coord_t x = sampleDem.xFromCell(cell);
			coord_t y = sampleDem.yFromCell(cell);
			expectedNum.atXY(x, y).has_value() = true;
			expectedNum.atXY(x, y).value() += cell;
			expectedDenom.atXY(x, y).has_value() = true;
			expectedDenom.atXY(x, y).value()++;
		}

		th.handleDem(sampleDem, 0);
		Raster<coord_t> meanElev = th.meanElev();
		ASSERT_TRUE(meanElev.consistentAlignment(expectedNum));

		for (cell_t cell = 0; cell < expectedNum.ncell(); ++cell) {
			if (expectedDenom[cell].has_value()) {
				EXPECT_TRUE(meanElev[cell].has_value());
				coord_t expectedValue = expectedNum[cell].value() / expectedDenom[cell].value();
				EXPECT_DOUBLE_EQ(expectedValue, meanElev[cell].value());

			}
			else {
				EXPECT_FALSE(meanElev[cell].has_value());
			}
		}
	}
}