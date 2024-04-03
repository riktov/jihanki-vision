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
#include "perspective_lines.hpp"

/**
 * @brief Draw the bounding line segments, and the full quadrilateral made be extending those segments
 * 
 * @param img Source image which will be drawn on
 * @param bounds_tblr vector of 4 points
 * @param rect_color color for the quadrilateral
 */
void plot_bounds(Mat img, const std::vector<Vec4i> bounds_tblr, Scalar rect_color) {
	if (bounds_tblr.empty()) {
		std::cout << "bounds is empty" << std::endl ;
		return ;	
	}
	if (std::any_of(bounds_tblr.begin(), bounds_tblr.end(),
		[](Vec4i l){
			return is_zero_line(l) ;
		})) {
			std::cout << "Zero line in bounds, bailing" << std::endl ;
			return ;
		}

	Vec4i top_line = bounds_tblr[0] ;
	Vec4i bottom_line = bounds_tblr[1] ;
	Vec4i left_line = bounds_tblr[2] ;
	Vec4i right_line = bounds_tblr[3] ;

	std::cout << "About to check for zero lines" << std::endl ;


	int line_thickness = 24 ;

	std::pair<Vec4i, Scalar> lines_and_colors[] = {
		std::make_pair(left_line, Scalar(255, 0, 0)),
		std::make_pair(right_line, Scalar(0, 255, 0)),
		std::make_pair(top_line, Scalar(0, 0, 255)),
		std::make_pair(bottom_line, Scalar(255, 0, 255))
	} ;

	std::cout << "plot_bounds() " << std::endl ;
	for(auto l_c : lines_and_colors) {
		draw_line(img, l_c.first, l_c.second, line_thickness) ;
		std::cout << l_c.first << std::endl ;
	}

	std::cout << "About to get line intersections" << std::endl ;

	Point2f pt_tl = line_intersection(top_line, left_line) ;
	Point2f pt_tr = line_intersection(top_line, right_line) ;
	Point2f pt_br = line_intersection(bottom_line, right_line) ;
	Point2f pt_bl = line_intersection(bottom_line, left_line) ;

	Point2f corners[] = {
		pt_tl, pt_tr, pt_br, pt_bl
	} ;

	Point2f pt_prev = pt_bl ;

	Scalar col_corner = Scalar(255, 255, 0) ;
	Scalar col_edge = rect_color ;//Scalar(255, 63, 127) ;
	for(auto pt : corners) {
		circle(img, pt, 10, col_corner, 4) ;
		line(img, pt_prev, pt, col_edge, 8) ;
		pt_prev = pt ;
	} ;
}

/**
 * @brief display
 * 
 * @param img Ref because image itself is replaced using cvtColor
 * @param rc 
 */
void plot_margins(Mat &img, Rect rc) {
	Mat img_inner = img(rc).clone();

	cvtColor(img_inner, img_inner, COLOR_BGR2GRAY) ;
	cvtColor(img_inner, img_inner, COLOR_GRAY2BGR) ;
	// circle(img, Point(100, 100), 1500, Scalar(255, 45, 0), 50) ;

	img_inner.copyTo(img(rc)) ;

	// Point2i pt_tr = 
	line(img, rc.tl(), Point2i(rc.x + rc.width, rc.y), Scalar(1, 100, 255), 20) ;	//top
	line(img, rc.tl(), Point2i(rc.x, rc.y + rc.height), Scalar(1, 100, 255), 20) ;	//left
	line(img, Point2i(rc.x + rc.width, rc.y), rc.br(), Scalar(1, 100, 255), 20) ;	//right
	line(img, Point2i(rc.x, rc.y + rc.height), rc.br(), Scalar(1, 100, 255), 20) ;	//bottom

}

/**
 * @brief 
 * 
 * @param img 
 * @param lines 
 */
void plot_lines(Mat img, const std::vector<Vec4i> lines, Scalar color) {
	for(auto lin : lines) {
		// std::cout << lin << std::endl ;
		line(img, Point(lin[0], lin[1]), Point(lin[2], lin[3]), color, 4) ;
	}
}

void plot_lines(cv::Mat img, const std::vector<perspective_line> plines, cv::Scalar color) {
	std::vector<Vec4i> lines ;
	/*
	std::transform(plines.begin(), plines.end(), lines.begin(), [](perspective_line plin){
		return plin.line ;
	}) ;
	*/
	for(auto plin : plines) {
		lines.push_back(plin.line) ;
	}
	plot_lines(img, lines, color) ;
}

void plot_lines(cv::Mat img, const std::pair<perspective_line, perspective_line> plines, cv::Scalar color) {
	std::vector<Vec4i> lines ;
	lines.push_back(plines.first.line) ;	
	lines.push_back(plines.second.line) ;

	plot_lines(img, lines, color) ;	
}

void annotate_plines(Mat img, const std::vector<perspective_line> plines, Scalar color) {
	int scale = 2 * img.rows  / 3000  ;
	for(auto plin : plines) {
		float angle = angle_deg(plin.line) ;
		std::string label = std::to_string(cvRound(angle)) + ":" + std::to_string(plin.zero_intercept) ;
		cv::putText(img, label, Point(plin.line[0], plin.line[1]), FONT_HERSHEY_SIMPLEX, scale, color) ;
	}
}