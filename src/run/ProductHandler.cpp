#include"run_pch.hpp"
#include"ProductHandler.hpp"
#include"PointMetricCalculator.hpp"

namespace lapis {

	ProductHandler::ProductHandler(ParamGetter* p) : _sharedGetter(p)
	{
		if (p == nullptr) {
			throw std::invalid_argument("ParamGetter is null");
		}
	}
	std::filesystem::path ProductHandler::parentDir() const
	{
		return _sharedGetter->outFolder();
	}
	std::filesystem::path ProductHandler::tempDir() const
	{
		return parentDir() / "Temp";
	}

	void ProductHandler::deleteTempDirIfEmpty() const
	{
		namespace fs = std::filesystem;
		if (fs::exists(tempDir()) && fs::is_empty(tempDir())) {
			fs::remove(tempDir());
		}
	}
	void ProductHandler::tryRemove(std::filesystem::path p)
	{
		try {
			std::filesystem::remove_all(p);
		}
		catch (...) {
			LapisLogger::getLogger().logWarning("Unable to delete " + p.string());
		}
	}
	std::filesystem::path ProductHandler::getFullFilename(const std::filesystem::path& dir,
		const std::string& baseName, OutputUnitLabel u, const std::string& extension) const
	{
		std::string unitString;
		using oul = OutputUnitLabel;
		if (u == oul::Default) {
			LinearUnit outUnit = _sharedGetter->outUnits();
			std::regex meterregex{ ".*met.*",std::regex::icase };
			std::regex footregex{ ".*f(o|e)(o|e)t.*",std::regex::icase };
			if (std::regex_match(outUnit.name(), meterregex)) {
				unitString = "_Meters";
			}
			else if (std::regex_match(outUnit.name(), footregex)) {
				unitString = "_Feet";
			}
		}
		else if (u == oul::Percent) {
			unitString = "_Percent";
		}
		else if (u == oul::Radian) {
			unitString = "_Radians";
		}
		else if (u == oul::Degree) {
			unitString = "_Degrees";
		}
		std::string runName = _sharedGetter->name().size() ? _sharedGetter->name() + "_" : "";
		std::filesystem::path fullPath = dir / (runName + baseName + unitString + "." + extension);
		return fullPath;
	}
	std::filesystem::path ProductHandler::getFullTempFilename(const std::filesystem::path& dir,
		const std::string& baseName, OutputUnitLabel u, size_t index, const std::string& extension) const
	{
		return getFullFilename(dir, baseName + "_" + std::to_string(index), u, extension);
	}
	std::filesystem::path ProductHandler::getFullTileFilename(const std::filesystem::path& dir,
		const std::string& baseName, OutputUnitLabel u, cell_t tile, const std::string& extension) const
	{

		return getFullFilename(dir, baseName + "_" + _sharedGetter->layoutTileName(tile), u, extension);
	}

	HandlerRegistrar& HandlerRegistrar::get()
	{
		static HandlerRegistrar instance;
		return instance;
    }
	size_t HandlerRegistrar::registerHandlerFactory(std::function<ProductHandler*(void)> factory)
	{
		_factories.push_back(factory);
        return _factories.size() - 1;
	}
	size_t HandlerRegistrar::size() const
	{
		return _handlers.size();
	}
	void HandlerRegistrar::initHandlers()
	{
		_handlers.clear();
		for (auto& factory : _factories) {
			_handlers.push_back(factory());
        }
	}
	std::vector<ProductHandler*>::iterator HandlerRegistrar::begin()
	{
		return _handlers.begin();
    }
	std::vector<ProductHandler*>::iterator HandlerRegistrar::end()
	{
		return _handlers.end();
	}
	std::vector<ProductHandler*>::const_iterator HandlerRegistrar::begin() const
	{
		return _handlers.cbegin();
    }
	std::vector<ProductHandler*>::const_iterator HandlerRegistrar::end() const
	{
		return _handlers.cend();
	}



}