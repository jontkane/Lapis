#pragma once
#ifndef LP_AOIPARAMETER_H
#define LP_AOIPARAMETER_H

#include"Parameter.hpp"

namespace lapis {

	class AoIParameter : public Parameter {
	public:
		AoIParameter();

		void addToCmd(BoostOptDesc& visible,
			BoostOptDesc& hidden) override;

		std::ostream& printToIni(std::ostream& o) override;

		ParamCategory getCategory() const override;

		void renderGui() override;

		void importFromBoost() override;
		void updateUnits() override;

		bool prepareForRun() override;
		void cleanAfterRun() override;

		void reset() override;
		static size_t parameterRegisteredIndex;

		//this function indicates whether an area (presumably representing a las file) should be entirely skipped
		bool overlapsAoI(const Extent& e);

		//this function adds a filter to the LasReader, if appropriate. It will perform the transformations necessary to ensure projections are handled properly.
		void addFilter(LasReader& lr);

		//this function modified lpv to only contain points that fall within the AoI. e is an extent which much contain all points in lpv; presumably the extent of the las file that the points came from
		void filterPointsInPlace(LidarPointVector& lpv, const Extent& e);

	private:

		bool _runPrepared = false;

		void _errorWindow();

		std::string _errorMessage = "";

		enum class AoIType {
			None = 0,
			Extent = 1,
			Raster = 2,
			Vector = 3,
			Error = -1
		};

		class AoITypeDecider {
		public:
			int operator()(const std::string& s) const;
			std::string operator()(int i) const;
		};
		RadioSelect<AoITypeDecider, AoIType> _typeRadio{ "Area of Interest:","aoi" };

		RadioBoolean _specifyManually{ "","Specify Manually","Specify From File" };
		NumericTextBox _extentXMin{ "Min X","aoixmin",0 };
		NumericTextBox _extentXMax{ "Max X","aoixmax",0 };
		NumericTextBox _extentYMin{ "Min Y","aoiymin",0 };
		NumericTextBox _extentYMax{ "Max Y","aoiymax",0 };
		CRSInput _extentCRS{ "CRS","aoicrs","Same as Output" };
		std::string _extentFile = "";
		std::unique_ptr<Extent> _extentFilter;
	};
}

#endif