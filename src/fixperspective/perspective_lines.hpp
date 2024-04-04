/**
 * @file perpsective_lines.hpp
 * @author Paul Richter (paul@sagasoda.com)
 * @brief An ortho line is a line that is close to vertical or horizontal. We analyze collections of ortho lines
 * to determine if they are convergent and thus likely to depict lines that are physically parallel, or parts of the
 * same physical edge that is partically occluded.
 * @version 0.1
 * @date 2022-07-12
 * 
 * @copyright Copyright (c) 2022
 * 
 */

//structs and classes
struct ortho_line {
	cv::Vec4i line ;
	int slope ;
	float angle ;
	int zero_intercept ;
	int max_intercept ;

	const static int max_edge = 5000 ;

	//ctor
	ortho_line(cv::Vec4i) ;
	ortho_line(float angle, int zero_intercept, bool is_horizontal, int min_extent, int max_extent) ;
	cv::Vec4i full_line() const ;
	int intercept_at(int orth) const ;
	std::string as_string() const ;
	bool is_horizontal() const ;
} ;

void fill_perspective_lines(std::vector<ortho_line> &plines, std::vector<cv::Vec4i> lines) ;
std::vector<ortho_line> merge_lines_binned(std::vector<ortho_line> &plines, bool is_horizontal, bool is_merged_only=false) ;
std::vector<ortho_line> merge_lines(std::vector<ortho_line> &plines, int intercept = 0) ;
std::vector<ortho_line> filter_skewed_lines(std::vector<ortho_line> plines) ;
ortho_line merge_combine_average(ortho_line pl1, ortho_line pl2) ;


