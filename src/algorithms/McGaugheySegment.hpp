#pragma once
#ifndef LP_MCGAUGHEYSEGMENT_H
#define LP_MCGAUGHEYSEGMENT_H
#include"algo_pch.hpp"
#include"TaoSegmentAlgorithm.hpp"

namespace lapis {

    enum class McGaugheySmoothType {
        fusion,
        simple,
        none
    };

    class McGaugheySegment : public TaoSegmentAlgorithm {
    public:
        McGaugheySegment(int nVertices,
            csm_t slopeChangeMultiplier,
            csm_t heightCutoffMultiplier,
            coord_t maxDistMultiplier,
            McGaugheySmoothType smoothType);

        SegmentResults segment(const Raster<csm_t>& bufferedCsm, const std::vector<IDedTao>& taos, const Extent& unbufferedExtent) override;

        const std::string& name() const override;

        void describeInPdf(MetadataPdf& pdf, TaoParameterGetter* getter) override;

        bool producesRaster() const override;
        bool producesVector() const override;

    private:
        int _nVertices; // = 16
        csm_t _slopeChangeMultiplier; // = 4
        csm_t _heightCutoffMultiplier; // = 2/3
        coord_t _maxDistMultiplier; // = 3/4
        McGaugheySmoothType _smoothType;

        Polygon _processOneTao(coord_t x, coord_t y, const Raster<csm_t>& csm) const;
        void _fusionSmooth(std::vector<coord_t>& vertexDistances) const;
        void _simpleSmooth(std::vector<coord_t>& vertexDistances) const;

    };
}

#endif