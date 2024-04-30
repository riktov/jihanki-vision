/**
 * @file perpsective_lines.hpp
 * @author Paul Richter (paul@sagasoda.com)
 * @brief Line-related functions that are specific to the the fixperspective program. These deal with identifying
 * lines which may diverge on a 2D image as being parallel in the depicted 3D scenes, and identifying line segments
 * as being part of an occluded continuous line.
 * @version 0.1
 * @date 2022-07-12
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

const int SORT_ANGLE = 0 ;
const int SORT_INTERCEPT = 1 ;

//structs and classes
/**
 * @brief An ortho line is a line that is close to vertical or horizontal. 
 * It is useful in the following ways:
 * 	- slope, angle, and zero intercept are stored as member data.
 *  - The user is assumed to know the orientation. This allows a uniform interface to data
 * such as the length and width components (lc, wc) without worrying if it is longer along the x axis or y axis.
 * 
 * An ortho_line should be considered immutable.
 * Calculations on collections fo ortho lines should be done only on ortho lines of the same orientation (horiz / vert).  
 * 
 */
struct ortho_line {
	cv::Vec4i line ;
	int slope ;
	float angle ;
	int zero_intercept ;

	//ctor
	ortho_line(cv::Vec4i) ;
	ortho_line(float angle, int zero_intercept, bool is_horizontal, int min_extent, int max_extent) ;

	//methods
	cv::Vec4i full_line(int max_edge) const ;
	int intercept_at(int orth) const ;
	std::string as_string() const ;
	inline cv::Point origin() const { return cv::Point(this->line[0], this->line[1]) ; } 
	inline cv::Point end() const { return cv::Point(this->line[2], this->line[3]) ; } 
	inline bool is_horizontal() const {	return abs(this->line[0] - this->line[2]) > abs(this->line[1] - this->line[3]) ; }
	inline int lc() const { return std::max(abs(this->line[0] - this->line[2]), abs(this->line[1] - this->line[3])) ; }
	int wc() const  ;
	inline int sc() const { return wc() ; }
} ;

void fill_perspective_lines(std::vector<ortho_line> &olines, std::vector<cv::Vec4i> lines) ;
std::vector<ortho_line> merge_lines_binned(std::vector<ortho_line> &lines, bool is_horizontal, bool is_merged_only=false) ;
void merge_lines(std::vector<ortho_line> &lines, std::vector<ortho_line> &merged, int intercept = 0, int sort_by = SORT_ANGLE) ;
std::vector<ortho_line> filter_skewed_lines(std::vector<ortho_line> lines, int max_edge) ;
ortho_line merge_combine_average(ortho_line pl1, ortho_line pl2) ;

std::pair<cv::Vec4i, cv::Vec4i> best_vertical_lines(std::vector<cv::Vec4i> lines, int gap) ;
std::pair<cv::Vec4i, cv::Vec4i> best_vertical_lines(std::vector<ortho_line> lines, int gap) ;
std::pair<cv::Vec4i, cv::Vec4i> best_horizontal_lines(std::vector<cv::Vec4i> lines, int gap) ;
std::pair<cv::Vec4i, cv::Vec4i> best_horizontal_lines(std::vector<ortho_line> lines, int gap) ;

void separate_lines(std::vector<cv::Vec4i> lines, std::vector<cv::Vec4i> &horizontals, std::vector<cv::Vec4i> &verticals) ;