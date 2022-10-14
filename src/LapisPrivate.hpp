#pragma once
#ifndef lp_privateprocessing_h
#define lp_privateprocessing_h

#include"app_pch.hpp"
#include"LapisTypedefs.hpp"

//The functions defined in this file are intended to be used for defining new analyses that for whatever reason aren't appropriate for a stable release of lapis
//but where the user wants to piggyback on lapis' existing workflow
//In the master branch of the repository, the functions will be called but not do anything. By branching the repository and adding content to the functions,
//you can add private functionality to lapis without needing to understand the entirety of the project

namespace lapis {

	class LapisPrivate {
	public:
		//This constructor is called before any processing is done
		LapisPrivate();

		void oncePerLas(const Extent& e, const LidarPointVector& points);

		void oncePerCsmTile(const Raster<csm_t>& csm);

		void afterProcessing();

	private:
		//any data you need to do your processing can go here
	};
}


#endif