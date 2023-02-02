#include"MetadataPdf.hpp"
#include"LapisLogger.hpp"
#include<cassert>
#include<queue>
#include<iomanip>
#include<sstream>

namespace lapis {
	MetadataPdf::MetadataPdf()
	{
		auto error_handler = [](
			HPDF_STATUS error_no,
			HPDF_STATUS detail_no,
			void* user_data) {
#ifndef NDEBUG
				LapisLogger::getLogger().logMessage("PDF Error");
				assert(false);
#endif
		};
		_doc = HPDF_New(error_handler, nullptr);
		_normalFont = HPDF_GetFont(_doc, "Times-Roman", nullptr);
		_boldFont = HPDF_GetFont(_doc, "Times-Bold", nullptr);
	}
	MetadataPdf::~MetadataPdf()
	{
		HPDF_Free(_doc);
	}
	void MetadataPdf::writeLeftAlignedTextLine(const std::string& text, HPDF_Font font, HPDF_REAL size)
	{
		_newPageIfNeeded();
		HPDF_Page_SetFontAndSize(_currentPage, font, size);
		HPDF_Page_ShowText(_currentPage, text.c_str());
		_moveTextDown(2.f * size);
	}
	void MetadataPdf::writeCenterAlignedTextLine(const std::string& text, HPDF_Font font, HPDF_REAL size)
	{
		_newPageIfNeeded();
		HPDF_Page_SetFontAndSize(_currentPage, font, size);
		HPDF_REAL textWidth = HPDF_Page_TextWidth(_currentPage, text.c_str());
		HPDF_REAL moveBy = HPDF_Page_GetWidth(_currentPage) / 2.f - textWidth / 2.f - _margin;
		HPDF_Page_MoveTextPos(_currentPage, moveBy, 0.f);
		HPDF_Page_ShowText(_currentPage, text.c_str());
		HPDF_Page_MoveTextPos(_currentPage, -moveBy, 0.f);
		_moveTextDown(2.f * size);
	}
	void MetadataPdf::writePageTitle(const std::string& text)
	{
		_newPageIfNeeded();
		writeCenterAlignedTextLine(text, _boldFont, 24);
		_moveTextDown(30.f);
	}
	void MetadataPdf::writeSubsectionTitle(const std::string& text)
	{
		_moveTextDown(20.f);
		_newPageIfNeeded();
		writeLeftAlignedTextLine(text, _boldFont, 16);
	}
	void MetadataPdf::writeTextBlockWithWrap(const std::string& text)
	{
		HPDF_Page_SetFontAndSize(_currentPage, _normalFont, 12);

		std::queue<std::string> lines;

		HPDF_UINT _currentBegin = 0;
		HPDF_REAL totalWidth = HPDF_Page_GetWidth(_currentPage) - 2 * _margin;
		while (_currentBegin < text.size()) {

			HPDF_UINT toPrint = HPDF_Font_MeasureText(
				_normalFont,
				(HPDF_BYTE*)text.c_str(),
				(HPDF_UINT)text.size(),
				totalWidth,
				12,
				HPDF_Page_GetCharSpace(_currentPage),
				HPDF_Page_GetWordSpace(_currentPage),
				HPDF_TRUE,
				nullptr
			);

			auto correctToBegin = [&]()->HPDF_UINT {
				//I'm not really sure why, but sometimes harupdf returns really dumb wordwrap suggestions
				//this code is to correct for that
				if (toPrint <= 0) {
					return toPrint;
				}
				if ((size_t)_currentBegin + toPrint >= text.size()) {
					return toPrint;
				}
				for (HPDF_UINT i = _currentBegin + toPrint - 1; i > _currentBegin; --i) {
					if (text[i] == ' ') {
						return (HPDF_UINT)(i - _currentBegin);
					}
				}
				return toPrint;
			};

			toPrint = correctToBegin();

			auto addLine = [&](HPDF_UINT n) {
				const static std::string WHITESPACE = " \n\r\t\f\v";
				std::string s = text.substr(_currentBegin, n);
				size_t start = s.find_first_not_of(WHITESPACE);
				if (start != std::string::npos) {
					lines.push(s.substr(start));
				}
			};

			if (toPrint > 0) {
				addLine(toPrint);
				_currentBegin += toPrint;
			}
			else {
				addLine((HPDF_UINT)text.size() - _currentBegin);
				break;
			}
		}

		while (lines.size()) {
			writeLeftAlignedTextLine(lines.front(), _normalFont, 12);
			lines.pop();
		}
	}
	void MetadataPdf::blankLine()
	{
		_moveTextDown(24.f);
	}
	void MetadataPdf::newPage()
	{
		_currentPage = HPDF_AddPage(_doc);
		HPDF_Page_BeginText(_currentPage);
		_currentY = HPDF_Page_GetHeight(_currentPage) - _margin;
		HPDF_Page_MoveTextPos(_currentPage, _margin, _currentY);
	}
	void MetadataPdf::writeToFile(std::string filename)
	{
		HPDF_SaveToFile(_doc, filename.c_str());
	}
	std::string MetadataPdf::numberAsDisplayString(double x)
	{
		std::stringstream temp;
		temp << std::fixed << std::setprecision(2);
		temp << x;
		std::string out = temp.str();
		out.erase(out.find_last_not_of('0') + 1, std::string::npos);
		out.erase(out.find_last_not_of('.') + 1, std::string::npos);
		return out;
	}
	std::string MetadataPdf::numberWithUnits(double x, const std::string& singular, const std::string& plural)
	{
		std::string s = numberAsDisplayString(x);
		std::string unit = s == "1" ? singular : plural;
		return s + " " + strToLower(unit);
	}
	std::string MetadataPdf::strToLower(std::string s)
	{
		std::transform(s.begin(), s.end(), s.begin(),
			[](unsigned char c) { return std::tolower(c); });
		return s;
	}
	HPDF_Font MetadataPdf::normalFont()
	{
		return _normalFont;
	}
	HPDF_Font MetadataPdf::boldFont()
	{
		return _boldFont;
	}
	void MetadataPdf::_moveTextDown(HPDF_REAL amount)
	{
		HPDF_Page_MoveTextPos(_currentPage, 0, -amount);
		_currentY -= amount;
	}
	void MetadataPdf::_newPageIfNeeded()
	{
		if (_currentY < _margin) {
			newPage();
		}
	}
}