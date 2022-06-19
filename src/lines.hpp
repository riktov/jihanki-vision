/**
 * lines.hpp
 * 
 * Inteface for functions dealing with lines to determine slopes, intercepts, etc.
 */

bool is_horizontal(int x1, int y1, int x2, int y2, int max_dy) ;

inline bool is_horizontal(cv::Vec4i lin, int max_dy) {
  return is_horizontal(lin[0], lin[1], lin[2], lin[3], max_dy) ;
}

bool is_vertical(int x1, int y1, int x2, int y2, int max_dx) ;

inline bool is_vertical(cv::Vec4i lin, int max_dx) {
  return is_vertical(lin[0], lin[1], lin[2], lin[3], max_dx) ;
}

bool is_zero_line(const cv::Vec4i l) ;

float normalized_slope(cv::Vec4i line) ;

cv::Point2f line_intersection(cv::Vec4i l1, cv::Vec4i l2) ;

cv::Vec4i rt_to_pt(float rho, float theta, double alpha = 1000) ;

float y_intercept(cv::Vec4i lin) ;

bool is_collinear(cv::Vec4i l1, cv::Vec4i l2, double max_diff_angle) ;

inline int mid_x(cv::Vec4i lin) { return (lin[0] + lin[2]) / 2 ; }
inline int mid_y(cv::Vec4i lin) { return (lin[1] + lin[3]) / 2 ; }