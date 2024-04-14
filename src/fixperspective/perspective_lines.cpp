/**
 * @file perspective_lines.cpp
 * @author Paul Richter (paul@sagasoda.com)
 * @brief Line-related functions that are specific to the the fixperspective program
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

extern bool cmdopt_verbose ;

//local function declarations
void fill_slope_dict(std::map<int, std::vector<ortho_line> > &lines_of_slope, const std::vector<ortho_line> plines) ;
void fill_angle_dict(std::map<int, std::vector<ortho_line> > &lines_of_slope, const std::vector<ortho_line> plines) ;
void fill_intercept_dict(std::map<int, std::vector<ortho_line> > &lines_of_intercept, const std::vector<ortho_line> plines) ;
ortho_line merge_pline_collection(const std::vector<ortho_line> plines) ;

/**
 * @brief Construct a new perspective line::perspective line object
 * 
 * @param lin line
 */
ortho_line::ortho_line(Vec4i lin) 
	: line(lin), slope(normalized_slope(lin)), zero_intercept(-1)
{
	bool is_horizontal = abs(lin[0] - lin[2]) > abs(lin[1] - lin[3]) ;

	if(is_horizontal) {
		//the y intercept is the point where the line crosses the y-axis. It is the y value, where the point's x value is 0.
		if(lin[0] > lin[2]) {
			this->line = Vec4i(lin[2], lin[3], lin[0], lin[1]) ;
		}
		if(this->slope == 0) {
			this->zero_intercept = lin[1] ;	//or 3
		} else {
			this->zero_intercept = lin[1] - (lin[0] / this->slope) ;
		}
	} else {
		//the x intercept is the point where the line crosses the x-axis. It is the x value, where the point's y value is 0.
		if(lin[1] > lin[3]) {
			this->line = Vec4i(lin[2], lin[3], lin[0], lin[1]) ;
		}
		if(this->slope == 0) {
			this->zero_intercept = lin[0] ;
		} else {
			this->zero_intercept = lin[0] - (lin[1] / this->slope) ;
		}
	}
	this->angle = angle_deg(this->line) ;
}

ortho_line::ortho_line (float angle, int zero_intercept, bool is_horizontal, int min_extent, int max_extent) {
	int min_offset = zero_intercept + sin(angle * M_1_PI / 180) * min_extent ;
	int max_offset = zero_intercept + sin(angle * M_1_PI / 180) * max_extent ;

	if(is_horizontal) {
		// this-> 
	}
}

Vec4i ortho_line::full_line(int max_edge) const {
	bool is_horizontal = abs(this->line[0] - this->line[2]) > abs(this->line[1] - this->line[3]) ;

	if(is_horizontal) {
		return Vec4i(0, this->zero_intercept, max_edge, this->intercept_at(max_edge)) ;
	} else {
		return Vec4i(this->zero_intercept, 0, this->intercept_at(max_edge), max_edge) ;
	}
}

int ortho_line::intercept_at(int orth) const {
	if(this->slope == 0) {
		return this->zero_intercept ;
	} else {
		return this->zero_intercept + orth / this->slope ;
	}
}

/**
 * @brief The width component, positive or negative relative to the length component.
 * 
 * @return int 
 */
int ortho_line::wc() const { 
	return min(abs(this->line[0] - this->line[2]), abs(this->line[1] - this->line[3])) ; 
}

std::string ortho_line::as_string() const {
	return std::to_string(cvRound(this->angle)) + (this->is_horizontal() ? "H" : "V") + ":" + std::to_string(this->zero_intercept) ;
}

/**
 * @brief Rercursively merge a collection of lines into a new set of equal or fewer lines.
 * 
 * @param lines vector of lines
 * @param is_horizontal orientation of the lines 
 * @param is_merged_only If true, do not return input lines which have been merged
 * @return std::vector<Vec4i> 
 */
std::vector<ortho_line> merge_lines_binned(std::vector<ortho_line> &pers_lines, bool is_horizontal, bool is_merged_only) {

	std::vector<ortho_line> plines_with_merges ;

	auto num_lines_in = pers_lines.size() ;

	//from the vector of plines, build a dictionary keyed by slope
	//problem here is that two lines of 0.999 and 1.001 would span two itegral keys, so they would not be merged.
	std::map<int, std::vector<ortho_line> > lines_of_angle ;
	fill_angle_dict(lines_of_angle, pers_lines) ;
	
	//for each collection of plines with a given slope, build a dictionary of lines keyed by intercept
	for(const auto &pair : lines_of_angle) {
		auto angle = pair.first ;
		auto plines = pair.second ;
		if (plines.size() > 1) {
			std::cout << angle << " degrees (" << plines.size() << " lines): " << std::endl ;

			std::map<int, std::vector<ortho_line> > lines_of_intercept ;
			fill_intercept_dict(lines_of_intercept, plines) ;
			
            for(const auto &pair : lines_of_intercept) {
				auto intercept = pair.first ;
				auto plines = pair.second ;

				//merge 
				if (plines.size() > 1) {
					std::cout << "  " << intercept << " intercept (" << plines.size() << ")" << std::endl ; ;
					ortho_line merged = merge_pline_collection(plines) ;
					
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
		return merge_lines_binned(plines_with_merges, is_horizontal, is_merged_only) ;
	}

	return plines_with_merges ;
}

/**
 * @brief Merge a collection of lines into a new set of equal or fewer lines.
 * 
 * @param lines vector of lines
 * @param intercept where along the length of lines to compare the distance between lines
 * @param sort_by Sort input lines by angle or intercept, SORT_ANGLE or SORT_INTERCEPT
 * @return std::vector<ortho_line> 
 */
std::vector<ortho_line> merge_lines(std::vector<ortho_line> &lines, int intercept, int sort_by) {
	const float ANGLE_DELTA = 1.5 ;
	const float INTERCEPT_DELTA = 10 ;

	if(lines.size() < 2) {
		if(cmdopt_verbose) { std::cout << "Only one line; skipping merge" << std::endl ; }
		return lines ;
	}

	std::sort(lines.begin(), lines.end(), 
	[sort_by, intercept](ortho_line pl1, ortho_line pl2){
		if(sort_by == SORT_ANGLE) {
			return pl1.angle < pl2.angle ;
		} else {
			return pl1.intercept_at(intercept) < pl2.intercept_at(intercept) ;
		}
	}) ;

	std::vector<ortho_line> merged ;

	for(size_t i = 1 ; i < lines.size() ; i++) {
		auto p_prev = lines.at(i - 1) ;
		auto p_this = lines.at(i) ;

		if( (abs(p_prev.angle - p_this.angle) < ANGLE_DELTA) &&
			(abs(p_prev.intercept_at(intercept) - p_this.intercept_at(intercept)) < INTERCEPT_DELTA)) {
			auto merged_pair = merge_combine_average(p_prev.line, p_this.line) ;
			merged.push_back(merged_pair) ;

			if(cmdopt_verbose) {
				std::cout << p_prev.as_string() << " + " << p_this.as_string() << " --> " << merged_pair.as_string() << std::endl   ;
			}

			i++ ;			
		} else {
			merged.push_back(p_prev) ;
			if(cmdopt_verbose) { std::cout << p_prev.as_string() << std::endl ; }

			//if we're on the last element, and it hasn't been merged with the previous one,
			//then it has nothing else to merge with, so push it.
			if((i + 1) == lines.size()) {
				merged.push_back(p_this) ;
				if(cmdopt_verbose) { std::cout << p_this.as_string() << std::endl ; }
			}
			
		}
	} 

	if(cmdopt_verbose) {
		std::cout << "Merged " << lines.size() << " lines in to " << merged.size() << std::endl ;
	}

	if(lines.size() > merged.size()) {
		return merge_lines(merged, intercept, sort_by) ;
	}

	return merged ;
}

/**
 * @brief Merge a collection of collinear plines in to one line
 * 
 * @param plines Collection of lines
 * @return ortho_line 
 */
ortho_line merge_pline_collection(const std::vector<ortho_line> plines) {
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
	ortho_line p(reduced) ;
	
	return p ;
}

/**
 * @brief Fill a dictionary (std::map) with plines keyed by slope
 * 
 * @param plines_of - Dictionary to fill
 * @param plines - plines
 */
void fill_slope_dict(std::map<int, std::vector<ortho_line> > &plines_of, const std::vector<ortho_line> plines) {
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
			std::vector<ortho_line> new_plines ;
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
void fill_angle_dict(std::map<int, std::vector<ortho_line> > &plines_of, const std::vector<ortho_line> plines) {
	for(const auto &plin : plines) {
		int angle_key = int(plin.angle) ;

		//round on 2 to merge bins
		const int key_resolution = 8 ;	//should depend on image size
		angle_key = (angle_key / key_resolution) * key_resolution ;

		auto search = plines_of.find(angle_key) ;
		if(search != plines_of.end()) {	//found
			plines_of[angle_key].push_back(plin) ;
		} else {
			std::vector<ortho_line> new_plines ;
			new_plines.push_back(plin) ;
			plines_of[angle_key] = new_plines ;
		}
	}
}
/**
 * @brief Fill a dictionary (std::map) with plines keyed by intercept
 * 
 * @param lines_of_intercept Dictionary to fill
 * @param plines plines
 */
void fill_intercept_dict(std::map<int, std::vector<ortho_line> > &plines_of, const std::vector<ortho_line> plines) {
	for(const auto &plin: plines) {
		auto intercept_key = plin.zero_intercept ;
	
		const int key_resolution = 32 ;
		intercept_key = (intercept_key / key_resolution) * key_resolution ;
		
		auto search = plines_of.find(intercept_key) ;
		if(search != plines_of.end()) { //found
			plines_of[intercept_key].push_back(plin) ;
		} else {
			std::vector<ortho_line> lines ;
			lines.push_back(plin) ;
			plines_of[intercept_key] = lines ;
		}
	}	
}

/**
 * @brief Filter out skewed lines by checking that lines transition in slope in a proportional way.
 * @param lines Collection of input lines
 * @param max_edge Dimension of the image in the direction of the lines
 */
std::vector<ortho_line> filter_skewed_lines(std::vector<ortho_line> lines, int max_edge) {
	//sort the plines by intercept

	std::sort(lines.begin(), lines.end(), 
	[](ortho_line pl1, ortho_line pl2){
		return pl1.zero_intercept < pl2.zero_intercept ;
	}) ;


	std::vector<ortho_line> filtered ;
	std::vector<float> ratios ;

	// filtered.push_back(plines.at(0)) ;
	const auto MAX_RATIO = 3 ;
	const auto MIN_RATIO = 0.3 ;
	
	for(size_t i = 1 ; i < lines.size() ; i++) {
		// int iz_after, im_after ;
		auto prev_line = lines.at(i - 1) ;
		auto this_line = lines.at(i) ;

		auto iz_this = this_line.zero_intercept ;
		// iz_after  = plines.at(i + 1).zero_intercept ;
		auto im_this = this_line.intercept_at(max_edge) ;
		// im_after  = plines.at(i + 1).max_intercept ;

		int iz_diff_prev = iz_this - prev_line.zero_intercept ;
		// int iz_diff_after  = iz_after - iz_this ;

		int im_diff_prev = im_this - prev_line.intercept_at(max_edge) ;
		// int im_diff_after  = im_after - im_this ;

		//calculate the ratio of spacing between zero-intercepts and max-intercepts
		//for two adjacent lines. This should be fairly constant.

		float ratio_before = (1.0 * iz_diff_prev) / im_diff_prev ;
		// float ratio_after  = (1.0 * iz_diff_after) / im_diff_after ;

		ratios.push_back(ratio_before) ;
	}

	for(auto i = 0 ; i < ratios.size() ; i++) {
		auto rat = ratios.at(i) ;
		if(cmdopt_verbose) {
			std::cout << rat << " -- " ;
			std::cout << lines.at(i).as_string() << ", " ;
			std::cout << lines.at(i + 1).as_string() ;
		}
		
		/* If a line is skewed, then the two transition ratios around it should be unusual 
		
		prev  this
		|   |         |
		|   |    |
		- 3rd line is skewed

		prev  this
		|      | |
		|   |    |
		- 2nd line is skewed


		*/
		if(abs(rat) < MAX_RATIO && abs(rat) > MIN_RATIO) {
		} else {
			if(cmdopt_verbose) {
				std::cout << " <<SKEW>>"  ;
			}
		}
		if(cmdopt_verbose) {
			std::cout << std::endl  ;
		}

	}
	filtered = lines ;	//temporary filler
	return filtered ;
}

//New method: get the convergence points for all lines, place them in bins, and get the most populated bin
//Iterate through the lines again, and keep only lines that pass through that area.

/// @brief 
/// @param plines 
void check_convergence(std::vector<ortho_line> plines) {
	//if there are three or fewer lines, we can not determine which point is valid

	//we should get convergence points only for lines which are sufficiently separated.
	//If they are too close, slight variations in slope will make the point vary greatly, 
	//even vary it between positive and negative.
}

/**
 * @brief Return an ortho line created by merging two similar ortho lines. The resulting segment
 * has a length that spans both segments, and an angle that is the average of the two segments.
 * Since both lines have the same orthogonality, the result will always be between the narrower angle.
 * 
 * @param pl1 a line
 * @param pl2 a line
 * @return perspective_line 
 */
ortho_line merge_combine_average(ortho_line lin1, ortho_line lin2) {
	int max_extent, min_extent, min_offset, max_offset ;

	if(lin1.is_horizontal()) {
		max_extent = max(max(lin1.line[0], lin2.line[0]), max(lin1.line[2], lin2.line[2]))  ;
		min_extent = min(min(lin1.line[0], lin2.line[0]), min(lin1.line[2], lin2.line[2]))  ;
	} else {
		max_extent = max(max(lin1.line[1], lin2.line[1]), max(lin1.line[3], lin2.line[3]))  ;
		min_extent = min(min(lin1.line[1], lin2.line[1]), min(lin1.line[3], lin2.line[3]))  ;
	}

	max_offset = (lin1.intercept_at(max_extent) + lin2.intercept_at(max_extent)) / 2 ;
	min_offset = (lin1.intercept_at(min_extent) + lin2.intercept_at(min_extent)) / 2 ;

	if(lin1.is_horizontal()) {
		return ortho_line(cv::Vec4i(min_extent, min_offset, max_extent, max_offset)) ;
	} else {
		return ortho_line(cv::Vec4i(min_offset, min_extent, max_offset, max_extent)) ;
	}
}