/**
 * Functions that create or work with monochrome images
 * */
int detect_threshold(cv::Mat img) ;
cv::Mat merge_thresh_images(const std::vector<cv::Mat> images) ;
// cv::Mat fill_bumpy_edge(const cv::Mat img, int spacing_div) ;
// cv::Mat fill_bumpy_edge(std::vector<int> dots, cv::Size img_size, int spacing_div) ;
void fill_bumpy_edge(const cv::Mat &img, cv::Mat &dest, int spacing_div) ;
std::vector<int> boundary(cv::Mat img, int direction) ;
int first_trough(std::vector<int>) ;
cv::Mat remove_solid_rows(cv::Mat &img) ;
void trim_sides_and_expand(cv::Mat &src, cv::Mat &dest, int fraction) ;
void crop_to_lower_edge(const cv::Mat &src, cv::Mat &dst) ;