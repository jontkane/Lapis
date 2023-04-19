#pragma once
#ifndef lapis_coordinate_h
#define lapis_coordinate_h

#include"coordtransform.hpp"

//This header defines classes for coordinates that know their own CRS

namespace lapis {

	//A class suitable for holding a large number of points that know their CRS, without duplicating overhead
	//T should be a class with x and y as public coord_t members.
	template<class T>
	class CoordVector2D {
	public:

		using value_t = T;

		CoordRef crs; //keeping this public because there's no guarantees about it; making it private would just result in a boilerplate getter and setter

		CoordVector2D() = default;
		CoordVector2D(const CoordRef& crs) : crs(crs) {}
		CoordVector2D(const size_t n);
		CoordVector2D(const size_t n, const CoordRef& crs);

		virtual ~CoordVector2D() = default;

		void reserve(const size_t n);
		void resize(const size_t n);
		void clear();
		size_t size() const;
		void shrink_to_fit();

		T* data();

		//transforms all the points in this object to be in the given CRS
		//this is done *in place*. If you don't have ownership of the object, you should use the oop equivalent defined below
		void transform(const CoordRef& newcrs);

		void push_back(const T& t);
		template<class... Args>
		void emplace_back(Args&&... args) {
			_points.emplace_back(args...);
		}

		//Copies the contents of the given CoordVector into the end of this one, applying the correct coordinate transformation
		//if this CoordVector is an XY vector, only x and y will be transformed
		//if this is an XYZ vector, x, y and z will all be transformed
		//In either case, all untransformed data will be copied unchanged
		void addAll(const CoordVector2D<T>& other);
		auto begin();
		auto end();
		const auto begin() const;
		const auto end() const;

		T& operator[](size_t n);
		const T& operator[](size_t n) const;
		T& at(size_t n);
		const T& at(size_t n) const;

	protected:
		std::vector<T> _points;

		virtual void _transform(CoordTransform& tr, size_t startIdx);
	};

	template<class T>
	class CoordVector3D : public CoordVector2D<T> {
	public:
		CoordVector3D() = default;
		CoordVector3D(const CoordRef & crs) : CoordVector2D<T>(crs) {}
		CoordVector3D(const size_t n) : CoordVector2D<T>(n) {}
		CoordVector3D(const size_t n, const CoordRef & crs) : CoordVector2D<T>(n,crs) {}

		virtual ~CoordVector3D() = default;

		void setZ(coord_t z, size_t idx, const LinearUnit& zUnits) {
			this->_points[idx].z = convertUnits(z, zUnits, this->crs.getZUnits()); //why was VS giving me a red squiggly without the this->?
		}

	protected:
		void _transform(CoordTransform& tr, size_t startIdx) override;
	};

	using CoordXYVector = CoordVector2D<CoordXY>;
	using CoordXYZVector = CoordVector3D<CoordXYZ>;

	//Does an out-of-place x/y-only transformation of the given coordxyvector. The input vector is not changed, unlike the class member version.
	//The other members of T (z, intensity, etc) will be copied unchanged
	template<class T>
	CoordVector2D<T> transform(const CoordVector2D<T>& in, const CoordRef& crs) {
		CoordVector2D<T> out = in;
		out.transform(crs);
		return out;
	}

	template<class T>
	CoordVector3D<T> transform(const CoordVector3D<T>& in, const CoordRef& crs) {
		CoordVector3D<T> out = in;
		out.transform(crs);
		return out;
	}

	template<class T>
	CoordVector2D<T>::CoordVector2D(const size_t n) : crs() {
		_points = std::vector<T>(n);
	}

	template<class T>
	CoordVector2D<T>::CoordVector2D(const size_t n, const CoordRef& crs) : crs(crs) {
		_points = std::vector<T>(n);
	}

	template<class T>
	void CoordVector2D<T>::reserve(const size_t n) {
		_points.reserve(n);
	}

	template<class T>
	inline void CoordVector2D<T>::resize(const size_t n)
	{
		_points.resize(n);
	}

	template<class T>
	void CoordVector2D<T>::clear() {
		_points.clear();
	}

	template<class T>
	size_t CoordVector2D<T>::size() const {
		return _points.size();
	}
	
	template<class T>
	void CoordVector2D<T>::transform(const CoordRef& newcrs) {
		if (crs.isEmpty()) {
			crs = newcrs;
			return;
		}
		if (newcrs.isConsistent(crs)) {
			return;
		}
		auto trans = CoordTransform(crs, newcrs);
		_transform(trans, 0);
		crs = newcrs;
	}

	template<class T>
	inline void CoordVector2D<T>::push_back(const T& t) {
		_points.push_back(t);
	}

	template<class T>
	void CoordVector2D<T>::addAll(const CoordVector2D<T>& other) {
		if (other.size() == 0) {
			return;
		}
		_points.reserve(_points.size() + other.size());
		size_t presize = _points.size();
		_points.insert(_points.end(), other.begin(), other.end());
		if (!crs.isConsistent(other.crs)) {
			auto trans = CoordTransform(other.crs, crs);
			_transform(trans, presize);
		}
	}

	template<class T>
	auto CoordVector2D<T>::begin() {
		return _points.begin();
	}

	template<class T>
	auto CoordVector2D<T>::end() {
		return _points.end();
	}

	template<class T>
	const auto CoordVector2D<T>::begin() const {
		return _points.begin();
	}

	template<class T>
	const auto CoordVector2D<T>::end() const {
		return _points.end();
	}

	template<class T>
	inline T& CoordVector2D<T>::at(size_t n) {
		return _points.at(n);
	}

	template<class T>
	inline const T& CoordVector2D<T>::at(size_t n) const {
		return _points.at(n);
	}

	template<class T>
	void CoordVector2D<T>::_transform(CoordTransform& tr, size_t startIdx) {
		tr.transformXY<T>(_points, startIdx);
	}

	template<class T>
	inline T& CoordVector2D<T>::operator[](size_t n) {
		return _points[n];
	}

	template<class T>
	inline const T& CoordVector2D<T>::operator[](size_t n) const {
		return _points[n];
	}

	template<class T>
	void CoordVector3D<T>::_transform(CoordTransform& tr, size_t startIdx) {
		tr.transformXYZ<T>(this->_points, startIdx); //will someone please tell me why I get a red squiggly if I don't put this-> here
	}

	template<class T>
	inline void CoordVector2D<T>::shrink_to_fit()
	{
		_points.shrink_to_fit();
	}

	template<class T>
	inline T* CoordVector2D<T>::data()
	{
		return _points.data();
	}

}


#endif