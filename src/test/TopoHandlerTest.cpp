#include"test_pch.hpp"
#include"ParameterSpoofer.hpp"
#include"..\run\TopoHandler.hpp"

namespace lapis {
	class TopoHandlerProtectedAccess : public TopoHandler {
	public:
		TopoHandlerProtectedAccess(ParamGetter* getter) : TopoHandler(getter) {}

		Raster<coord_t>& elevNumerator() {
			return _elevNumerator;
		}
		Raster<coord_t>& elevDenominator() {
			return _elevDenominator;
		}
		std::vector<TopoMetric>& topoMetrics() {
			return _topoMetrics;
		}
	};

	TEST(TopoHandlerTest, constructortest) {
		TopoParameterSpoofer spoof;
		setReasonableSharedDefaults(spoof);

		TopoHandlerProtectedAccess th(&spoof);

		EXPECT_TRUE(th.elevNumerator().isSameAlignment(*spoof.metricAlign()));
		EXPECT_TRUE(th.elevDenominator().isSameAlignment(*spoof.metricAlign()));

		EXPECT_GT(th.topoMetrics().size(), 0);
	}

	TEST(TopoHandlerTest, handledemtest) {
		TopoParameterSpoofer spoof;
		setReasonableSharedDefaults(spoof);

		TopoHandlerProtectedAccess th(&spoof);

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

		for (cell_t cell = 0; cell < expectedNum.ncell(); ++cell) {
			EXPECT_EQ(expectedNum[cell].has_value(), th.elevNumerator()[cell].has_value());
			EXPECT_EQ(expectedDenom[cell].has_value(), th.elevDenominator()[cell].has_value());
			if (expectedNum[cell].has_value()) {
				EXPECT_EQ(expectedNum[cell].value(), th.elevNumerator()[cell].value());
				EXPECT_EQ(expectedDenom[cell].value(), th.elevDenominator()[cell].value());
			}
		}
	}
}