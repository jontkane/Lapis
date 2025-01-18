#include"algo_pch.hpp"
#include"AlreadyNormalized.hpp"
#include"..\utils\MetadataPdf.hpp"

namespace lapis {
	std::unique_ptr<DemAlgoApplier> AlreadyNormalized::getApplier(LasReader&& l, const CoordRef& outCrs)
	{
		return std::make_unique<AlreadyNormalizedApplier>(std::move(l), outCrs, _minHt, _maxHt);
	}
	void AlreadyNormalized::describeInPdf(MetadataPdf& pdf)
	{
		pdf.newPage();
		pdf.writePageTitle("Ground Model Algorithm");
		pdf.writeTextBlockWithWrap("The input data in this run had Z values representing height above ground, not "
			"elevation above sea level. Thus, no ground model was needed.");
	}
	AlreadyNormalizedApplier::AlreadyNormalizedApplier(LasReader&& l, const CoordRef& outCrs, coord_t minHt, coord_t maxHt)
		: DemAlgoApplier(std::move(l), outCrs, minHt, maxHt)
	{
	}
	AlreadyNormalizedApplier::AlreadyNormalizedApplier(const Extent& e, const CoordRef& outCrs, coord_t minHt, coord_t maxHt)
	{
	}
	std::shared_ptr<Raster<coord_t>> AlreadyNormalizedApplier::getDem()
	{
		return nullptr;
	}
	std::span<LasPoint> AlreadyNormalizedApplier::getPoints(size_t n)
	{
		_points = _las.getPoints(n);
		_points.transform(_crs);

		normalizePointVector(_points);

		return std::ranges::views::counted(_points.begin(), _points.size());
	}
	size_t AlreadyNormalizedApplier::pointsRemaining()
	{
		return _las.nPointsRemaining();
	}
}