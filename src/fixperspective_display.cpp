/*
Functions used by the fixperspective program to draw for user feedback
to check how it is working. These would not be needed for the actual
batch-processing program as run on a server 
 */
#if CV_VERSION_MAJOR >= 4
    #include <opencv4/opencv2/highgui.hpp>
    #include <opencv4/opencv2/imgproc.hpp>
#else
    #include <opencv2/highgui/highgui.hpp>
    #include <opencv2/imgproc/imgproc.hpp>
#endif


using namespace cv ;

#include <iostream>

#include "display.hpp"
#include "lines.hpp"

void plot_hough_and_bounds(Mat img, std::vector<Vec4i> hough_lines, std::vector<Vec4i> bounds_tblr) {
	Vec4i top_line = bounds_tblr[0] ;
	Vec4i bottom_line = bounds_tblr[1] ;
	Vec4i left_line = bounds_tblr[2] ;
	Vec4i right_line = bounds_tblr[3] ;

	for(const auto &lin : hough_lines) {
		draw_line(img, lin, Scalar(127, 200, 127), 3) ;
	}

	int line_thickness = 24 ;

	std::pair<Vec4i, Scalar> lines_and_colors[] = {
		std::make_pair(left_line, Scalar(255, 0, 0)),
		std::make_pair(right_line, Scalar(0, 255, 0)),
		std::make_pair(top_line, Scalar(0, 0, 255)),
		std::make_pair(bottom_line, Scalar(255, 0, 255))
	} ;

	for(auto l_c : lines_and_colors) {
		draw_line(img, l_c.first, l_c.second, line_thickness) ;
		std::cout << l_c.first << std::endl ;
	}

	Point2f pt_tl = line_intersection(top_line, left_line) ;
	Point2f pt_tr = line_intersection(top_line, right_line) ;
	Point2f pt_br = line_intersection(bottom_line, right_line) ;
	Point2f pt_bl = line_intersection(bottom_line, left_line) ;

	Point2f corners[] = {
		pt_tl, pt_tr, pt_br, pt_bl
	} ;

	Point2f pt_prev = pt_bl ;

	Scalar col_corner = Scalar(255, 255, 0) ;
	Scalar col_edge = Scalar(255, 63, 127) ;
	for(auto pt : corners) {
		circle(img, pt, 10, col_corner, 4) ;
		line(img, pt_prev, pt, col_edge, 8) ;
		pt_prev = pt ;
	} ;
}
