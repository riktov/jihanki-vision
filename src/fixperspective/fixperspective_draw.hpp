#include "perspective_lines.hpp"

void plot_bounds(cv::Mat img, const std::vector<cv::Vec4i> bounds_tblr, cv::Scalar rect_color) ;
void plot_margins(cv::Mat &img, cv::Rect rc) ;
void plot_lines(cv::Mat img, const std::vector<cv::Vec4i> lines, cv::Scalar color, int thickness = 4) ;
void plot_lines(cv::Mat img, const std::vector<ortho_line> plines, cv::Scalar color, int thickness = 4) ;
void plot_lines(cv::Mat img, const std::pair<ortho_line, ortho_line> plines, cv::Scalar color,  int thickness = 4) ;
void plot_lines(cv::Mat img, cv::Vec4i lin, cv::Scalar color,  int thickness = 4) ;
void annotate_plines(cv::Mat img, const std::vector<ortho_line> plines, cv::Scalar color, float scale = 1) ;

