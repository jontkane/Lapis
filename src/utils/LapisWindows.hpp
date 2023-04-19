#pragma once
#ifndef LP_LAPISWINDOWS_H
#define LP_LAPISWINDOWS_H

//This header file contains utilities to handle Windows-specific stuff

#ifdef _WIN32
#undef APIENTRY
#define _WIN32_WINNT 0x0501
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include<windows.h>
#include<shellapi.h>
#undef near
#endif

#include<iostream>
#include<sstream>

namespace lapis {
	class LapisWindowsWriter {
	public:
		LapisWindowsWriter() : _os(nullptr), _handle(nullptr) {}

		//HANDLE is an alias for void*; using the true type so that this compiles on other OSes
		LapisWindowsWriter(std::ostream* os, void* windowsHandle) : _os(os), _handle(windowsHandle) {}

		template<class T>
		LapisWindowsWriter& operator<<(const T& t) {
			if (_os) {
				*_os << t;
			}
#ifdef _WIN32
			if (_handle == nullptr) {
				return *this;
			}
			std::stringstream ss;
			ss << t;
			std::string s = ss.str();
			DWORD bytesWritten = 0;
			WriteFile(_handle, s.c_str(), (DWORD)s.size(), &bytesWritten, nullptr);
#endif
			return *this;
		}

	private:
		std::ostream* _os;
		void* _handle;
	};


	//these variables get re-defined if the program enters via WinMain
	inline LapisWindowsWriter lapisCout = LapisWindowsWriter(&std::cout, nullptr);
	inline LapisWindowsWriter lapisCerr = LapisWindowsWriter(&std::cerr, nullptr);
}
#endif