#pragma once
#ifndef LP_FINEINTHANDLER_H
#define LP_FINEINTHANDLER_H

#include"ProductHandler.hpp"

namespace lapis {
	class FineIntHandler : public ProductHandler {
	public:
		using ParamGetter = FineIntParameterGetter;
		FineIntHandler(ParamGetter* p);

		void handlePoints(const LidarPointVector& points, const Extent& e, size_t index) override;
		void handleDem(const Raster<coord_t>& dem, size_t index) override;
		void handleCsmTile(const Raster<csm_t>& bufferedCsm, cell_t tile) override;
		void cleanup() override;
		void reset() override;
		static size_t handlerRegisteredIndex;

		std::filesystem::path fineIntDir() const;
		std::filesystem::path fineIntTempDir() const;

	private:
		ParamGetter* _getter;

		std::string _numeratorBasename = "numerator";
		std::string _denominatorBasename = "denominator";
		std::string _fineIntBaseName;
	};
}

#endif