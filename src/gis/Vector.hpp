#pragma once
#include"gis_pch.hpp"
#include"CoordRef.hpp"
#include"Raster.hpp"

namespace lapis {

	class GisVector {
	public:
		CoordRef& crs();
		virtual std::string wkt() = 0;
	protected:
		CoordRef _crs;
	};

	class Polygon : GisVector {
	public:
	private:
	};

	class MultiPolygon : GisVector {
	public:
	private:
		std::vector<Polygon> _polygons;
	};

	template<class T>
	class VectorsAndAttributes {
	public:
		UniqueGdalDataset asGdal() const;
	private:
	};

	//returns a polygonization of the input raster. All cells with the same value will be part of the same MultiPolygon
	//cells connected orthgonally (i.e., not diagonally) will be part of the same Polygon
	//the output will have an attribute table with a single column: ID, which contains the value associated with that MultiPolygon
	//it will be either a Real or an Int64, depending on the type of T
	template<class T>
	inline VectorsAndAttributes<MultiPolygon> rasterToMultiPolygon(const Raster<T>& r) {

		//this algorithm is based on https://www.tandfonline.com/doi/epdf/10.1080/10824000809480639?needAccess=true
		//though with a decent number of modifications because the paper is very unclear in multiple parts
		//it's likely that the algorithm they had in mind is more efficient than the modified version I'm using here
		struct Vertex {
			//these coordinates always refer to the cell intersection in the upper left of the named cell
			rowcol_t row;
			rowcol_t col;
			Vertex(rowcol_t row, rowcol_t col) : row(row), col(col) {}
			bool operator==(const Vertex&) const = default;
		};
		constexpr bool CLOCKWISE = true;
		constexpr bool COUNTERCLOCKWISE = false;
		struct Arc {
			std::list<Vertex> vertices;
			std::shared_ptr<Arc> nextArc = nullptr;
			std::shared_ptr<Arc> prevArc = nullptr;
			bool clockwise;
			Arc(Vertex firstVertex, bool clockwise) : clockwise(clockwise) {
				vertices.push_back(firstVertex);
			}
		};

		//one should be counterclockwise of two
		//two will be added as the *next* arc after one, so this makes the ring as a whole go clockwise
		auto connectArcs = [](std::shared_ptr<Arc> one, std::shared_ptr<Arc> two) {
			assert(one->clockwise != two->clockwise && "arc connection error in polygonization");
			assert(!(one->nextArc != nullptr || two->prevArc != nullptr) && "arc connection error in polygonization");

			one->nextArc = two;
			two->prevArc = one;
		};

		struct FullPolygonId {
			T multiPolygonId; //the value in the raster; i.e, which multipolygon this will eventually belong to
			int polygonId; //the specific polygon the cell belongs to. Only needs to be unique within a multiPolygonId, not globally unique
			bool operator==(const FullPolygonId&) const = default;
		};
		struct TwoArms {
			bool horizontalIsSolid; //i.e., does the cell above this one belong to the same polygon
			bool verticalIsSolid; //i.e., does the cell to the left of this one belong to the same polygon

			std::shared_ptr<Arc> horizontalArcOuter;
			std::shared_ptr<Arc> horizontalArcInner;
			std::shared_ptr<Arc> verticalArcOuter;
			std::shared_ptr<Arc> verticalArcInner;

			std::optional<FullPolygonId> thisPoly;
			std::optional<FullPolygonId> leftPoly;
			std::optional<FullPolygonId> abovePoly;
		};

		struct InProgressPolygon {
			std::shared_ptr<Arc> firstArc; //important because the first arc encoutnered will always be part of the outer ring
			std::unordered_set<std::shared_ptr<Arc>> allArcs;
		};
		std::unordered_map<T, std::unordered_map<int, InProgressPolygon>> allPolygons;
		auto addArcToPoly = [&allPolygons](std::shared_ptr<Arc> arc, FullPolygonId id) {
			//the behavior of operator[] to default-construct the value if necessary is desirable here
			InProgressPolygon& thisPoly = allPolygons[id.multiPolygonId][id.polygonId];
			if (thisPoly.firstArc == nullptr) {
				thisPoly.firstArc = arc;
			}
			thisPoly.allArcs.insert(arc);
		};



		Raster<FullPolygonId> intermediateInfo{ (Alignment)r };

		//first pass gets all the cells to have the correct polygon id, using union find
		std::unordered_map<T, int> nextID; //records how many IDs have been used for the raster value so far, so when a new ID is needed, uniqueness can be assured
		std::unordered_map<int, int> aliases; //in this object, the key needs to be replaced by the value in the final raster
		for (rowcol_t row = 0; row < intermediateInfo.nrow(); ++row) {
			for (rowcol_t col = 0; col < intermediateInfo.ncol(); ++col) {
				auto v = r.atRCUnsafe(row, col);
				auto intermediateInfoCell = intermediateInfo.atRCUnsafe(row, col);
				if (!v.has_value()) {
					intermediateInfoCell.has_value() = false;
					continue;
				}
				intermediateInfoCell.has_value() = true;
				intermediateInfoCell.value().multiPolygonId = v.value();
				if (!nextID.contains(v.value())) {
					nextID.emplace(v.value(), 0);
				}
				int leftId = -1;
				int aboveId = -1;
				if (row > 0) {
					auto vAbove = r.atRCUnsafe(row - 1, col);
					if (vAbove.has_value()) {
						aboveId = vAbove.value();
					}
				}
				if (col > 0) {
					auto vLeft = r.atRCUnsafe(row, col - 1);
					if (vLeft.has_value()) {
						leftId = vLeft.value();
					}
				}
				if (leftId == -1 && aboveId == -1) {
					intermediateInfoCell.value().polygonId = nextID.at(v.value());
					nextID.at(v.value())++;
				}
				else if (leftId != -1 && aboveId == -1) {
					intermediateInfoCell.value().polygonId = leftId;
				}
				else if (leftId == -1 && aboveId != -1) {
					intermediateInfoCell.value().polygonId = aboveId;
				}
				else {
					intermediateInfoCell.value().polygonId = leftId;
					int id = aboveId;
					std::vector<int> toRedirect;
					toRedirect.push_back(id);
					while (aliases.contains(id)) {
						id = aliases.at(id);
						toRedirect.push_back(id);
					}
					for (int n : toRedirect) {
						aliases.emplace(n, leftId);
					}
				}
			}
		}

		//re-label everything based on aliases to ensure there's a single id per polygon
		for (rowcol_t row = 0; row < intermediateInfo.nrow(); ++row) {
			for (rowcol_t col = 0; col < intermediateInfo.ncol(); ++col) {
				auto v = intermediateInfo.atRCUnsafe(row, col);
				if (!v.has_value()) {
					continue;
				}
				int id = v.value().polygonId;
				while (aliases.contains(id)) {
					id = aliases.at(id);
				}
				v.value().polygonId = id;
			}
		}

		//finally, do some edge tracing
		//with this algorithm, edges are identified to the left and above the current cell, so we need to go to the "virtual" cells below and to the right of the real data
		std::vector<TwoArms> prevRow = std::vector<TwoArms>(intermediateInfo.ncol() + 1);
		for (rowcol_t row = 0; row < intermediateInfo.nrow() + 1; ++row) {
			std::vector<TwoArms> thisRow = std::vector<TwoArms>(intermediateInfo.ncol() + 1);

			for (rowcol_t col = 0; col < thisRow.size(); ++col) {
				TwoArms& thisArms = thisRow[col];
				if (col < intermediateInfo.ncol() && row < intermediateInfo.nrow()) {
					if (intermediateInfo.atRCUnsafe(row, col).has_value()) {
						thisArms.thisPoly = intermediateInfo.atRCUnsafe(row, col).value();
					}
				}

				bool leftHorizontalSolid = false;
				bool aboveVerticalSolid = false;
				TwoArms* leftArms = nullptr;
				TwoArms* aboveArms = nullptr;
				if (col > 0) {
					leftArms = &thisRow[col - 1];
					thisArms.leftPoly = leftArms->thisPoly;
					thisArms.verticalIsSolid = (thisArms.leftPoly == thisArms.thisPoly);
					leftHorizontalSolid = leftArms->horizontalIsSolid;
				}
				else {
					thisArms.verticalIsSolid = true;
				}
				if (row > 0) {
					aboveArms = &prevRow[col];
					thisArms.abovePoly = aboveArms->thisPoly;
					thisArms.horizontalIsSolid = (thisArms.abovePoly == thisArms.thisPoly);
					aboveVerticalSolid = aboveArms->verticalIsSolid;
				}
				else {
					thisArms.horizontalIsSolid = true;
				}

				constexpr uint8_t THIS_VERT_SOLID = 1 << 0;
				constexpr uint8_t THIS_HORIZ_SOLID = 1 << 1;
				constexpr uint8_t UPPER_VERT_SOLID = 1 << 2;
				constexpr uint8_t LEFT_HORIZ_SOLID = 1 << 3;
				uint8_t thisCase =
					(uint8_t)(thisArms.verticalIsSolid) << 0
					| (uint8_t)(thisArms.horizontalIsSolid) << 1
					| (uint8_t)(aboveVerticalSolid) << 2
					| (uint8_t)(leftHorizontalSolid) << 3;

				Vertex currentVertex = Vertex(row, col);

				//these are helper functions for common tasks in the below cases
				auto closeUpperLeftPolyInner = [&]() {
					aboveArms->verticalArcOuter->vertices.push_back(currentVertex);
					leftArms->horizontalArcOuter->vertices.push_back(currentVertex);
					connectArcs(aboveArms->verticalArcOuter, leftArms->horizontalArcOuter);
				};
				auto upperVertContinueCornerInner = [&]() {
					aboveArms->verticalArcInner->vertices.push_back(currentVertex);
					thisArms.horizontalArcOuter = aboveArms->verticalArcInner;
				};
				auto leftHorizContinueCornerInner = [&]() {
					leftArms->horizontalArcInner->vertices.push_back(currentVertex);
					thisArms.verticalArcOuter = leftArms->horizontalArcInner;
				};
				auto makeNewArcsForInnerPoly = [&]() {
					if (!thisArms.thisPoly.has_value()) {
						return;
					}
					thisArms.verticalArcInner = std::make_shared<Arc>(currentVertex, COUNTERCLOCKWISE);
					thisArms.horizontalArcInner = std::make_shared<Arc>(currentVertex, CLOCKWISE);
					connectArcs(thisArms.verticalArcInner, thisArms.horizontalArcInner);
					addArcToPoly(thisArms.verticalArcInner, *thisArms.thisPoly);
					addArcToPoly(thisArms.horizontalArcInner, *thisArms.thisPoly);
				};
				auto continueOuterHoriz = [&]() {
					thisArms.horizontalArcOuter = leftArms->horizontalArcOuter;
				};
				auto continueOuterVert = [&]() {
					thisArms.verticalArcOuter = aboveArms->verticalArcOuter;
				};
				auto makeNewArcsForOuterPoly = [&]() {
					if (!thisArms.leftPoly.has_value()) {
						return;
					}
					assert(thisArms.upperPoly.has_value()); //in cases where this is called, the upper and left poly should be the same
					assert(thisArms.upperPoly.value() == thisArms.leftPoly.value());
					thisArms.verticalArcOuter = std::make_shared<Arc>(currentVertex, COUNTERCLOCKWISE);
					thisArms.horizontalArcOuter = std::make_shared<Arc>(currentVertex, CLOCKWISE);
					connectArcs(thisArms.verticalArcOuter, thisArms.horizontalArcOuter);
					addArcToPoly(thisArms.verticalArcOuter, *thisArms.leftPoly);
					addArcToPoly(thisArms.horizontalArcOuter, *thisArms.leftPoly);
				};
				auto continueInnerVert = [&]() {
					thisArms.verticalArcInner = aboveArms->verticalArcInner;
				};
				auto leftHorizContinueCornerOuter = [&]() {
					leftArms->horizontalArcOuter->vertices.push_back(currentVertex);
					thisArms.verticalArcInner = leftArms->horizontalArcOuter;
				};
				auto continueInnerHoriz = [&]() {
					thisArms.horizontalArcInner = leftArms->horizontalArcInner;
				};
				auto upperVertContinueCornerOuter = [&]() {
					aboveArms->verticalArcOuter->vertices.push_back(currentVertex);
					thisArms.horizontalArcInner = aboveArms->verticalArcOuter;
				};
				auto closeUpperLeftPolyOuter = [&]() {
					aboveArms->verticalArcInner->vertices.push_back(currentVertex);
					leftArms->horizontalArcInner->vertices.push_back(currentVertex);
					connectArcs(aboveArms->verticalArcInner, leftArms->horizontalArcInner);
				};

				//these cases are pulled from the paper cited above
				//when the upper vertical is solid, aboveArms should never be nullptr
				//when the left horizontal is solid, leftArms should never be nullptr
				switch (thisCase) {
				case (THIS_VERT_SOLID | THIS_HORIZ_SOLID | UPPER_VERT_SOLID | LEFT_HORIZ_SOLID): //case a
					closeUpperLeftPolyInner();
					upperVertContinueCornerInner();
					leftHorizContinueCornerInner();
					makeNewArcsForInnerPoly();
					break;
				case (THIS_VERT_SOLID | THIS_HORIZ_SOLID | LEFT_HORIZ_SOLID): //case b
					continueOuterHoriz();
					leftHorizContinueCornerInner();
					makeNewArcsForInnerPoly();
					break;
				case (THIS_VERT_SOLID | THIS_HORIZ_SOLID | UPPER_VERT_SOLID): //case c
					continueOuterVert();
					upperVertContinueCornerInner();
					makeNewArcsForInnerPoly();
					break;
				case (THIS_VERT_SOLID | THIS_HORIZ_SOLID): //case d
					makeNewArcsForInnerPoly();
					makeNewArcsForOuterPoly();
					break;
				case (THIS_VERT_SOLID | UPPER_VERT_SOLID | LEFT_HORIZ_SOLID): //case e
					closeUpperLeftPolyInner();
					leftHorizContinueCornerInner();
					continueInnerVert();
					break;
				case (THIS_VERT_SOLID | LEFT_HORIZ_SOLID): //case f
					leftHorizContinueCornerInner();
					leftHorizContinueCornerOuter();
					break;
				case (THIS_VERT_SOLID | UPPER_VERT_SOLID): //case g
					continueOuterVert();
					continueInnerVert();
					break;
				case (THIS_VERT_SOLID): //case h
					assert(false);
					throw std::runtime_error("impossible case in polygonization");
					break;
				case (THIS_HORIZ_SOLID | UPPER_VERT_SOLID | LEFT_HORIZ_SOLID): //case i
					closeUpperLeftPolyInner();
					continueInnerHoriz();
					upperVertContinueCornerInner();
					break;
				case (THIS_HORIZ_SOLID | LEFT_HORIZ_SOLID): //case j
					continueInnerHoriz();
					continueOuterHoriz();
					break;
				case (THIS_HORIZ_SOLID | UPPER_VERT_SOLID): //case k
					upperVertContinueCornerInner();
					upperVertContinueCornerOuter();
				case (THIS_HORIZ_SOLID): //case l
					assert(false);
					throw std::runtime_error("impossible case in polygonization");
					break;
				case (UPPER_VERT_SOLID | LEFT_HORIZ_SOLID): //case m
					closeUpperLeftPolyInner();
					closeUpperLeftPolyOuter();
					break;
				case (LEFT_HORIZ_SOLID): //case n
					assert(false);
					throw std::runtime_error("impossible case in polygonization");
					break;
				case (UPPER_VERT_SOLID): //case o
					assert(false);
					throw std::runtime_error("impossible case in polygonization");
					break;
				case (0): //case p
					//no action necessary
					break;
				}




			prevRow = std::move(thisRow);
		}
	}
}