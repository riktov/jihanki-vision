#if CV_VERSION_MAJOR >= 4
#include <opencv4/opencv2/highgui.hpp>
#include <opencv4/opencv2/imgproc.hpp>
#include <opencv4/opencv2/calib3d.hpp>
#else
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#endif

using namespace cv ;

#include <iostream>
#include "trim_rect.hpp"

extern bool cmdopt_verbose ;

int max_vertical_aggregation(Mat img) ;

/**
 * Trim a rectangle using two methods:
 * 1) Remove flat background. Detect corners and trim off everything above the highest detected corner point
 *    The most prominent corners will be within the container design
 * 2) Trim sides. Detect the two sides of the container with edges. Select the two most prominent vertical lines 
 *    (aggregations of edge points) on each side of the image.
 */
Rect get_trim_rect(Mat src_gray, Mat &detected_corners, float threshold_ratio, bool is_equalize)
{
    Mat img_corners;
    //, img_corners_norm, img_corners_no.rm_scaled ;s
    //, img_corners, img_corners_norm, img_corners_norm_scaled;
    
    cornerHarris(src_gray, img_corners, 2, 3, 0.04);

    /* The Python sample progrm filters out corners through a threshold of 0.01 of the max value.
    This seems to work out a lot better than the normalization scheme used
    in the C++ sample. Perhaps that was chosen because the demo program is interactive.
    But in our case, we just want an initial value that works, and this
    is much better.
    */

    /*
    normalize(img_corners, img_corners_norm, 0, 255, NORM_MINMAX, CV_32FC1, Mat() );
    img_corners_norm_scaled = img_corners_norm ;
    convertScaleAbs(img_corners_norm, img_corners_norm_scaled );
    */

    double max_val = 0;
    double min_val = 0;

    minMaxIdx(img_corners, &min_val, &max_val) ;
        
    if (cmdopt_verbose)
    {
        // std::cout << "Min:" << min_val;
        // std::cout << ", Max:" << max_val << std::endl;
    }

    float corner_threshold_val = max_val * threshold_ratio ;

    threshold(img_corners, img_corners, corner_threshold_val, 255, THRESH_BINARY) ;
    
    // cv::imshow("Detected Corners", img_corners);
    // cv::waitKey();
    // return Rect(0, 0, 0, 0);


    std::vector<size_t> marked_rows ;
    
    // Consider a more graphic but inefficient way: reduce to a column, threshold at 1, 

    for (size_t i = 0; i < (size_t)img_corners.rows; i++) {
        Mat row = img_corners.row(i);

        if (std::any_of(row.begin<int>(), row.end<int>(), [](int val) { return val > 0; })) {
            marked_rows.push_back(i);
        }
    }

    //find the greatest gap between consecutive marked rows,
    //this should be the background area at the top of the slot

    int top_edge = 0;
    int bottom_edge = 0;
    int prev = 0;

    for (auto y : marked_rows) {
        if(y > (size_t)((src_gray.rows * 3) / 2)) {
            break ;
        }
        int span = y - prev;
        if (span > bottom_edge - top_edge)
        {
            top_edge = prev;
            bottom_edge = y;
        }
        prev = y;
    }

    //check if the detected value is reasonable for an extracted slot image
    //if not, reject it and set it back to the top
    if ((src_gray.rows - bottom_edge) < src_gray.cols * 1.3) {
        bottom_edge = 0 ;
    }

    Rect rc_clip = Rect(0, bottom_edge, src_gray.cols, src_gray.rows - bottom_edge);

    // std::cout << rc_clip << std::endl ;
    
    if(detected_corners.size() == src_gray.size()) {
        detected_corners = img_corners ;
    }
    
    //!! don't trim sides!!
    return rc_clip ;

    
	/////////////////
    // trim the sides by detecting edges

    Mat img_canny ;

    int hys_min = 15 ;
    int hys_max = 120 ;
    //default 30, 450
    // 100, 350 good for text logos
    
    
    //canny is greatly affected by equalizeHist
    if(is_equalize) {
        equalizeHist(src_gray, src_gray);
        hys_min = 40 ;
        hys_max = 200 ;
    }

    Canny(src_gray(rc_clip), img_canny, hys_min, hys_max, 3) ;

	int inset = img_canny.cols / 4 ;

	auto rc_left = Rect(0, 0, inset, img_canny.rows) ;
	auto rc_right = Rect(img_canny.cols - inset, 0, inset, img_canny.rows) ;

	int max_idx_left  = max_vertical_aggregation(img_canny(rc_left)) ;
	int max_idx_right = max_vertical_aggregation(img_canny(rc_right)) ;

    int left_edge = max_idx_left ;
    int right_edge = img_canny.cols - inset + max_idx_right ;

    auto rc_trimmed_sides = Rect(left_edge, 0, right_edge - left_edge, src_gray.rows) ;

    rc_clip &= rc_trimmed_sides ;

    if(false) {
        cv::imshow("Detected Canny Edges", img_canny);
        cv::waitKey();
    }

    return rc_clip;
}

/*
Returns the index of the column in a binary image with the greatest number of white dots
*/
int max_vertical_aggregation(Mat img) {
    Mat img_vert_lines ;
	reduce(img, img_vert_lines, 0, REDUCE_AVG) ;
	pyrDown(img_vert_lines, img_vert_lines) ;   //combine pairs of adjacent lines into one

    // std::cout << img_vert_lines << std::endl ;

	double min_val, max_val ;
	int min_idxs[2], max_idxs[2] ;    //We are still dealing with a 2-dim matrix

    minMaxIdx(img_vert_lines, &min_val, &max_val, min_idxs, max_idxs) ;

    int max_idx = max_idxs[1];
	// std::cout << max_val << " at " << max_idx << std::endl ;

	return max_idx * 2;	//scale back up after pyrDown ;
}