#pragma once
#ifndef LP_METADATAPDF_H
#define LP_METADATAPDF_H

#include<hpdf.h>
#include<string>

namespace lapis {


	class MetadataPdf {
	public:
		MetadataPdf();
		~MetadataPdf();
		MetadataPdf(const MetadataPdf&) = delete;

		void writeLeftAlignedTextLine(const std::string& text, HPDF_Font font, HPDF_REAL size);
		void writeCenterAlignedTextLine(const std::string& text, HPDF_Font font, HPDF_REAL size);

		void writePageTitle(const std::string& text);
		void writeSubsectionTitle(const std::string& text);
		void writeTextBlockWithWrap(const std::string& text);

		void blankLine();

		void newPage();

		void writeToFile(std::string filename);

		static std::string numberAsDisplayString(double x);
		static std::string numberWithUnits(double x, const std::string& singular, const std::string& plural);
		static std::string strToLower(std::string s);

		HPDF_Font normalFont();
		HPDF_Font boldFont();

	private:
		HPDF_Doc _doc;
		HPDF_Page _currentPage;
		HPDF_Font _normalFont;
		HPDF_Font _boldFont;
		HPDF_REAL _margin = 50;
		HPDF_REAL _currentY;

		void _moveTextDown(HPDF_REAL amount);
		void _newPageIfNeeded();
	};
}

#endif