/**
 * lines.hpp
 * 
 * Inteface for functions dealing with lines to determine slopes, intercepts, etc.
 */

bool is_horizontal(int x1, int y1, int x2, int y2, int max_dy=0) ;

inline bool is_horizontal(cv::Vec4i lin, int max_dy=0) { return is_horizontal(lin[0], lin[1], lin[2], lin[3], max_dy) ; }

bool is_vertical(int x1, int y1, int x2, int y2, int max_dx=0) ;

inline bool is_vertical(cv::Vec4i lin, int max_dx=0) { return is_vertical(lin[0], lin[1], lin[2], lin[3], max_dx) ; }

inline bool is_horizontal_generally(cv::Vec4i lin) {
  return abs(lin[0] - lin[2]) > abs(lin[1] - lin[3]) ;
}

bool is_zero_line(const cv::Vec4i l) ;

cv::Vec4i rt_to_pt(float rho, float theta, double alpha = 1000) ;
float slant(cv::Vec4i line) ;
int normalized_slope(cv::Vec4i line) ;
cv::Point2f line_intersection(cv::Vec4i l1, cv::Vec4i l2) ;


float y_intercept(cv::Vec4i lin) ;

bool is_collinear(cv::Vec4i l1, cv::Vec4i l2, double max_diff_angle) ;

inline int mid_x(cv::Vec4i lin) { return (lin[0] + lin[2]) / 2 ; }
inline int mid_y(cv::Vec4i lin) { return (lin[1] + lin[3]) / 2 ; }
inline int len_sq(cv::Vec4i lin) { return std::pow((lin[0] - lin[2]), 2) + std::pow((lin[1] - lin[3]), 2) ; }

cv::Vec4i merge_collinear(cv::Vec4i l1, cv::Vec4i l2) ;
float angle_deg(cv::Vec4i line) ;


inline int horizontal_length(int x1, int x2) { return abs(x1 - x2) ;}
inline int max_x(cv::Vec4i lin) { return std::max(lin[0], lin[2]) ; }
inline int min_x(cv::Vec4i lin) { return std::min(lin[0], lin[2]) ; }
inline int max_y(cv::Vec4i lin) { return std::max(lin[1], lin[3]) ; }
inline int min_y(cv::Vec4i lin) { return std::min(lin[1], lin[3]) ; }
inline bool is_zero_line(const cv::Vec4i l) { return (l[0] == 0 && l[1] == 0 && l[2] == 0 && l[3] == 0) ; }
