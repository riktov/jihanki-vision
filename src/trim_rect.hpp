/*
Trims the background from an image. A large block of plain background is trimmed from the top,
and the edges are trims by detecting vertical lines.

src_gray: Image to be analyzed for trimming
marked_rows: Vector of rows in the image which contain detected corners. Passed as receptacle, 
    for generating visual feedback
threshold_ratio: Ratio of the maximum corner detection value to generate threshold
*/
cv::Rect get_trim_rect(cv::Mat src_gray, cv::Mat &detected_features, float threshold_ratio = 0.01, bool isEqualize=false) ;
