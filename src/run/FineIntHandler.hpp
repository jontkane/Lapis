#pragma once
#ifndef LP_FINEINTHANDLER_H
#define LP_FINEINTHANDLER_H

#include"ProductHandler.hpp"

namespace lapis {
	class FineIntHandler : public ProductHandler {
	public:
		using ParamGetter = FineIntParameterGetter;
		FineIntHandler(ParamGetter* p);

		void prepareForRun() override;
		void handlePoints(const std::span<LasPoint>& points, const Extent& e, size_t index) override;
		void finishLasFile(const Extent& e, size_t index) override;
		void handleDem(const Raster<coord_t>& dem, size_t index) override;
		void handleCsmTile(const Raster<csm_t>& bufferedCsm, cell_t tile) override;
		void cleanup() override;
		void reset() override;
		bool doThisProduct() override;
		std::string name() override;
		static size_t handlerRegisteredIndex;

		void describeInPdf(MetadataPdf& pdf) override;

		std::filesystem::path fineIntDir() const;
		std::filesystem::path fineIntTempDir() const;

	protected:
		std::string _numeratorBasename = "numerator";
		std::string _denominatorBasename = "denominator";
		std::string _fineIntBaseName;

		ParamGetter* _getter;

		struct NumDenom {
			NumDenom(const Alignment& a) : num(a), denom(a) {}

			Raster<intensity_t> num;
			Raster<intensity_t> denom;
		};
		std::unordered_map<size_t, NumDenom> _tiles;
	};
}

#endif