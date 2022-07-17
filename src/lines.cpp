#include <cmath>
#include <iostream>
//#include <algorithm>


#if CV_VERSION_MAJOR >= 4
#include <opencv4/opencv2/core.hpp>
#else
#include <opencv2/core/core.hpp>
#endif

#include "lines.hpp"

using namespace cv ;

bool is_horizontal(int x1, int y1, int x2, int y2, int max_dy) {
  // int delta_x = abs(x1 - x2) ;
  int delta_y = abs(y1 - y2) ;

  return (delta_y <= max_dy) ;
}


bool is_vertical(int x1, int y1, int x2, int y2, int max_dx) {
  int delta_x = abs(x1 - x2) ;
  // int delta_y = abs(y1 - y2) ;

  return (delta_x <= max_dx) ;
}



/**
 * @brief Return a float representing slant from orthogonal (0.0) to diagonal (1.0)
Is this essentially just an arcsin?
 * 
 * @param line Vec4i
 * @return float 
 */
float slant(cv::Vec4i line) {
	int dx = abs(line[0] - line[2]) ;
	int dy = abs(line[1] - line[3]) ;

	if(dx == 0) { return 0 ; }

	if(dy == 0) { return 0 ; }


	//  float slope = float(min(dty, dtx)) / float(max(dty, dtx)) ;

	int numer = std::min(dy, dx) ;
	int denom = std::max(dy, dx) ;

	return (numer * 1.0) / denom ;
}

float angle_deg(Vec4i line) {
	int dx = line[0] - line[2] ;
	int dy = line[1] - line[3] ;

	bool is_vertical = abs(dy) > abs(dx) ;

	float slope ;
	if(is_vertical) {	//vertical
		if(dx == 0) { return 0.0 ; }
		slope = dy / dx ;
	} else {
		if(dy == 0) { return 0.0 ; }
		slope = dx / dy ;
	}

	//from here, just blindly hacking to get the expected results.
	float angle = (atan(slope) * 180 / M_PI) - 90 ;
	if(angle < -90) {
		angle += 180 ;
	}

	if(!is_vertical) {
		angle *= -1 ;
	}
	return angle ;
}


/**
 * @brief Return a signed integer representing the slope in the orientation of the line. Inverse of slant, with sign.
 * 
 * @param line 
 * @return int 
 */
int normalized_slope(Vec4i line) {
	int dx = line[0] - line[2] ;
	int dy = line[1] - line[3] ;

	if(dx == 0) { return 0 ; }
	if(dy == 0) { return 0 ; }

	int slope ;

	if(abs(dy) > abs(dx)) {	//vertical
		slope = dy / dx ;
	} else {
		slope = dx / dy ;
	}

	// std::cout << "Normalized slope of " << line << ": " << slope << std::endl ;
	return slope ;
}


inline int horizontal_length(int x1, int x2) {
  return abs(x1 - x2) ;
}

float y_intercept(cv::Vec4i lin) {
  /* might divide by zero, no checking */
  int x1 = lin[0] ;
  int y1 = lin[1] ;
  int x2 = lin[2] ;
  int y2 = lin[3] ;

  float m = float(y2 - y1) / (x2 - x1) ;
  float b = y1 - (m * x1) ;

  return b ;
}

cv::Point2f line_intersection(cv::Vec4i l1, cv::Vec4i l2) {
  //std::cout << "line_intersection(" << l1 << ", " << l2 << ");" << std::endl ;

  cv::Point2f pt ;

  int l1x1 = l1[0] ;
  int l1x2 = l1[2] ;
  int l1y1 = l1[1] ;
  int l1y2 = l1[3] ;

  int l2x1 = l2[0] ;
  int l2x2 = l2[2] ;
  int l2y1 = l2[1] ;
  int l2y2 = l2[3] ;

  //  std::cout << __FILE__ << ":" << __LINE__ << std::endl ;
  if(l1x1 == l1x2) {//l1 is vertical
    std::cout << "line_intersection(): l1 is vertical." << std::endl ;
    float x = float(l1x1) ;

    float m2 = (l2y2 - l2y1) / (l2x2 - l2x1) ;
    float b2 = y_intercept(l2) ;

    float y = (m2 * x) + b2 ;

    pt.x = x ;
    pt.y = y ;
    return pt ;
  }

  //std::cout << __FILE__ << ":" << __LINE__ << std::endl ;
  if(l2x1 == l2x2) {//l2 is vertical
    std::cout << "l2 is vertical." << std::endl ;
    float x = float(l2x1) ;

    float m1 = (l1y2 - l1y1) / (l1x2 - l1x1) ;
    float b1 = y_intercept(l1) ;

    float y = (m1 * x) + b1 ;

    pt.x = x ;
    pt.y = y ;
    return pt ;
  }

  if(l1y1 == l1y2) { //l1 is horizontal

  }

  float m1 = float(l1y2 - l1y1) / (l1x2 - l1x1) ;
  float m2 = float(l2y2 - l2y1) / (l2x2 - l2x1) ;

  float b1 = y_intercept(l1) ;
  float b2 = y_intercept(l2) ;

  float x = (b2 - b1) / (m1 - m2) ;
  float y = (m1 * x) + b1 ;

  pt.x = x ;
  pt.y = y ;

  return pt ;
}

/*
Convert rho-theta line to Cartesian
The lines are normalized to length of 1, so alpha is
the factor
*/
cv::Vec4i rt_to_pt(float rho, float theta, double alpha) {
	cv::Point pt1, pt2 ;
	double a = cos(theta), b = sin(theta) ;
	double x0 = a*rho, y0 = b*rho ;
	//double alpha = 1000 ;//alpha is the normalized length of the line
	pt1.x = cvRound(x0 + alpha * (-b)) ;
	pt1.y = cvRound(y0 + alpha * (a)) ;
	pt2.x = cvRound(x0 - alpha * (-b)) ;
	pt2.y = cvRound(y0 - alpha * (a)) ;

	cv::Vec4i xyline = cv::Vec4i(pt1.x, pt1.y, pt2.x, pt2.y) ;
	return xyline ;
}


/**
 * @brief Is the line a zero-line: a degenerate line with all points zero
 * 
 * @param l A line
 * @return true Line is a zero-line
 * @return false Line is not zero-line
 */
bool is_zero_line(const cv::Vec4i l) {
  return (l[0] == 0 && l[1] == 0 && l[2] == 0 && l[3] == 0) ;
}

bool is_edge_line(cv::Mat img, cv::Vec4i l) {
  if (l[0] == 0 && l[2] == 0) { //left
    return true ;
  }

  if (l[1] == 0 && l[3] == 0) { //top
    return true ;
  }

  auto right_edge_col = img.cols - 1  ;
 
  if (l[0] == right_edge_col && l[2] == right_edge_col) { //left
    return true ;
  }

  auto bot_edge_col = img.rows - 1 ;
 
  if (l[0] == bot_edge_col && l[2] == bot_edge_col) { //bottom
    return true ;
  }

  return false ;
}

/**
 * @brief Indicate whether two line segments are collinear or nearly so 
 * 
 * @param l1 
 * @param l2 
 * @param max_diff_angle Maximum difference between the lines
 * @return true 
 * @return false 
 */
bool xxxis_collinear(cv::Vec4i l1, cv::Vec4i l2, double max_diff_angle) {
	/* We create four segments connecting the endpoints of the two lines.
  If any of them have slopes that are close enough to either of the two lines,
	they are collinear.
	*/

	std::vector<cv::Vec4i> connectors ;

	connectors.push_back(cv::Vec4i(l1[0], l1[1], l2[0], l2[1])) ;
	connectors.push_back(cv::Vec4i(l1[0], l1[1], l2[2], l2[3])) ;
	connectors.push_back(cv::Vec4i(l1[2], l1[3], l2[0], l2[1])) ;
	connectors.push_back(cv::Vec4i(l1[2], l1[3], l2[2], l2[3])) ;

  std::vector<double> slopes ;

  std::any_of(connectors.begin(), connectors.end(),
    [l1, l2](cv::Vec4i connector){
      return false ;
    }) ;

  return false ;

}

/**
 * @brief Return a line created by merging two collinear line segments.
 * 
 * @param l1 
 * @param l2 
 * @return Vec4i 
 */
Vec4i merge_collinear(Vec4i l1, Vec4i l2) {
	//if vertical, all points on both lines have same x, so max does not identify a unique point
	if(is_vertical(l1, 0)) {
		int min_y = min(min(l1[1], l1[3]), min(l2[1], l2[3])) ;
		int max_y = max(max(l1[1], l1[3]), max(l2[1], l2[3])) ;
	
		return Vec4i(l1[0], min_y, l1[0], max_y) ;
	}

	if(is_horizontal(l1, 0)) {
		int min_x = min(min(l1[0], l1[2]), min(l2[0], l2[2])) ;
		int max_x = max(max(l1[0], l1[2]), max(l2[0], l2[2])) ;
	
		return Vec4i(min_x, l1[1], max_x, l1[1]) ;
	}

	int min_x = min(min(l1[0], l1[2]), min(l2[0], l2[2])) ;
	int max_x = max(max(l1[0], l1[2]), max(l2[0], l2[2])) ;
	int max_x_pt_y ;

	if(l1[0] == max_x) max_x_pt_y = l1[1] ; 
	if(l1[2] == max_x) max_x_pt_y = l1[3] ; 
	if(l2[0] == max_x) max_x_pt_y = l2[1] ; 
	if(l2[2] == max_x) max_x_pt_y = l2[3] ; 

	int min_x_pt_y ;

	if(l1[0] == min_x) min_x_pt_y = l1[1] ; 
	if(l1[2] == min_x) min_x_pt_y = l1[3] ; 
	if(l2[0] == min_x) min_x_pt_y = l2[1] ; 
	if(l2[2] == min_x) min_x_pt_y = l2[3] ; 

	return Vec4i(min_x, min_x_pt_y, max_x, max_x_pt_y) ;
}