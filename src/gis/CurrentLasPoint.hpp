#pragma once
#ifndef lp_laspointwrapper_h
#define lp_laspointwrapper_h

#include"lasextent.hpp"
#include"..\LapisTypeDefs.hpp"


namespace lapis {
		// this class is a more sophisticated wrapper around a laszip_POINTER
		class CurrentLasPoint : public LasExtent {
		public:
#pragma warning(suppress : 26495)
			CurrentLasPoint() = default;
			CurrentLasPoint(const std::string& file);
			CurrentLasPoint(const CurrentLasPoint&) = delete;
			CurrentLasPoint(CurrentLasPoint&&) = default;
			CurrentLasPoint& operator=(const CurrentLasPoint&) = delete;
			CurrentLasPoint& operator=(CurrentLasPoint&&) = default;

			virtual ~CurrentLasPoint() = default;

			coord_t x() const {
				return _x;
			}
			coord_t y() const {
				return _y;
			}
			coord_t z() const {
				return _z;
			}
			std::uint16_t intensity() const {
				//intensity is before the first bits that differ between point10 and point14, so no check is needed
				return _point14()->intensity;
			}
			uint8_t returnNumber() const {
				if (_ispoint14) {
					return _point14()->returnNumber();
				}
				return _point10()->returnNumber();
			}
			std::uint8_t numberOfReturns() const {
				if (_ispoint14) {
					return _point14()->numberOfReturns();
				}
				return _point10()->numberOfReturns();
			}
			
			//scan direction flag
			//edge of flight line
			
			std::uint8_t classification() const {
				//in point formats 0 through 5, the first 5 bits control the classifiction
				//in formats 6+, it's its own field
				if (_ispoint14) {
					return _point14()->classification();
				}
				return _point10()->classification();
			}

			bool synthetic() const {
				//the first bit in point14, and the first bit after the class in point10
				if (_ispoint14) {
					return _point14()->synthetic();
				}
				return _point10()->synthetic();
			}

			bool keypoint() const {
				//the second bit in point14, and the second bit after the class in point10
				if (_ispoint14) {
					return _point14()->keypoint();
				}
				return _point10()->keypoint();
			}

			bool withheld() const {
				//the third bit in point14, and the third bit after the class in point10
				if (_ispoint14) {
					return _point14()->withheld();
				}
				return _point10()->withheld();
			}

			bool overlap() const {
				//the fourth bit in point14, not present in point10
				if (_ispoint14) {
					return _point14()->overlap();
				}
				return false;
			}

			double scanAngle() const {
				if (_ispoint14) {
					return _point14()->scanAngle();
				}
				return _point10()->scanAngle();
			}

			//user data
			//point source ID

			size_t nPoints() const;
			size_t nPointsRemaining() const;

		protected:
			size_t _currentPoint = 0;

			void advance() {
				_las.readPoint(_buffer.data());
				_updateXYZ();
				++_currentPoint;
			}

		private:
			LasIO _las;
			std::vector<char> _buffer;
			coord_t _x = 0, _y = 0, _z = 0;

			bool _ispoint14 = false;

			LasPoint14* _point14() const {
				return (LasPoint14*)_buffer.data();
			}
			LasPoint10* _point10() const {
				return (LasPoint10*)_buffer.data();
			}

			void _updateXYZ() {
				_x = (coord_t)(_point14()->x) * _las.header.ScaleFactor.x + _las.header.Offset.x;
				_y = (coord_t)(_point14()->y) * _las.header.ScaleFactor.y + _las.header.Offset.y;
				_z = (coord_t)(_point14()->z) * _las.header.ScaleFactor.z + _las.header.Offset.z;
			}
	};
}

#endif