/**
 * @file perspective_lines.cpp
 * @author Paul Richter (paul@sagasoda.com)
 * @brief A "perspective line" is a line on an image of a 3-D scene. We analyze collections of perspective lines
 * to determine if they are convergent and thus likely to depict lines that are physically parallel, or parts of the
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
void fill_angle_dict(std::map<int, std::vector<perspective_line> > &lines_of_slope, const std::vector<perspective_line> plines) ;
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

	// std::cout << "Constructing a pline: " << this->line << ", slope:" << this->slope << std::endl ;
	if(is_horizontal) {
		//the y intercept is the point where the line crosses the y-axis. It is the y value, where the point's x value is 0.
		if(this->slope == 0) {
			this->zero_intercept = lin[1] ;	//or 3
			this->max_intercept  = lin[1] ;	//or 3
		} else {
			this->zero_intercept = lin[1] - (lin[0] / this->slope) ;
			this->max_intercept  = lin[1] + ((this->max_edge - lin[0]) / this->slope) ;
		}
	} else {
		//the x intercept is the point where the line crosses the x-axis. It is the x value, where the point's y value is 0.
		if(this->slope == 0) {
			this->zero_intercept = lin[0] ;
			this->max_intercept  = lin[0] ;
		} else {
			this->zero_intercept = lin[0] - (lin[1] / this->slope) ;
			this->max_intercept  = lin[0] + ((this->max_edge - lin[1]) / this->slope) ;
		}
	}
	this->angle = angle_deg(this->line) ;
}

Vec4i perspective_line::full_line() {
	bool is_horizontal = abs(this->line[0] - this->line[2]) > abs(this->line[1] - this->line[3]) ;

	if(is_horizontal) {
		return Vec4i(0, this->zero_intercept, this->max_edge, this->max_intercept) ;
	} else {
		return Vec4i(this->zero_intercept, 0, this->max_intercept, this->max_edge) ;
	}
}


/**
 * @brief Rercursively merge a collection of lines into a new set of equal or fewer lines.
 * 
 * @param lines vector of lines
 * @param is_horizontal Boolean: 
 * @param is_merged_only If true, do not return input lines which have been merged
 * @return std::vector<Vec4i> 
 */
std::vector<perspective_line> merge_lines(std::vector<perspective_line> &pers_lines, bool is_horizontal, bool is_merged_only) {

	std::vector<perspective_line> plines_with_merges ;

	auto num_lines_in = pers_lines.size() ;

	//from the vector of plines, build a dictionary keyed by slope
	std::map<int, std::vector<perspective_line> > lines_of_angle ;
	fill_angle_dict(lines_of_angle, pers_lines) ;
	
	//for each collection of plines with a given slope, build a dictionary of lines keyed by intercept
	for(const auto &pair : lines_of_angle) {
		auto angle = pair.first ;
		auto plines = pair.second ;
		if (plines.size() > 1) {
			std::cout << angle << " degrees (" << plines.size() << " lines): " << std::endl ;

			std::map<int, std::vector<perspective_line> > lines_of_intercept ;
			fill_intercept_dict(lines_of_intercept, plines) ;
			
            for(const auto &pair : lines_of_intercept) {
				auto intercept = pair.first ;
				auto plines = pair.second ;

				//merge 
				if (plines.size() > 1) {
					std::cout << "  " << intercept << " intercept (" << plines.size() << ")" << std::endl ; ;
					perspective_line merged = merge_pline_collection(plines) ;
					
					plines_with_merges.push_back(merged) ;	
				} else {
					//push this single pline of this intercept on to lines
					if(!is_merged_only) {
						plines_with_merges.push_back(plines.front()) ;
					}
				}
			}
		} else {
			//push this single pline of this slope on to lines
			if(!is_merged_only) {
				plines_with_merges.push_back(plines.front()) ;
			}
		}
	}

/*
	for(const auto &pline : pers_lines) {
		plines_with_merges.push_back(pline) ;
	}
*/
	auto num_lines_out = plines_with_merges.size() ;

	if(num_lines_out > num_lines_in) {
		return merge_lines(plines_with_merges, is_horizontal, is_merged_only) ;
	}

	return plines_with_merges ;
}

/**
 * @brief Merge a collection of collinear plines in to one line
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

	// std::cout << "Reducing: " ;
	for(auto lin: lines) {
		// std::cout << lin << ", " ;
		reduced = merge_collinear(reduced, lin) ;
	}

	// std::cout << " to " << reduced << std::endl ;


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

		//almost orthogonal is good enough
		if(abs(slope_key) > 56) {
			slope_key = 0 ;
		}

		//round slope on 2 to merge bins
		const int key_resolution = 8 ;	//should depend on image size
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
 * @brief Fill a dictionary (std::map) with plines keyed by integral angle
 * 
 * @param plines_of - Dictionary to fill
 * @param plines - plines
 */
void fill_angle_dict(std::map<int, std::vector<perspective_line> > &plines_of, const std::vector<perspective_line> plines) {
	for(const auto &plin : plines) {
		int angle_key = int(plin.angle) ;

		//round on 2 to merge bins
		const int key_resolution = 8 ;	//should depend on image size
		angle_key = (angle_key / key_resolution) * key_resolution ;

		auto search = plines_of.find(angle_key) ;
		if(search != plines_of.end()) {	//found
			plines_of[angle_key].push_back(plin) ;
		} else {
			std::vector<perspective_line> new_plines ;
			new_plines.push_back(plin) ;
			plines_of[angle_key] = new_plines ;
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
	
		const int key_resolution = 32 ;
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
 * @brief Check that all similarly oriented lines transition in slope the same way.
 * @param plines Collection of input lines
 */
void slope_transitions(std::vector<perspective_line> plines) {
	//sort the plines by intercept

	std::sort(plines.begin(), plines.end(), 
	[](perspective_line pl1, perspective_line pl2){
		return pl1.zero_intercept < pl2.zero_intercept ;
	}) ;

	int iz_before, iz_this, iz_after ;
	int im_before, im_this, im_after ;

	for(size_t i = 1 ; i < plines.size() ; i++) {
		iz_before = plines.at(i - 1).zero_intercept ;
		iz_this   = plines.at(i).zero_intercept ;
		// iz_after  = plines.at(i + 1).zero_intercept ;

		im_before = plines.at(i - 1).max_intercept ;
		im_this   = plines.at(i).max_intercept ;
		// im_after  = plines.at(i + 1).max_intercept ;

		int iz_diff_before = iz_this - iz_before ;
		// int iz_diff_after  = iz_after - iz_this ;

		int im_diff_before = im_this - im_before ;
		// int im_diff_after  = im_after - im_this ;

		//calculate the ratio of spacing between zero-intercepts and max-intercepts
		//for two adjacent lines. This should be fairly constant.

		float ratio_before = (1.0 * iz_diff_before) / im_diff_before ;
		// float ratio_after  = (1.0 * iz_diff_after) / im_diff_after ;

		std::cout << ratio_before << std::endl ;
	}

	std::cout << std::endl ;
}

//New method: get the convergence points for all lines, place them in bins, and get the most populated bin
//Iterate through the lines again, and keep only lines that pass through that area.
void check_convergence(std::vector<perspective_line> plines) {
	//if there are three or fewer lines, we can not determine which point is valid

	//we should get convergence points only for lines which are sufficiently separated.
	//If they are too close, slight variations in slope will make the point vary greatly, 
	//even vary it between positive and negative.
}