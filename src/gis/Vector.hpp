#pragma once
#include"gis_pch.hpp"
#include"CoordRef.hpp"
#include"Raster.hpp"

namespace lapis {

	class GisVector {
	public:
		virtual UniqueOGRGeometry asGdal() const = 0;
		CoordRef& crs();
		const CoordRef& crs() const;
	protected:
		CoordRef _crs;

		static constexpr bool isNativeLittleEndian();
		
		template<class T>
		static void _appendToWkb(std::vector<uint8_t>& wkb, T value);

		static void _firstBytesOfWkb(std::vector<uint8_t>& wkb, uint32_t wkbFormatNumber);
	};

	class Polygon : public GisVector {
		friend class MultiPoylgon;
	public:

		constexpr static OGRwkbGeometryType gdalGeometryType = wkbPolygon;
		UniqueOGRGeometry asGdal() const override;

		Polygon() = default;

		//in these functions, do *not* duplicate the first vertex
		//the outer ring should be listed in counterclockwise order, and inner rings in clockwise order
		Polygon(const std::list<CoordXY>& outerRing);
		void addInnerRing(const std::list<CoordXY>& innerRing);
	private:
		std::list<CoordXY> _outerRing;
		std::vector<std::list<CoordXY>> _innerRings;
		void _appendToWkbNoHeader(std::vector<uint8_t>& wkb) const;
		static void _appendRingToWkb(std::vector<uint8_t>& wkb, const std::list<CoordXY>& ring);
		static constexpr uint32_t _wkbFormatNumber = 3;
	};

	class MultiPolygon : public GisVector {
	public:
		constexpr static OGRwkbGeometryType gdalGeometryType = wkbMultiPolygon;
		UniqueOGRGeometry asGdal() const override;

		MultiPolygon() = default;
		void addPolygon(const Polygon& polygon);
	private:
		std::vector<Polygon> _polygons;
		static constexpr uint32_t _wkbFormatNumber = 6;
	};

	enum class FieldType {
		Integer,
		Real,
		String
	};

	class AttributeTable {
	public:
		void addStringField(const std::string& name, size_t width);
		void addIntegerField(const std::string& name);
		void addRealField(const std::string& name);
		template<class T>
		void addNumericField(const std::string& name);
		
		void resize(size_t nrow);
		void addRow();

		size_t nrow();

		std::vector<std::string> getAllFieldNames() const;
		FieldType getFieldType(const std::string& name) const;
		size_t getStringFieldWidth(const std::string& name) const;

		const std::string& getStringField(size_t index, const std::string& name) const;
		int64_t getIntegerField(size_t index, const std::string& name) const;
		double getRealField(size_t index, const std::string& name) const;
		template<class T>
		T getNumericField(size_t index, const std::string& name) const;

		void setStringField(size_t index, const std::string& name, const std::string& value);
		void setIntegerField(size_t index, const std::string& name, int64_t value);
		void setRealField(size_t index, const std::string& name, double value);
		template<class T>
		void setNumericField(size_t index, const std::string& name, T value);
	private:

		class FixedWidthString {
		public:
			const std::string& get() const;
			void set(const std::string& value, size_t width);
		private:
			std::string _data;
		};

		using Variant = std::variant<int64_t, double, FixedWidthString>;
		struct Field {
			FieldType type;
			size_t width; //only used if the type is String
			std::vector<Variant> values;
		};

		size_t _nrow;
		std::unordered_map<std::string, Field> _fields;
		std::vector<std::string> _fieldNamesInOrder;
	};

	template<class GEOMETRY>
	class SingleGeometryWithAttributes {
	public:

		SingleGeometryWithAttributes(std::shared_ptr<AttributeTable> fullAttributeTable, GEOMETRY& geometry, size_t attributeIndex);

		std::vector<std::string> getAllFieldNames() const;
		FieldType getFieldType(const std::string& name) const;
		size_t getStringFieldWidth(const std::string& name) const;

		const std::string& getStringField(const std::string& name) const;
		int64_t getIntegerField(const std::string& name) const;
		double getRealField(const std::string& name) const;
		template<class T>
		T getNumericField(const std::string& name) const;

		void setStringField(const std::string& name, const std::string& value);
		void setIntegerField(const std::string& name, int64_t value);
		void setRealField(const std::string& name, double value);
		template<class T>
		void setNumericField(const std::string& name, T value);

		const GEOMETRY& getGeometry() const;
	private:
		std::shared_ptr<AttributeTable> _fullAttributeTable;
		GEOMETRY& _geometry;
		size_t _attributeIndex;
	};

	template<class GEOMETRY>
	class VectorsAndAttributes {
	public:
		VectorsAndAttributes() = default;
		VectorsAndAttributes(const std::string& filename);

		void writeShapefile(const std::filesystem::path& filename);

		void addStringField(const std::string& name, size_t width);
		void addIntegerField(const std::string& name);
		void addRealField(const std::string& name);
		template<class T>
		void addNumericField(const std::string& name);

		std::vector<std::string> getAllFieldNames() const;
		FieldType getFieldType(const std::string& name) const;
		size_t getStringFieldWidth(const std::string& name) const;

		const std::string& getStringField(size_t index, const std::string& name) const;
		int64_t getIntegerField(size_t index, const std::string& name) const;
		double getRealField(size_t index, const std::string& name) const;
		template<class T>
		T getNumericField(size_t index, const std::string& name) const;

		void setStringField(size_t index, const std::string& name, const std::string& value);
		void setIntegerField(size_t index, const std::string& name, int64_t value);
		void setRealField(size_t index, const std::string& name, double value);
		template<class T>
		void setNumericField(size_t index, const std::string& name, T value);

		const GEOMETRY& getGeometry(size_t index) const;
		void addGeometry(const GEOMETRY& g);
		SingleGeometryWithAttributes<GEOMETRY> getFeature(size_t index);
		SingleGeometryWithAttributes<GEOMETRY> front();
		SingleGeometryWithAttributes<GEOMETRY> back();

		CoordRef& crs();
		const CoordRef& crs() const;

		class iterator {
		public:
			iterator(std::shared_ptr<AttributeTable> attributes, std::vector<GEOMETRY>::iterator geomIt, size_t attributeIndex);

			iterator& operator++();
			bool operator==(const iterator& other) const = default;
			SingleGeometryWithAttributes<GEOMETRY> operator*();
		private:
			std::shared_ptr<AttributeTable> _attributes;
			size_t _attributeIndex;
			std::vector<GEOMETRY>::iterator _geomIt;
		};
		iterator begin();
		iterator end();


	private:
		std::vector<GEOMETRY> _geometry;
		std::shared_ptr<AttributeTable> _attributes;
		CoordRef _crs;
	};

	template<class GEOMETRY>
	inline const GEOMETRY& VectorsAndAttributes<GEOMETRY>::getGeometry(size_t index) const
	{
		return _geometry[index];
	}

	template<class GEOMETRY>
	inline void VectorsAndAttributes<GEOMETRY>::addGeometry(const GEOMETRY& g)
	{
		_geometry.push_back(g);
		_attributes->addRow();
	}

	template<class GEOMETRY>
	inline SingleGeometryWithAttributes<GEOMETRY> VectorsAndAttributes<GEOMETRY>::getFeature(size_t index)
	{
		return SingleGeometryWithAttributes<GEOMETRY>(_attributes, _geometry.at(index), index);
	}

	template<class GEOMETRY>
	inline SingleGeometryWithAttributes<GEOMETRY> VectorsAndAttributes<GEOMETRY>::front()
	{
		return SingleGeometryWithAttributes<GEOMETRY>(_attributes, _geometry.front(), 0);
	}

	template<class GEOMETRY>
	inline SingleGeometryWithAttributes<GEOMETRY> VectorsAndAttributes<GEOMETRY>::back()
	{
		return SingleGeometryWithAttributes<GEOMETRY>(_attributes, _geometry.back(), _geometry.size() - 1);
	}

	template<class GEOMETRY>
	inline CoordRef& VectorsAndAttributes<GEOMETRY>::crs()
	{
		return _crs;
	}

	template<class GEOMETRY>
	inline const CoordRef& VectorsAndAttributes<GEOMETRY>::crs() const
	{
		return _crs;
	}

	template<class GEOMETRY>
	inline VectorsAndAttributes<GEOMETRY>::iterator VectorsAndAttributes<GEOMETRY>::begin()
	{
		return iterator(_attributes, _geometry.begin(), 0);
	}

	template<class GEOMETRY>
	inline VectorsAndAttributes<GEOMETRY>::iterator VectorsAndAttributes<GEOMETRY>::end()
	{
		return iterator(_attributes, _geometry.end(), _geometry.size());
	}

	template<class GEOMETRY>
	inline VectorsAndAttributes<GEOMETRY>::iterator::iterator(std::shared_ptr<AttributeTable> attributes, std::vector<GEOMETRY>::iterator geomIt, size_t attributeIndex) :
		_attributes(attributes), _geomIt(geomIt), _attributeIndex(attributeIndex) {}

	template<class GEOMETRY>
	inline VectorsAndAttributes<GEOMETRY>::iterator& VectorsAndAttributes<GEOMETRY>::iterator::operator++()
	{
		_attributeIndex++;
		_geomIt++;
		return *this;
	}

	template<class GEOMETRY>
	inline SingleGeometryWithAttributes<GEOMETRY> VectorsAndAttributes<GEOMETRY>::iterator::operator*()
	{
		return SingleGeometryWithAttributes(_attributes, *_geomIt, _attributeIndex);
	}

	inline const std::string& AttributeTable::FixedWidthString::get() const
	{
		return _data;
	}

	inline void AttributeTable::FixedWidthString::set(const std::string& value, size_t width)
	{
		if (value.size() > width) {
			_data = value.substr(0, width);
		}
		else {
			_data = value + std::string(width - value.size(), '\0');
		}
	}

	template<class GEOMETRY>
	inline const GEOMETRY& SingleGeometryWithAttributes<GEOMETRY>::getGeometry() const
	{
		return _geometry;
	}

	template<class T>
	inline void AttributeTable::addNumericField(const std::string& name)
	{
		if constexpr (std::is_integral<T>()) {
			addIntegerField(name);
		}
		else if constexpr (std::is_floating_point<T>()) {
			addRealField(name);
		}
		else {
			[] <bool flag = false>()
			{
				static_assert(flag, "incorrect type in addTemplateField");
			}();
		}
	}

	template<class T>
	inline T AttributeTable::getNumericField(size_t index, const std::string& name) const
	{
		if constexpr (std::is_integral<T>()) {
			return getIntegerField(index, name, value);
		}
		else if constexpr (std::is_floating_point<T>()) {
			return getRealField(index, name, value);
		}
		else {
			[] <bool flag = false>()
			{
				static_assert(flag, "incorrect type in getNumericField");
			}();
			return 0;
		}
	}

	template<class T>
	inline void AttributeTable::setNumericField(size_t index, const std::string& name, T value)
	{
		if constexpr (std::is_integral<T>()) {
			setIntegerField(index, name, value);
		}
		else if constexpr (std::is_floating_point<T>()) {
			setRealField(index, name, value);
		}
		else {
			[] <bool flag = false>()
			{
				static_assert(flag, "incorrect type in setNumericField");
			}();
		}
	}

	template<class GEOMETRY>
	inline void VectorsAndAttributes<GEOMETRY>::writeShapefile(const std::filesystem::path& filename)
	{
		gdalAllRegisterThreadSafe();
		UniqueGdalDataset outshp = gdalCreateWrapper("ESRI Shapefile", filename.string().c_str(), 0, 0, GDT_Unknown);
		OGRSpatialReference crs;
		crs.importFromWkt(_crs.getCleanEPSG().getCompleteWKT().c_str());

		OGRLayer* layer = outshp->CreateLayer("layer", &crs, GEOMETRY::gdalGeometryType, nullptr);

		for (const auto& fieldName : getAllFieldNames()) {
			switch(getFieldType(fieldName)) {
			case FieldType::String:
				OGRFieldDefn(fieldName.c_str(), OFTString);
				break;
			case FieldType::Real:
				OGRFieldDefn(fieldName.c_str(), OFTReal);
				break;
			case FieldType::Integer:
				OGRFieldDefn(fieldName.c_str(), OFTInteger64);
				break;
			}
		}
		for (SingleGeometryWithAttributes<GEOMETRY> feature : *this) {
			UniqueOGRFeature gdalFeature = createFeatureWrapper(layer);
			for (const auto& fieldName : getAllFieldNames()) {
				switch (getFieldType(fieldName)) {
				case FieldType::String:
					gdalFeature->SetField(fieldName.c_str(), feature.getStringField(fieldName).c_str());
					break;
				case FieldType::Real:
					gdalFeature->SetField(fieldName.c_str(), feature.getRealField(fieldName));
					break;
				case FieldType::Integer:
					gdalFeature->SetField(fieldName.c_str(), feature.getIntegerField(fieldName));
					break;
				}
			}
			auto geometry = feature.getGeometry().asGdal();
			gdalFeature->SetGeometry(geometry.get());
			layer->CreateFeature(gdalFeature.get());
		}
	}

	template<class GEOMETRY>
	inline void VectorsAndAttributes<GEOMETRY>::addStringField(const std::string& name, size_t width)
	{
		_attributes->addStringField(name, width);
	}

	template<class GEOMETRY>
	inline void VectorsAndAttributes<GEOMETRY>::addIntegerField(const std::string& name)
	{
		_attributes->addIntegerField(name);
	}

	template<class GEOMETRY>
	inline void VectorsAndAttributes<GEOMETRY>::addRealField(const std::string& name)
	{
		_attributes->addRealField(name);
	}

	template<class GEOMETRY>
	inline std::vector<std::string> VectorsAndAttributes<GEOMETRY>::getAllFieldNames() const
	{
		return _attributes->getAllFieldNames();
	}

	template<class GEOMETRY>
	inline FieldType VectorsAndAttributes<GEOMETRY>::getFieldType(const std::string& name) const
	{
		return _attributes->getFieldType(name);
	}

	template<class GEOMETRY>
	inline size_t VectorsAndAttributes<GEOMETRY>::getStringFieldWidth(const std::string& name) const
	{
		return _attributes->getStringFieldWidth(name);
	}

	template<class GEOMETRY>
	inline const std::string& VectorsAndAttributes<GEOMETRY>::getStringField(size_t index, const std::string& name) const
	{
		return _attributes->getStringField(index, name);
	}

	template<class GEOMETRY>
	inline int64_t VectorsAndAttributes<GEOMETRY>::getIntegerField(size_t index, const std::string& name) const
	{
		return _attributes->getIntegerField(index, name);
	}

	template<class GEOMETRY>
	inline double VectorsAndAttributes<GEOMETRY>::getRealField(size_t index, const std::string& name) const
	{
		return _attributes->getRealField(index, name);
	}

	template<class GEOMETRY>
	inline void VectorsAndAttributes<GEOMETRY>::setStringField(size_t index, const std::string& name, const std::string& value)
	{
		_attributes->setStringField(index, name, value);
	}

	template<class GEOMETRY>
	inline void VectorsAndAttributes<GEOMETRY>::setIntegerField(size_t index, const std::string& name, int64_t value)
	{
		_attributes->setIntegerField(index, name, value);
	}

	template<class GEOMETRY>
	inline void VectorsAndAttributes<GEOMETRY>::setRealField(size_t index, const std::string& name, double value)
	{
		_attributes->setRealField(index, name, value);
	}

	template<class GEOMETRY>
	template<class T>
	inline void VectorsAndAttributes<GEOMETRY>::addNumericField(const std::string& name)
	{
		_attributes->addNumericField<T>(name);
	}

	template<class GEOMETRY>
	template<class T>
	inline T VectorsAndAttributes<GEOMETRY>::getNumericField(size_t index, const std::string& name) const
	{
		return _attributes->getNumericField<T>(index, name);
	}

	template<class GEOMETRY>
	template<class T>
	inline void VectorsAndAttributes<GEOMETRY>::setNumericField(size_t index, const std::string& name, T value)
	{
		_attributes->setNumericField<T>(index, name, value);
	}

	template<class GEOMETRY>
	inline SingleGeometryWithAttributes<GEOMETRY>::SingleGeometryWithAttributes
	(std::shared_ptr<AttributeTable> fullAttributeTable, GEOMETRY& geometry, size_t attributeIndex) :
		_fullAttributeTable(fullAttributeTable), _geometry(geometry), _attributeIndex(attributeIndex)
	{
	}

	template<class GEOMETRY>
	inline std::vector<std::string> SingleGeometryWithAttributes<GEOMETRY>::getAllFieldNames() const
	{
		return _fullAttributeTable->getAllFieldNames();
	}

	template<class GEOMETRY>
	inline FieldType SingleGeometryWithAttributes<GEOMETRY>::getFieldType(const std::string& name) const
	{
		return _fullAttributeTable->getFieldType(name);
	}

	template<class GEOMETRY>
	inline size_t SingleGeometryWithAttributes<GEOMETRY>::getStringFieldWidth(const std::string& name) const
	{
		return _fullAttributeTable->getStringFieldWidth(name);
	}

	template<class GEOMETRY>
	inline const std::string& SingleGeometryWithAttributes<GEOMETRY>::getStringField(const std::string& name) const
	{
		return _fullAttributeTable->getStringField(_attributeIndex, name);
	}

	template<class GEOMETRY>
	inline int64_t SingleGeometryWithAttributes<GEOMETRY>::getIntegerField(const std::string& name) const
	{
		return _fullAttributeTable->getIntegerField(_attributeIndex, name);
	}

	template<class GEOMETRY>
	inline double SingleGeometryWithAttributes<GEOMETRY>::getRealField(const std::string& name) const
	{
		return _fullAttributeTable->getRealField(_attributeIndex, name);
	}

	template<class GEOMETRY>
	inline void SingleGeometryWithAttributes<GEOMETRY>::setStringField(const std::string& name, const std::string& value)
	{
		return _fullAttributeTable->setStringField(_attributeIndex, name, value);
	}

	template<class GEOMETRY>
	inline void SingleGeometryWithAttributes<GEOMETRY>::setIntegerField(const std::string& name, int64_t value)
	{
		_fullAttributeTable->setIntegerField(_attributeIndex, name, value);
	}

	template<class GEOMETRY>
	inline void SingleGeometryWithAttributes<GEOMETRY>::setRealField(const std::string& name, double value)
	{
		_fullAttributeTable->setRealField(_attributeIndex, name, value);
	}

	template<class GEOMETRY>
	template<class T>
	inline T SingleGeometryWithAttributes<GEOMETRY>::getNumericField(const std::string& name) const
	{
		return _fullAttributeTable->getNumericField<T>(_attributeIndex, name);
	}

	template<class GEOMETRY>
	template<class T>
	inline void SingleGeometryWithAttributes<GEOMETRY>::setNumericField(const std::string& name, T value)
	{
		_fullAttributeTable->setNumericField<T>(_attributeIndex, name, value);
	}

	template<class T>
	inline void GisVector::_appendToWkb(std::vector<uint8_t>& wkb, T value)
	{
		uint8_t* bytes = reinterpret_cast<uint8_t*>(&value);
		wkb.insert(wkb.end(), bytes, bytes + sizeof(T));
	}

}