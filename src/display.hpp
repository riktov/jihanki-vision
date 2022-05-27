std::pair<int, int> screen_dims() ;
cv::Mat scale_for_display(cv::Mat src, int width, int height) ;
inline cv::Mat scale_for_display(cv::Mat src) { 
    auto dims = screen_dims() ;
    return scale_for_display(src, dims.first, dims.second) ;
};
inline void draw_line(cv::Mat image, cv::Vec4i l, cv::Scalar color, int width) {
  cv::line(image, cv::Point(l[0], l[1]), cv::Point(l[2], l[3]), color, width, CV_8S);
  //line(canny, Point(0, top_edge), Point(src.cols, top_edge), Scalar(255, 255, 0), 3, CV_AA);
}
