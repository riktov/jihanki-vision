#include <iostream>

#if CV_VERSION_MAJOR >= 4
#include <opencv4/opencv2/highgui.hpp>
#include <opencv4/opencv2/imgproc.hpp>
#include <opencv4/opencv2/calib3d.hpp>
// #include <opencv4/opencv2/viz/types.hpp>
#else
// #include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
// #include <opencv2/viz/types.hpp>
#endif

using namespace cv ;

#include "../lines.hpp"
#include "../display.hpp"

extern bool cmdopt_verbose ;

/**
 * @brief Create an image showing dense areas which have lots of edge dots, and merge them into big blotches
 * 
 * @param img_edges Monochrome image with Canny edges
 * @return Mat 
 */
void detect_dense_areas(Mat img_edges, Mat &img_out) {
	// Mat img_out = img_edges.clone();

	const auto dilation_type = MORPH_ERODE ;
	auto dilation_size = 5 ;	//2
	Mat element = getStructuringElement(dilation_type,
						Size( 2*dilation_size + 1, 2*dilation_size+1 ),
						Point( dilation_size, dilation_size ) );

	dilation_size = 1 ;
	Mat element2 = getStructuringElement(dilation_type,
						Size( 2*dilation_size + 1, 2*dilation_size+1 ),
						Point( dilation_size, dilation_size ) );

	const int pyr_factor = 3;	//3

	img_edges.copyTo(img_out) ;
	
	for(int i = 0 ; i <  pyr_factor ; i++) {
		pyrDown(img_out, img_out);
	}

	blur(img_out, img_out, Size(3, 3)) ;

//    /* CURRENt WORKING 
	dilate(img_out, img_out, element );
	erode(img_out, img_out, element);

	dilate(img_out, img_out, element );
	erode(img_out, img_out, element);

	//if this is left in, it tends to allow detection of lots of overlapping lines at the margin
	//but it omitted, it doesn't allow enough margin and lines might not be detected
	erode(img_out, img_out, element2, Point(-1, -1), 2);
  //  */



    ///////////
	//OK results: blur, dilate, threshold, erode
	//erode, blur, dilate, threshold : nothing

	// threshold(img_working, img_working, 31, 255, THRESH_BINARY) ;

	// std::cout << "Image size after " << pyr_factor << " pyrDown: " << img_working.size() << std:: endl ;  
	for(int i = 0 ; i <  pyr_factor ; i++) {
		pyrUp(img_out, img_out);
	}

	threshold(img_out, img_out, 31, 255, THRESH_BINARY) ;

/*	GOOD RESULTS:
	down, blur, dilate, erode, up, thresh
	down, blur, dilate, erode+, up, thresh
	down 4, blur 3, dilate 5, erode, dilate, erode, up, thresh 31!!!!

*/
	// return img_out(Rect(0, 0, img_edges.cols, img_edges.rows)) ;
}


void detect_dense_areas2(Mat img_edges, Mat &img_out) {
	//The shape (contours) of the block does not matter, 
	//so we don't need to dilate and erode,
	//even blurring isn't needed as pyrDown will do it for us 
	//We just need to space between very dense area.

	// Mat img_out = img_edges.clone();

	const int dilation_size = 5 ;
	Mat kernel_dilate = getStructuringElement(MORPH_DILATE,
						Size( 2*dilation_size + 1, 2*dilation_size+1 ),
						Point( dilation_size, dilation_size ) );

	Mat kernel_erode = getStructuringElement(MORPH_ERODE,
						Size( 2*dilation_size + 1, 2*dilation_size+1 ),
						Point( dilation_size, dilation_size ) );


	const int blockiness = 3;	//3

	img_edges.copyTo(img_out) ;

	for(int i = 0 ; i <  blockiness ; i++) {
		pyrDown(img_out, img_out);
	}

	// blur(img_out, img_out, Size(3, 3)) ;
	// threshold(img_out, img_out, 191, 255, THRESH_BINARY) ;
	dilate(img_out, img_out, kernel_dilate, Point(-1, -1), 1);
	erode(img_out, img_out, kernel_erode, Point(-1, -1), 3);

	for(int i = 0 ; i <  blockiness ; i++) {
		pyrUp(img_out, img_out);
	}

	erode(img_out, img_out, kernel_erode, Point(-1, -1), 2);

	threshold(img_out, img_out, 1, 255, THRESH_BINARY) ;

}

void detect_dense_areas_simple(Mat img_edges, Mat &img_out) {
	//The shape (contours) of the block does not matter, 
	//so we don't need to dilate and erode,
	//even blurring isn't needed as pyrDown will do it for us 
	//We just need to space between very dense area.

	// Mat img_out = img_edges.clone();


	const int blockiness = 4;	//3

	img_edges.copyTo(img_out) ;

	for(int i = 0 ; i <  blockiness ; i++) {
		// blur(img_out, img_out, Size(3, 3)) ;
		threshold(img_out, img_out, 15, 255, THRESH_BINARY) ;
		pyrDown(img_out, img_out);
	}

	blur(img_out, img_out, Size(3, 3)) ;
	threshold(img_out, img_out, 208, 255, THRESH_BINARY) ;

	for(int i = 0 ; i <  blockiness ; i++) {
		pyrUp(img_out, img_out);
		threshold(img_out, img_out, 207, 255, THRESH_BINARY) ;
	}

	resize(img_out, img_out, img_edges.size()) ;
}

void xxdetect_dense_areas_simple(Mat img_edges, Mat &img_out) {
	//The shape (contours) of the block does not matter, 
	//so we don't need to dilate and erode,
	//even blurring isn't needed as pyrDown will do it for us 
	//We just need to space between very dense area.

	// Mat img_out = img_edges.clone();


	const int blockiness = 4;	//3

	img_edges.copyTo(img_out) ;

	for(int i = 0 ; i <  blockiness ; i++) {
		pyrDown(img_out, img_out);
		threshold(img_out, img_out, 1, 255, THRESH_BINARY) ;
	}

	for(int i = 0 ; i <  blockiness ; i++) {
	}

	resize(img_out, img_out, img_edges.size()) ;
}


/**
 * @brief Detect good lines on a canny edge image
 * 
 * @param img_cann image with edges
 * @param strip_offset ??
 * @return std::vector<Vec4i> 
 */
std::vector<Vec4i> detect_lines(Mat img_edges, int threshold, int strip_offset) {
	// Mat cann ;
	std::vector<Vec4i> lines ;
	
	// const int hough_threshold = 400 ;	//250, 200 300 - 400
	//with new algorithm, raising max_gap to 100 helps

	bool is_vertical = img_edges.rows > img_edges.cols ;

	const int min_vertical_length = img_edges.rows / 3 ; //4
	const int min_horizontal_length = img_edges.cols / 3 ; //6, 10

	const int min_length = is_vertical ? min_horizontal_length : min_vertical_length ;
	const int max_gap = min_length / 6 ; //70

	//HoughLinesP(img_cann, lines, 1, CV_PI/45, threshold, min_length, max_gap) ;
	// medianBlur(img_cann, img_cann, 5) ;
    const auto theta = CV_PI/180 ;

	HoughLinesP(img_edges, lines, 1, theta, threshold, min_length, max_gap) ;

	int num_all_detected = -1 ;
	if(cmdopt_verbose) {
		num_all_detected = lines.size() ;
	}
	
	//remove diagonal lines

	auto slant_iter = std::remove_if(lines.begin(), lines.end(),
		[](Vec4i lin){ return slant(lin) > 0.1 ; }) ;	// about 10 degrees
	lines.erase(slant_iter, lines.end()) ;

	if(cmdopt_verbose) {
		// int num_filtered = lines.size() ;
		// std::cout << num_all_detected - num_filtered << " diagonals removed." << std::endl ;
	}

	return lines ;
}

/**
 * @brief Return a value indicating how likely it is that the photograph depicts
 * a vending machine at night with the display panel illumination on. 
 * 
 * @param img 
 * @return float 
 */
float lighting(Mat img) {
	//take strips off the top and bottom
	//take strips off the sides of the remainder
	//compare the mean value of the the strips and the inner section

	Mat img_grey ;
	cvtColor(img, img_grey, COLOR_BGR2GRAY) ;

	//slice it in thirds so that all strips are the same thicknss.
	int top = img.size().height / 3 ;
	int left = img.size().width / 3 ;
	int vstrip_thickness = left ;
	int hstrip_thickness = top ;
	int bottom  = img.size().height - hstrip_thickness ;
	int right  = img.size().width - vstrip_thickness ;

	auto rc_center = Rect(vstrip_thickness, hstrip_thickness, vstrip_thickness, hstrip_thickness) ;
	auto roi_top = img(Rect(0, 0, img.size().width, hstrip_thickness)) ;
	auto roi_bot = img(Rect(0, bottom, img.size().width, hstrip_thickness)) ;
	auto roi_left  = img(Rect(0, top, img.size().width, vstrip_thickness)) ;
	auto roi_right = img(Rect(0, bottom, img.size().width, vstrip_thickness)) ;

	return 0.0 ;
}
