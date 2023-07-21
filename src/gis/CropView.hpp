#pragma once
#ifndef lp_cropview_h
#define lp_cropview_h

#include"raster.hpp"

namespace lapis {

	//This class allows access to the data of a raster object with cell, row, and col values corresponding to a cropped alignment
	template<class T>
	class CropView : public Alignment {
	public:
		CropView() = delete; //there are no possible sensible defaults

		CropView(Raster<T>* parent, const Extent& e, const SnapType snap);

		auto operator[](cell_t cell) {
			return _parent->atRCUnsafe(rowFromCellUnsafe(cell) + _rowoffset, colFromCellUnsafe(cell) + _coloffset);
		}
		const auto operator[](cell_t cell) const {
			return _parent->atRCUnsafe(rowFromCellUnsafe(cell) + _rowoffset, colFromCellUnsafe(cell) + _coloffset);
		}

		auto atCellUnsafe(cell_t cell) {
#ifndef NDEBUG
			_checkCell(cell);
#endif
			return operator[](cell);
		}

		const auto atCellUnsafe(cell_t cell) const {
#ifndef NDEBUG
			_checkCell(cell);
#endif
			return operator[](cell);
		}

		auto atRCUnsafe(rowcol_t row, rowcol_t col) {
#ifndef NDEBUG
			_checkRow(row);
			_checkCol(col);
#endif
			return _parent->atRCUnsafe(row + _rowoffset, col + _coloffset);
		}
		const auto atRCUnsafe(rowcol_t row, rowcol_t col) const {
#ifndef NDEBUG
			_checkRow(row);
			_checkCol(col);
#endif
			return _parent->atRCUnsafe(row + _rowoffset, col + _coloffset);
		}
		auto atXYUnsafe(coord_t x, coord_t y) {
#ifndef NDEBUG
			_checkX(x);
			_checkY(y);
#endif
			return _parent->atXYUnsafe(x, y);
		}
		const auto atXYUnsafe(coord_t x, coord_t y) const {
#ifndef NDEBUG
			_checkX(x);
			_checkY(y);
#endif
			return _parent->atXYUnsafe(x, y);
		}

		auto atCell(cell_t cell) {
			return _parent->atXY(xFromCell(cell), yFromCell(cell));
		}
		const auto atCell(cell_t cell) const {
			return _parent->atXY(xFromCell(cell), yFromCell(cell));
		}
		auto atRC(rowcol_t row, rowcol_t col) {
			return _parent->atXY(xFromCol(col), yFromRow(row));
		}
		const auto atRC(rowcol_t row, rowcol_t col) const {
			return _parent->atXY(xFromCol(col), yFromRow(row));
		}
		auto atXY(coord_t x, coord_t y) {
			return _parent->atXY(x, y);
		}
		const auto atXY(coord_t x, coord_t y) const {
			return _parent->atXY(x, y);
		}

		bool hasAnyValue() const {
			for (cell_t cell = 0; cell < ncell(); ++cell) {
				if (atCellUnsafe(cell).has_value()) {
					return true;
				}
			}
			return false;
		}

		auto parentRasterHash() const {
			
			return std::hash<Raster<T>*>()(_parent);
		}

	private:
		Raster<T>* _parent;
		rowcol_t _rowoffset, _coloffset;
	};

	template<class T>
	CropView<T>::CropView(Raster<T>* parent, const Extent& e, const SnapType snap) :
		Alignment(cropAlignment(*parent, e, snap)), _parent(parent) {
		_rowoffset = parent->rowFromY(_ymax - (_yres / 2));
		_coloffset = parent->colFromX(_xmin + (_xres / 2));
	}
}

#endif