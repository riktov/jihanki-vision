/**
 * @file perspective_lines.cpp
 * @author Paul Richter (paul@sagasoda.com)
 * @brief A "perspective line" is a line on an image of a 3-D scene. We analyze collections of perspective lines
 * to determine if they are likely to depict lines that are physically parallel, or parts of the
 * same physical edge that is partically occluded.
 * @version 0.1
 * @date 2022-07-12
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#if CV_VERSION_MAJOR >= 4
#include <opencv4/opencv2/core.hpp>
#else
#include <opencv2/core/core.hpp>
#endif

#include <iostream>
#include <map>

using namespace cv ;

#include "perspective_lines.hpp"
#include "lines.hpp"

//local function declarations
void fill_slope_dict(std::map<int, std::vector<perspective_line> > &lines_of_slope, const std::vector<perspective_line> plines) ;
void fill_intercept_dict(std::map<int, std::vector<perspective_line> > &lines_of_intercept, const std::vector<perspective_line> plines) ;
perspective_line merge_pline_collection(const std::vector<perspective_line> plines) ;

/**
 * @brief Construct a new perspective line::perspective line object
 * 
 * @param lin 
 */
perspective_line::perspective_line(Vec4i lin) 
	: line(lin), slope(normalized_slope(lin)), zero_intercept(-1)
{
	bool is_horizontal = abs(lin[0] - lin[2]) > abs(lin[1] - lin[3]) ;

	if(is_horizontal) {
		//the y intercept is the point where the line crosses the y-axis. It is the y value, where the point's x value is 0.
		if(this->slope == 0) {
			this->zero_intercept = lin[1] ;	//or 3
		} else {
			this->zero_intercept = lin[1] + (lin[0] / this->slope) ;
		}
	} else {
		//the x intercept is the point where the line crosses the x-axis. It is the x value, where the point's y value is 0.
		if(this->slope == 0) {
			this->zero_intercept = lin[0] ;
		} else {
			this->zero_intercept = lin[0] + (lin[1] / this->slope) ;
		}
	}
}

void fill_perspective_lines(std::vector<perspective_line> &plines, std::vector<cv::Vec4i> lines) {
	if(lines.size() < 1) { return ; }
	Vec4i any_line = lines.front() ;
	bool is_horizontal = abs(any_line[0] - any_line[2]) > abs(any_line[1] - any_line[3]) ;

	for(auto lin : lines) {
		perspective_line pl(lin) ;

		int pseudo_slope ; 	
		int dx = lin[0] - lin[2] ;
		int dy = lin[1] - lin[3] ;
		int d_short, d_long ;

		if(is_horizontal) {
			d_short = dy ;
			d_long  = dx ;
		} else {
			d_short = dx ;
			d_long  = dy ;
		}

		if(d_short == 0) {
			pseudo_slope = 0 ;
		} else {
			pseudo_slope = d_long / d_short ;
		}


		int intercept = -1 ;

		if(is_horizontal) {
			//the y intercept is the point where the line crosses the y-axis. It is the y value, where the point's x value is 0.
			if(pseudo_slope == 0) {
				intercept = lin[1] ;	//or 3
			} else {
				intercept = lin[1] + (lin[0] / pseudo_slope) ;
			}
		} else {
			//the x intercept is the point where the line crosses the x-axis. It is the x value, where the point's y value is 0.
			if(pseudo_slope == 0) {
				intercept = lin[0] ;
			} else {
				intercept = lin[0] + (lin[1] / pseudo_slope) ;
			}
		}

		pl.line = lin ;
		pl.slope = pseudo_slope ;
		pl.zero_intercept = intercept ;

		plines.push_back(pl) ;
	
	}
}

/**
 * @brief 
 * 
 * @param lines 
 * @param is_horizontal 
 * @return std::vector<Vec4i> 
 */
std::vector<perspective_line> merge_lines(std::vector<perspective_line> &pers_lines, bool is_horizontal) {

	std::vector<perspective_line> plines_with_merges ;

	//from the vector of plines, build a dictionary keyed by slope
	std::map<int, std::vector<perspective_line> > lines_of_slope ;
	fill_slope_dict(lines_of_slope, pers_lines) ;
	
	//for each collection of plines with a given slope, build a dictionary of lines keyed by intercept
	for(const auto &pair : lines_of_slope) {
		auto slope = pair.first ;
		auto plines = pair.second ;
		if (plines.size() > 1) {
			std::cout << slope << " slope (" << plines.size() << "): " << std::endl ;

			std::map<int, std::vector<perspective_line> > lines_of_intercept ;
			fill_intercept_dict(lines_of_intercept, plines) ;
			
            for(const auto &pair : lines_of_intercept) {
				auto intercept = pair.first ;
				auto plines = pair.second ;

				//merge 
				if (plines.size() > 1) {
					std::cout << "  " << intercept << " intercept: " ;
					perspective_line merged = merge_pline_collection(plines) ;
					
					// plines_with_merges.push_back(merged) ;	

					for(auto plin : plines) {	//test
						plines_with_merges.push_back(plin) ;
					}				
				} else {
					//push this single pline of this intercept on to lines
					// plines_with_merges.push_back(plines.front()) ;
				}
			}
		} else {
			//push this single pline of this slope on to lines
			// plines_with_merges.push_back(plines.front()) ;
		}
	}

/*
	for(const auto &pline : pers_lines) {
		plines_with_merges.push_back(pline) ;
	}
*/
	return plines_with_merges ;
}

/**
 * @brief Merge a collection of plines 
 * 
 * @return perspective_line 
 */
perspective_line merge_pline_collection(const std::vector<perspective_line> plines) {
	std::vector<Vec4i> lines ;

	//transform vector of plines to vector of lines
	for(auto plin : plines) {
		lines.push_back(plin.line) ;
	}
	/* can't get transform to work
	std::transform(plines.begin(), plines.end(), lines.end(), [](perspective_line plin){
		return plin.line ;
	}) ;
	*/
	//reduce vector of lines with merge_collinear
	//std::reduce() only from C++17
	Vec4i reduced = lines.front() ;

	std::cout << "Reducing: " ;
	for(auto lin: lines) {
		std::cout << lin << ", " ;
		reduced = merge_collinear(reduced, lin) ;
	}

	std::cout << " to " << reduced << std::endl ;


	//return single pline made from line
	perspective_line p(reduced) ;
	
	return p ;
}

/**
 * @brief Fill a dictionary (std::map) with plines keyed by slope
 * 
 * @param plines_of - Dictionary to fill
 * @param plines - plines
 */
void fill_slope_dict(std::map<int, std::vector<perspective_line> > &plines_of, const std::vector<perspective_line> plines) {
	for(const auto &plin : plines) {
		auto slope_key = plin.slope ;

		//round slope on 2 to merge bins
		const int key_resolution = 4 ;	//should depend on image size
		slope_key = (slope_key / key_resolution) * key_resolution ;

		auto search = plines_of.find(slope_key) ;
		if(search != plines_of.end()) {	//found
			plines_of[slope_key].push_back(plin) ;
		} else {
			std::vector<perspective_line> new_plines ;
			new_plines.push_back(plin) ;
			plines_of[slope_key] = new_plines ;
		}
	}
}

/**
 * @brief Fill a dictionary (std::map) with plines keyed by intercept
 * 
 * @param lines_of_intercept - Dictionary to fill
 * @param plines - plines
 */
void fill_intercept_dict(std::map<int, std::vector<perspective_line> > &plines_of, const std::vector<perspective_line> plines) {
	for(const auto &plin: plines) {
		auto intercept_key = plin.zero_intercept ;
	
		const int key_resolution = 16 ;
		intercept_key = (intercept_key / key_resolution) * key_resolution ;
		
		auto search = plines_of.find(intercept_key) ;
		if(search != plines_of.end()) { //found
			plines_of[intercept_key].push_back(plin) ;
		} else {
			std::vector<perspective_line> lines ;
			lines.push_back(plin) ;
			plines_of[intercept_key] = lines ;
		}
	}	
}

/**
 * @brief 
 * 
 * Check that all similarly oriented lines transition in slope the same way.
 */
void slope_transitions(std::vector<perspective_line> plines) {
	//sort the plines by intercept

	std::sort(plines.begin(), plines.end(), 
	[](perspective_line pl1, perspective_line pl2){
		return pl1.zero_intercept < pl2.zero_intercept ;
	}) ;

	int s1, s2, s3 ;
	int i1, i2, i3 ;

	for(size_t i = 2 ; i < plines.size() ; i++) {
		s1 = plines.at(i - 2).slope ;
		s2 = plines.at(i - 1).slope ;
		s3 = plines.at(i).slope ;

		i1 = plines.at(i - 2).zero_intercept ;
		i2 = plines.at(i - 1).zero_intercept ;
		i3 = plines.at(i).zero_intercept ;

		std::cout << i1 << ":" << s1 << " -> " ;
	}
	std::cout << i2 << ":" << s2 << " -> " ;
	std::cout << i3 << ":" << s3 << " -> " ;

	std::cout << std::endl ;
}

