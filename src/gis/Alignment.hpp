#pragma once
#ifndef lapis_alignment_h
#define lapis_alignment_h

#include"gis_pch.hpp"
#include"GisExceptions.hpp"
#include"extent.hpp"
#include"QuadExtent.hpp"


namespace lapis {

	enum class SnapType {
		near, in, out, ll
	};

	class CellExtentIterator;

	class Alignment : public Extent {

	public:

		struct RowColExtent {
			rowcol_t minrow, maxrow, mincol, maxcol;
			RowColExtent(const rowcol_t minrow, const rowcol_t maxrow, const rowcol_t mincol, const rowcol_t maxcol) :
				minrow(minrow), maxrow(maxrow), mincol(mincol), maxcol(maxcol) {}
			RowColExtent() = default;
		};




		//default construct. Extent is a single point at the origin; xres and yres are 1
		Alignment() : Extent(), _xres(1), _yres(1), _nrow(0), _ncol(0) {}
		//Constructor from lower left corner and cell information
		Alignment(const coord_t xmin, const coord_t ymin, const rowcol_t nrow, const rowcol_t ncol, const coord_t xres, const coord_t yres, const CoordRef& crs = CoordRef()) :
			Extent(xmin, xmin + xres * ncol, ymin, ymin + yres * nrow, crs), _nrow(nrow), _ncol(ncol), _xres(xres), _yres(yres) {
			checkValidAlignment();
		}
		//Constructor which divides the given extent into the desired number of rows and columns
		Alignment(const Extent& e, const rowcol_t nrow, const rowcol_t ncol) :
			Extent(e), _nrow(nrow), _ncol(ncol), _xres((e.xmax() - e.xmin()) / ncol), _yres((e.ymax() - e.ymin()) / nrow) {
			checkValidAlignment();
		}
		//Constructor which forces an origin and deduces nrow and ncol from an extent and resolution
		//The resulting alignment will be at least as large as the input extent
		Alignment(const Extent& e, const coord_t xorigin, const coord_t yorigin, const coord_t xres, const coord_t yres);

		//Constructor from a raster file
		Alignment(const std::string& filename);

		Alignment(const Alignment&) = default;

		//just virtual declaring the destructor
		virtual ~Alignment() noexcept = default;

		//access functions for nrow and ncol
		rowcol_t nrow() const {
			return _nrow;
		}
		rowcol_t ncol() const {
			return _ncol;
		}

		//access functions for xres and yres
		coord_t xres() const {
			return _xres;
		}
		coord_t yres() const {
			return _yres;
		}

		cell_t ncell() const {
			return (cell_t)_nrow * _ncol;
		}

		//These functions return the x and y coordinates of the cell corner that is closest to 0 without being negative,
		//if the alignment were to be extended all the way to the origin
		coord_t xOrigin() const {
			coord_t out = std::fmod(xmin(), xres());
			if (out < 0) {
				out += xres();
			}
			return out;
		}
		coord_t yOrigin() const {
			coord_t out = std::fmod(ymin(), yres());
			if (out < 0) {
				out += yres();
			}
			return out;
		}

		//A number of functions for converting between x/y, row/col, and cell numbers
		cell_t cellFromRowCol(const rowcol_t row, const rowcol_t col) const {
			_checkRow(row);
			_checkCol(col);
			return cellFromRowColUnsafe(row, col);
		}
		cell_t cellFromXY(const coord_t x, const coord_t y) const {
			_checkX(x);
			_checkY(y);
			return cellFromRowColUnsafe(rowFromYUnsafe(y), colFromXUnsafe(x));
		}
		rowcol_t rowFromCell(const cell_t cell) const {
			_checkCell(cell);
			return rowFromCellUnsafe(cell);
		}
		rowcol_t rowFromY(const coord_t y) const {
			_checkY(y);
			return rowFromYUnsafe(y);
		}
		rowcol_t colFromCell(const cell_t cell) const {
			_checkCell(cell);
			return colFromCellUnsafe(cell);
		}
		rowcol_t colFromX(const coord_t x) const {
			_checkX(x);
			return colFromXUnsafe(x);
		}
		coord_t xFromCell(const cell_t cell) const {
			_checkCell(cell);
			return xFromColUnsafe(colFromCellUnsafe(cell));
		}
		coord_t xFromCol(const rowcol_t col) const {
			_checkCol(col);
			return xFromColUnsafe(col);
		}
		coord_t yFromCell(const cell_t cell) const {
			_checkCell(cell);
			return yFromRowUnsafe(rowFromCellUnsafe(cell));
		}
		coord_t yFromRow(const rowcol_t row) const {
			_checkRow(row);
			return yFromRowUnsafe(row);
		}

		//these versions of the functions don't bother checking if the input data is valid
		cell_t cellFromRowColUnsafe(const rowcol_t row, const rowcol_t col) const {
#ifndef NDEBUG
			_checkRow(row);
			_checkCol(col);
#endif
			cell_t cell = col + (cell_t)(_ncol)*row;
			return cell;
		}
		rowcol_t rowFromYUnsafe(const coord_t y) const {
#ifndef NDEBUG
			_checkY(y);
#endif
			rowcol_t out = (rowcol_t)((_ymax - y) / _yres);
			if (y == _ymin) {
				return _nrow - 1;
			}
			return out;
		}
		rowcol_t colFromXUnsafe(const coord_t x) const {
#ifndef NDEBUG
			_checkX(x);
#endif
			rowcol_t out = (rowcol_t)((x - _xmin) / _xres);
			if (x == _xmax) {
				return _ncol - 1;
			}
			return out;
		}
		rowcol_t rowFromCellUnsafe(const cell_t cell) const {
#ifndef NDEBUG
			_checkCell(cell);
#endif
			return (rowcol_t)(cell / _ncol);
		}
		rowcol_t colFromCellUnsafe(const cell_t cell) const {
#ifndef NDEBUG
			_checkCell(cell);
#endif
			return cell % _ncol;
		}
		coord_t xFromColUnsafe(const rowcol_t col) const {
#ifndef NDEBUG
			_checkCol(col);
#endif
			return _xmin + _xres * col + (_xres / 2);
		}
		coord_t yFromRowUnsafe(const rowcol_t row) const {
#ifndef NDEBUG
			_checkRow(row);
#endif
			return _ymax - _yres * row - (_yres / 2);
		}
		cell_t cellFromXYUnsafe(const coord_t x, const coord_t y) const {
#ifndef NDEBUG
			_checkX(x);
			_checkY(y);
#endif
			return cellFromRowColUnsafe(rowFromYUnsafe(y), colFromXUnsafe(x));
		}
		coord_t xFromCellUnsafe(const cell_t cell) const {
#ifndef NDEBUG
			_checkCell(cell);
#endif
			return xFromColUnsafe(colFromCellUnsafe(cell));
		}
		coord_t yFromCellUnsafe(const cell_t cell) const {
#ifndef NDEBUG
			_checkCell(cell);
#endif
			return yFromRowUnsafe(rowFromCellUnsafe(cell));
		}

		//Returns an extent which is as close as possible to the given extent, while having its bounds align with the cell borders of the alignment
		//If snap is near, all sides will round to the nearest
		//If snap is out, xmin and ymin will round down, and xmax and ymax will round up
		//If snap is in, xmin and ymin will round up, and xmax and ymax will round down
		//If snap is ll, all four values will round down
		Extent alignExtent(const Extent& e, const SnapType snap) const;

		//Returns a list of cells which fall inside the given extent, after aligning with the given snaptype
		std::vector<cell_t> cellsFromExtent(const Extent& e, const SnapType snap) const;

		//Returns the extent of the cell with the given index
		Extent extentFromCell(const cell_t cell) const {
			coord_t x = xFromCell(cell);
			coord_t y = yFromCell(cell);
			return Extent(x - xres() / 2, x + xres() / 2, y - yres() / 2, y + yres() / 2, crs());
		}

		//Checks whether two alignments are "the same", in a slightly more permissive way than operator==.
		//In particular, operator== requires them to have the same CRS. This function only requires them to have consistent CRS (so, one may be empty).
		bool isSameAlignment(const Alignment& a) const;

		//Checks whether two alignments are consistent; that is, have the same cellsize, origin, and a consistent crs
		bool consistentAlignment(const Alignment& a) const;

		//Returns the minimum and maximum row and column numbers that fall within the given extent, after aligning with the given snaptype
		RowColExtent rowColExtent(const Extent& e, SnapType snap) const;

		//Returns an alignment in the given CRS which is as close as reasonable possible to this
		Alignment transformAlignment(const CoordRef& crs) const;

	protected:
		coord_t _xres, _yres;
		rowcol_t _nrow, _ncol;

		void checkValidAlignment();
		void alignmentInitFromGDALRaster(GDALDatasetWrapper& wgd, const std::array<double, 6>& geotrans);

		//These functions just throw if the given values are outside the bounds of the alignment
		void _checkX(const coord_t x) const {
			if (x<xmin() || x>xmax()) {
				throw OutsideExtentException("X outside extent");
			}
		}
		void _checkY(const coord_t y) const {
			if (y<ymin() || y>ymax()) {
				throw OutsideExtentException("Y outside extent");
			}
		}
		void _checkRow(const rowcol_t row) const {
			if (row < 0 || row >= nrow()) {
				throw OutsideExtentException("Row outside extent");
			}
		}
		void _checkCol(const rowcol_t col) const {
			if (col < 0 || col >= ncol()) {
				throw OutsideExtentException("Col outside extent");
			}
		}
		void _checkCell(const cell_t cell) const {
			if (cell < 0 || cell >= ncell()) {
				throw OutsideExtentException("Cell outside extent");
			}
		}

	private:

		static coord_t snapExtent(coord_t xy, coord_t res, coord_t origin, coord_t(*snapFunc)(coord_t)) {
			return snapFunc((xy - origin) / res) * res + origin;
		}
	};

	class CellIterator {
	public:
		class iterator {
		public:
			iterator() : parent(nullptr), isEnd(true), row(-1), col(-1), rc() {}
			iterator(const CellIterator& parent) : parent(&parent), isEnd(false) {
				rc = parent._a.rowColExtent(parent._e, SnapType::near);
				row = rc.minrow;
				col = rc.mincol;
			}
			bool operator!=(const iterator& other) {
				if (other.isEnd) {
					return !isEnd;
				}
				if (isEnd) {
					return !other.isEnd;
				}
				return row != other.row || col != other.col;
			}
			iterator& operator++() {
				++col;
				if (col > rc.maxcol) {
					col = rc.mincol;
					++row;
				}
				if (row > rc.maxrow) {
					isEnd = true;
				}
				return *this;
			}
			cell_t operator*() {
				return parent->_a.cellFromRowColUnsafe(row, col);
			}

		private:
			Alignment::RowColExtent rc;
			const CellIterator* parent;
			rowcol_t row, col;
			bool isEnd;
		};

		CellIterator(Alignment a, Extent e, SnapType snap) : _a(a) {
			Extent snapE = a.alignExtent(e, snap);
			if (snapE.overlaps(a)) {
				_e = cropExtent(snapE, a);
				hasAnyCells = true;
			}
			else {
				hasAnyCells = false;
				_e = Extent();
			}
		}
		CellIterator(Alignment a) : CellIterator(a, a, SnapType::near) {}
		iterator begin() {
			if (!hasAnyCells) {
				return end();
			}
			if (_e.xmin() == _e.xmax() || _e.ymin() == _e.ymax()) {
				return end();
			}
			return iterator(*this);
		}
		iterator end() {
			return iterator();
		}

	private:
		bool hasAnyCells = true;
		Alignment _a;
		Extent _e;
	};

	inline std::ostream& operator<<(std::ostream& os, const Alignment& a) {
		os << "ALIGNMENT: xres: " << a.xres() << " yres: " << a.yres() << " ncol: " << a.ncol() << " nrow: " << a.nrow() << '\n';
		os << (Extent)a;
		return os;
	}

	//returns an alignment with the same origin and resolution as the given alignment, and an extent which is
	//the same as an extent-crop of a and e, after using alignExtent on e
	Alignment cropAlignment(const Alignment& a, const Extent& e, SnapType snap);

	Alignment extendAlignment(const Alignment& a, const Extent& e, SnapType snap);

	bool operator==(const Alignment& lhs, const Alignment& rhs);
	bool operator!=(const Alignment& lhs, const Alignment& rhs);
}

#endif