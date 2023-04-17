/*
Functions used by the extract_drinks program to draw for user feedback
to check how it is working. These would not be needed for the actual
batch-processing program as run on a server 
 */

void show_result_images(
    cv::Mat src, 
    std::vector<std::shared_ptr<ButtonStrip> > button_strips, 
    std::vector<std::vector<cv::Rect> > drink_rects, 
    std::vector<std::vector<cv::Rect> > price_rects, 
    std::vector<cv::Rect> price_strips, 
    const char *config_string = ""
    ) ;
void draw_threshold_crossings(cv::Mat img, cv::Rect rc_target, std::vector<int> runs) ;
void draw_markers(cv::Mat img, cv::Rect rc_target, std::vector<int> markers, int height) ;
void draw_button_strip(cv::Mat img, std::shared_ptr<ButtonStrip> strip, cv::Scalar color, std::string label) ;
void draw_strip_boundary(cv::Mat& img, const std::shared_ptr<ButtonStrip> strip) ;
