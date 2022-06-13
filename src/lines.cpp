#include <cmath>
#include <iostream>
//#include <algorithm>

#if CV_VERSION_MAJOR >= 4
#include <opencv4/opencv2/core.hpp>
#else
#include <opencv2/core/core.hpp>
#endif

#include "lines.hpp"

bool is_horizontal(int x1, int y1, int x2, int y2, int max_dy) {
  // int delta_x = abs(x1 - x2) ;
  int delta_y = abs(y1 - y2) ;

  return (delta_y < max_dy) ;
}


bool is_vertical(int x1, int y1, int x2, int y2, int max_dx) {
  int delta_x = abs(x1 - x2) ;
  // int delta_y = abs(y1 - y2) ;

  return (delta_x < max_dx) ;
}


/* return a float between 0 and 1.0 representing slant from orthogonal to diagonal
Is this essentially just an arcsin * 100?
*/
float normalized_slope(cv::Vec4i line) {
  int x1 = line[0] ;
  int y1 = line[1] ;
  int x2 = line[2] ;
  int y2 = line[3] ;

  if(x1 == x2) {
    return 0 ;
  }

  if(y1 == y2) {
    return 0 ;
  }

  int dtx = abs(x2 - x1) ;
  int dty = abs(y2 - y1) ;

  //  float slope = float(min(dty, dtx)) / float(max(dty, dtx)) ;

  int numer = std::min(dty, dtx) ;
  int denom = std::max(dty, dtx) ;

  //std::cout << numer << "/" << denom <<  std::endl ;

  return (numer * 1.0) / denom ;
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