#if CV_VERSION_MAJOR >= 4
#include <opencv4/opencv2/imgproc.hpp>
#include <opencv4/opencv2/highgui.hpp>
#else
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#endif

#include <iostream>

using namespace cv ;
 /**
 Detect the threshold for extracting the highest highlight.
 We find the highest local minimum in the value histogram.
 Returns a value between 0 and 255
 */
int detect_threshold(Mat img) {

    std::vector<Mat> hsv_planes ;

	Mat img_hsv ;
	cvtColor(img, img_hsv, COLOR_BGR2HSV) ;
    split(img_hsv, hsv_planes) ;

	int histSize = 256 ; //1-dimensional, with 256 bins
	// int histSize = 128 ; //1-dimensional, with 256 bins

	const float range[] = { 0, 256 } ; //each bin can contain a value n wher 0 > n > 256
    const float* hist_range = { range } ;//only 

	Mat hist_val ;
    Mat img_value = hsv_planes[2] ;

	calcHist(&img_value, 1, 0, Mat(), hist_val, 1, &histSize, &hist_range, true, false) ;

    // for(int i = 0 ; i < 32 ; i++) {
    //     std::cout << hist_val.at<float>(i) << ", ";
    // }
    // std::cout << std::endl ;

    //scan from the max downward for the first local minimum
    const int MAX_OFFSET = 4 ;
    int last_idx = histSize - MAX_OFFSET ;
    float last_val = hist_val.at<int>(last_idx) ;
    int min_idx = last_idx ;
    for( int i = last_idx; i >= 0 ; i--) {
        float this_val = hist_val.at<float>(i) ;
        // std::cout << i << ":" << this_val << ", " ;
        if(this_val > last_val) {
            min_idx = i ;
            break ;
        }
        last_val = this_val ;
    }

    // std::cout << "Detected threshold:" << min_idx << std::endl ;

    /*
    int hist_w = 512, hist_h = 200;
    Mat histImage( hist_h, hist_w, CV_8UC3, Scalar( 0,0,0) );
    normalize(hist_val, hist_val, 0, histImage.rows, NORM_MINMAX, -1, Mat() );


    int bin_w = cvRound( (double) hist_w/histSize );
    for( int i = 1; i < histSize; i++ )
    {
        line( histImage, 
                Point( bin_w*(i-1), hist_h - cvRound(hist_val.at<float>(i-1)) ),
                Point( bin_w*(i), hist_h - cvRound(hist_val.at<float>(i)) ),
                Scalar( 255, 255, 192), 2, 8, 0  );
    }


    imshow("calcHist Demo", histImage );
    waitKey();
    */
    int scale = 256 / histSize ;

	min_idx *= scale;
    min_idx -= 10 ;

    return min_idx ;
}

/** Given a collection of monochrome images,
 * merge them and threshold the result at 255
 * This is effectively the same as ORing them
 * */
Mat merge_thresh_images(const std::vector<Mat> images) {
    int max_width = 0 ;
    //get max width
    for(auto img : images) {
        int this_width = img.cols ;
        if (this_width > max_width) {
            max_width = this_width ;
        }
    }

    std::vector<Mat> resized_images ;

    for(auto img : images) {
        Mat img_resized ;
        //TODO resize only if similar in size, otherwise, repeat sideways
        float ratio = 1.0 * img.cols / max_width ;
        if(ratio > .9) {
            resize(img, img_resized, Size(max_width, 1)) ;
            resized_images.push_back(img_resized) ;
        }
    }

    Mat img_stacked ;
    vconcat(resized_images, img_stacked) ;

    Mat img_reduced ;
    reduce(img_stacked, img_reduced, 0, REDUCE_MAX) ;

    Mat img_unreduced_thresh ;
    threshold(img_stacked, img_unreduced_thresh, 127, 255, THRESH_BINARY) ;

    Mat img_thresh ;
    threshold(img_reduced, img_thresh, 1, 255, THRESH_BINARY) ;

    return img_unreduced_thresh ;
}

/* Given a monochrome image, return an array of the topmost white pixels' row positions 
* It rejects spurious white portions of the image
*/
std::vector<int> top_boundary(const Mat &src) {
    std::vector<int> dots(src.cols) ;
    int max_jump_y = src.rows / 1.5 ;

    for(int i = 0 ; i < src.cols ; i++) {
        //find outermost (lowest, highest) white dot
        //Mat col = img.col(i) ;
        int last_j = 0 ;
        // std::cout << i << ":" << img.cols << std::endl ;

        for(int j = 0 ; j < src.rows ; j++) {
            if(src.at<uchar>(j, i) != 0) {
                if((last_j > 0) && (abs(j - last_j) > max_jump_y)) {
                    std::cout << "Extreme jump in top edge" << std::endl ;
                    dots[i] = last_j ;
                } else {
                    dots[i] = j ;
                }
                break ;
            }
            last_j = j ;
        }
    }
    return dots ;

}

/* Given a monochrome image, return an array of the topmost or bottommost white pixel's row position 
* It rejects spurious white portions of the image
*/
std::vector<int> boundary(Mat img, int direction) {
    std::vector<int> dots(img.cols) ;
    int max_jump_y = img.rows / 1.5 ;

    // dots[0] = 0 ;   //should be midpoint

    for(int i = 0 ; i < img.cols ; i++) {
        //find outermost (lowest, highest) white dot
        //Mat col = img.col(i) ;
        int last_j = 0 ;
        // std::cout << i << ":" << img.cols << std::endl ;

        if(direction == 0) {    //top edge
            for(int j = 0 ; j < img.rows ; j++) {
                if(img.at<uchar>(j, i) != 0) {
                    if((last_j > 0) && (abs(j - last_j) > max_jump_y)) {
                        std::cout << "Extreme jump in top edge" << std::endl ;
                        dots[i] = last_j ;
                    } else {
                        dots[i] = j ;
                    }
                    break ;
                }
                last_j = j ;
            }
        } else {
            //work our way up to the first white dot
            for(int j = img.rows - 1 ; j >= 0 ; j--) {
                if(img.at<uchar>(j, i) != 0) {
                    dots[i] = j ;
                    break ;
                }
            }
            if((i > 0) && (abs(dots[i] - dots[i - 1]) > max_jump_y)) {
                // std::cout << "Extreme jump in bottom at column " << i << std::endl ;
                dots[i] = dots[i - 1] ;
            }
        }
    }
    return dots ;
}
/**
 Given a monochrome image with a bumpy top or bottom edge, fill in the bumps to make it a straight edge
 This will allow us to detect the tilt on an image.
 Since we do not know if the left and right edges are at the top or bottom of a wave,
 we fill it it in, then trim some from the left and right edges.
 */
Mat xxfill_bumpy_edge(std::vector<int> dots, Size img_size, int spacing_div) {
    int spacing = dots.size() / spacing_div ;

    Mat img_filled = Mat(img_size, CV_8U) ;

    for(size_t i = 0 ; i < dots.size() - spacing ; i++) {
        Point pt_this = Point(i, dots[i]) ;
        Point pt_next = Point(i + spacing, dots[i + spacing]) ;
        line(img_filled, pt_this, pt_next, 255) ;
    }

    return img_filled ;
}

/**
 Given a monochrome image with a bumpy top, fill in the peaks to make it a straight edge
 The sky should be black (0) and the mountains white (1-255)
 Since we do not know if the left and right edges are at the peaks or troughs,
 the resulting image should be trimmed a bit on the sides.
 If you want the reverse, just flip the source image!
 */
void fill_peaks(const Mat& src, Mat &dest, int spacing_div) {
    std::vector<int> top_dots = top_boundary(src) ;

    int spacing = src.cols / spacing_div ;

    for(int i = 0 ; i < src.cols - spacing ; i++) {
        Point pt_this = Point(i, top_dots[i]) ;
        Point pt_next = Point(i + spacing, top_dots[i + spacing]) ;
        line(dest, pt_this, pt_next, 255) ;
    }
}

/**
 Given a monochrome image with a bumpy top or bottom edge, fill in the bumps to make it a straight edge
 This will allow us to detect the tilt on an image.
 Since we do not know if the left and right edges are at the top or bottom of a wave,
 we fill it it in, then trim some from the left and right edges.
 */
void fill_bumpy_edge(const Mat& src, Mat &dest, int spacing_div) {
    std::vector<int> bottom_dots = boundary(src, 0) ;
    std::vector<int> top_dots = boundary(src, 1) ;

    int spacing = src.cols / spacing_div ;
    // Mat img_filled_bottom = fill_bumpy_edge(bottom_dots, img.size(), spacing_div) ;
    // Mat img_filled_top    = fill_bumpy_edge(top_dots, img.size(), spacing_div) ;

    for(int i = 0 ; i < src.cols - spacing ; i++) {
        Point pt_this = Point(i, bottom_dots[i]) ;
        Point pt_next = Point(i + spacing, bottom_dots[i + spacing]) ;
        line(dest, pt_this, pt_next, 255) ;
    }

    for(int i = 0 ; i < src.cols - spacing ; i++) {
        Point pt_this = Point(i, top_dots[i]) ;
        Point pt_next = Point(i + spacing, top_dots[i + spacing]) ;
        line(dest, pt_this, pt_next, 255) ;
    }
}


/* Return the slope of the bottom edge of a monochome image
*/
float img_bottom_edge_slope(Mat img, int margin_div = 20) {
    std::vector <float> slopes ;

    int margin = img.cols / margin_div ;
    int nsamples = 10 ;

    //Trim the unreliable edges
    //Sample the slope over several intervals, and throw out the extremes, then take an average

    return 0.0 ;
}

Mat fix_skewed_strip(Mat img_filled, Mat img_bumps) {
    return Mat() ;
}

int first_trough(std::vector<int> edge) {
    const int min_step = 2 ;
    for(size_t i = 1 ; i < edge.size() ; i++) {
        if(edge[i] <= edge[i - 1] - min_step) {
            return i ;
        }
    }
    return -1 ;
}

Mat remove_solid_rows(cv::Mat& img) {
    int top_edge, bottom_edge ;

    Mat img_reduced ;
    
    for(int i = 0 ; i < img.rows ; i++) {
        // auto row = img.row(i) ;
        std::vector<uchar> levels = img.row(i) ;
        if(std::any_of(levels.begin(), levels.end(), 
            [](uchar val){ return (val == 0) ;})) {
            img_reduced.push_back(img.row(i));
        }
    }
    return img_reduced ;
}

/**
 * Given an image, trim some portion from the sides and enlarge it to the same dimensions
 * */
void trim_sides_and_expand(Mat &src, Mat &dst, int fraction) {
    // std::cout << "trim_sides_and_expand" << std::endl ;
    const int margin_x = src.cols / fraction ;
    Rect rc_trim = Rect(Point(margin_x, 0), (Size(src.cols - (2 * margin_x), src.rows))) ;

    // std::cout << "rc_trim: " << rc_trim << std::endl  ;

    Mat img_trimmed = src(rc_trim) ;
    pyrUp(img_trimmed, img_trimmed) ;
    float scale = (src.cols * 1.0) / img_trimmed.cols ; 
    resize(img_trimmed, img_trimmed, Size(), scale, scale) ;

    Rect rcf = Rect(Point(0, (img_trimmed.rows - src.rows) / 2), src.size()) ;
    std::cout << rcf << std::endl ;
    dst = img_trimmed(rcf) ;
}


void crop_to_lower_edge(const Mat &src, Mat &dst) {
    Mat img_top = Mat(src.size(), src.type());

    //fill upward
    for(auto y = 0 ; y < src.rows ; y++) {
		auto rc1 = Rect(Point(0, y), Point(src.cols - 1, src.rows - 1)) ;
		auto rc2 = Rect(Point(0, 0), rc1.size()) ;
		bitwise_or(src(rc1), img_top(rc2), img_top(rc2)) ;

        // std::cout << "rc1: " << rc1 << std::endl ;
        // std::cout << "rc2: " << rc2 << std::endl ;
	}

    Mat img_bottom ;
    threshold(img_top, img_bottom, 127, 255, THRESH_BINARY_INV);

    Mat img_top_filled ; 
    Mat img_bottom_filled = img_bottom.clone() ;

    fill_peaks(img_bottom_filled, img_bottom_filled, 12) ;
    fill_peaks(img_bottom_filled, img_bottom_filled, 9) ;
    fill_peaks(img_bottom_filled, img_bottom_filled, 4) ;
    fill_peaks(img_bottom_filled, img_bottom_filled, 18) ;

    flip(img_top, img_top_filled, 0);
    fill_peaks(img_top_filled, img_top_filled, 12) ;
    fill_peaks(img_top_filled, img_top_filled, 9) ;
    fill_peaks(img_top_filled, img_top_filled, 4) ;
    fill_peaks(img_top_filled, img_top_filled, 18) ;
    flip(img_top_filled, img_top_filled, 0);

    Mat img_cropper = Mat(src.size(), src.type());
    blur(img_cropper, img_cropper, Size(3, 3)) ;
    threshold(img_cropper, img_cropper, 1, 255, THRESH_BINARY);
    blur(img_cropper, img_cropper, Size(3, 3)) ;
    threshold(img_cropper, img_cropper, 1, 255, THRESH_BINARY);

    bitwise_and(img_top_filled, img_bottom_filled, img_cropper) ;
    trim_sides_and_expand(img_cropper, img_cropper, 8);

    Mat img_cropped_top = Mat(img_top.size(), img_top.type());

    bitwise_and(img_cropper, img_top, img_cropped_top) ;

    dst = img_cropper ;
}