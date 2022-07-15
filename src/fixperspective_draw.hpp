void plot_bounds(cv::Mat img, const std::vector<cv::Vec4i> bounds_tblr, cv::Scalar rect_color) ;
void plot_margins(cv::Mat &img, cv::Rect rc) ;
void plot_lines(cv::Mat img, const std::vector<cv::Vec4i> lines, cv::Scalar color) ;
void plot_lines(cv::Mat img, const std::vector<perspective_line> plines, cv::Scalar color) ;
void plot_lines(cv::Mat img, const std::pair<perspective_line, perspective_line> plines, cv::Scalar color) ;
void annotate_plines(cv::Mat img, const std::vector<perspective_line> plines) ;

