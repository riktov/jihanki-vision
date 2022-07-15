/**
 * @file perpsective_lines.hpp
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

//structs and classes
struct perspective_line {
	cv::Vec4i line ;
	int slope ;
	int zero_intercept ;
	int max_intercept ;

	perspective_line(cv::Vec4i) ;
} ;

void fill_perspective_lines(std::vector<perspective_line> &plines, std::vector<cv::Vec4i> lines) ;
std::vector<perspective_line> merge_lines(std::vector<perspective_line> &plines, bool is_horizontal) ;
void slope_transitions(std::vector<perspective_line> plines) ;