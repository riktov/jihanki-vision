/** extract_drinks_write.hpp
 */



void write_slot_image_files(
    cv::Mat src,
    std::vector<std::vector<cv::Rect> > slot_image_rows, 
    std::string outfilepath,
    const std::string prefix
    ) ;
