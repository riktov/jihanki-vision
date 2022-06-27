#if CV_VERSION_MAJOR >= 4
#include <opencv4/opencv2/highgui.hpp>
#include <opencv4/opencv2/imgproc.hpp>
#include <opencv4/opencv2/calib3d.hpp>
#else
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#endif

#ifdef USE_EXIV2
#include <exiv2/exiv2.hpp>
#endif

#include <string>
#include <iostream>
#include <unistd.h>
#include <libgen.h>

#include "lines.hpp"

#ifdef USE_GUI
#include "fixperspective_display.hpp"
#include "display.hpp"
#endif



//#define VERBOSE_MESSAGE(msg) if(cmdopt_verbose) { std::cout << msg ;}

using namespace cv ;

/* local function declarations */
int process_file(char *src_file, const char *dest_file) ;
Mat transform_perspective(Mat img, Vec4i top, Vec4i bottom, Vec4i left, Vec4i right) ;
inline Mat transform_perspective(Mat img, const std::vector<Vec4i>lines_tblr) {
  return transform_perspective(img, lines_tblr[0], lines_tblr[1], lines_tblr[2], lines_tblr[3]) ;
}
//Mat crop_orthogonal(Mat src) ;
std::vector<Point2f> line_corners(Vec4i top, Vec4i bottom, Vec4i left, Vec4i right) ;
std::vector<Vec4i> detect_bounding_lines(Mat src, int hough_threshold = 50) ;
std::vector<Vec4i> detect_bounding_lines_iterate_scale(const Mat img_src) ;
Rect rect_within_image(Mat img, Mat corrected_img, std::vector<Point2f> detected_corners, std::vector<Point2f> dst_corners) ;
std::pair<Vec4i, Vec4i> leftmost_rightmost_lines(std::vector<Vec4i> lines) ;
std::pair<Vec4i, Vec4i> topmost_bottommost_lines(std::vector<Vec4i> lines) ;

#ifdef USE_EXIV2
bool copy_exif(std::string src_path, std::string dest_path) ;
#endif

//std::vector<Vec4i> button_strip_lines(Mat src_gray) ;
//std::vector<int> histogram(std::vector<int> vals) ;
//void draw_drink_separator_lines(Mat img, std::vector<int> runlengths, Point offset) ;
//std::vector<int> get_drink_columns(Mat img) ;



// global filenames
//char *filename ;
const char *dest_dir ;
const char *DEFAULT_DEST_DIR = "corrected" ;

const std::string path_separator = std::string("/") ;


// void draw_line(Mat image, Vec4i l, Scalar color, int width) {
//     line(image, Point(l[0], l[1]), Point(l[2], l[3]), color, width, CV_8S);
//     //line(canny, Point(0, top_edge), Point(src.cols, top_edge), Scalar(255, 255, 0), 3, CV_AA);
// }

/*
  static void onMouse(int event, int x, int y, int, void*) {
  if (event == EVENT_LBUTTONDOWN) {
  std::cout << "Mouse clicked at " << x << ":" << y << std::endl ;
  }
  }
*/

void help() {
	std::cout << "fixperspective" << std::endl ;
	std::cout << "  -b : batch mode (no display)" << std::endl ;
	std::cout << "  -c : clip image to transform borders" << std::endl ;
	std::cout << "  -d dir : batch mode target directory" << std::endl ;
	std::cout << "  -n : test mode; do not write file" << std::endl ;
	std::cout << "  -v : verbose messages" << std::endl ;
	exit(0) ;
}

void test() {
}

const char *window_name_canny = "Canny Eges" ;
const char *window_name_src = "Original" ;

bool cmdopt_batch = false ;
bool cmdopt_clip = false ;
bool cmdopt_verbose = false ;
bool cmdopt_nowrite = false ;


int main(int argc, char **argv) {
	//int flag_d = 0 ;
	//int flag_b = 0 ;
	char *cvalue = NULL ;
	int c ;
	// bool is_output_directory_exists = true ;
	
	//test() ; exit(0);

	dest_dir = DEFAULT_DEST_DIR ;
	
	const char *opts =  "bcd:nv";

	while((c = getopt(argc, argv, opts)) != -1) {
		switch(c) {
			case 'b':
				cmdopt_batch = true ;
				break;
			case 'c':
				cmdopt_clip = true ;
				break;
			case 'd':
				cvalue = optarg ;
				dest_dir = cvalue ;
				break ;
			case 'n':
				cmdopt_nowrite = true ;
				break;
			case 'v' ://verbose
				cmdopt_verbose = true ;
				std::cout << "Verbose mode" << std::endl ;
				break ;
		}
	}

	int num_file_args = argc - optind - 1 ;

	if (num_file_args < 1) {
		std::cout << "No input files" << std::endl ; 
		help() ;
	}
	
	for (int idx = optind ; idx < argc ; idx++) {
		char *filename ;
		filename = argv[idx] ;
		
		std::stringstream dest_path ;
		
		if(!cmdopt_nowrite) {		
			dest_path << dest_dir << "/" << basename(filename) ;
		}
	
	process_file(filename, dest_path.str().c_str()) ;
	}
}

int process_file(char *filename, const char *dest_file) {
	// std::stringstream ss_filename ;

	// const auto filenamestr = std::string(basename(filename)) ;
	// const auto lastindex = filenamestr.find_last_of(".") ;
	// const auto fnoext = filenamestr.substr(0, lastindex) ;
	// const auto filename_extension = filenamestr.substr(lastindex + 1) ;

	auto src  = imread(filename, 1) ; //color
  
	if(src.empty()) {
		std::cerr << "Input file is empty: " << filename << std::endl ;
		return -1 ;
	} else {
		if(cmdopt_verbose) {
			std::cout << std::endl << "Read image file: " << filename << std::endl ;
		}
	}

	/*
	int left_margin = src.cols / 4 ;
	int right_margin = src.cols - left_margin ;
	int top_margin = src.rows / 6 ;
	int bottom_margin = src.rows - top_margin ;

	display_margins(src, Rect(left_margin, top_margin, src.cols - left_margin - left_margin, src.rows - top_margin - top_margin)) ;
	imshow("Margins" , scale_for_display(src)) ;
	waitKey() ;
	return -1 ;
	*/

  //convert to value or hue channel?
  //hue generally gives better results, but is weak if the machine is white.
	Mat img_val, img_hue ;

	cvtColor(src, img_val, COLOR_BGR2GRAY) ;
	cvtColor(src, img_hue, COLOR_BGR2HSV) ;	//we will overwrite with just the hue channel

	std::vector<Mat> hsv_planes;

	split(img_hue, hsv_planes );
	img_hue = hsv_planes[0] ;

	Mat imgs[] = {
		img_val, 
		img_hue
	} ;

	blur(img_hue, img_hue, Size(3, 3));

	// for(auto const img : imgs ) {

	//Try to get the maximal quadrilateral bounds

	//Mat img_gray_reduced, img_color_reduced ;
	//img_gray_reduced = src_gray.clone() ;
	//img_gray_reduced = Scalar(0, 0, 0) ;




	if(cmdopt_verbose) { std::cout << std::endl << "-------Trying to detect on hue" << std::endl ; }
	auto bounding_lines_hue = detect_bounding_lines_iterate_scale(img_hue) ;

	if(cmdopt_verbose) { std::cout << std::endl << "-------Trying to detect on val" << std::endl ; }
	auto bounding_lines_val = detect_bounding_lines_iterate_scale(img_val) ;
	
	if(bounding_lines_hue.empty() && bounding_lines_val.empty()) {
		std::cout << "No good lines at all!! - bailing." << std::endl ;
		return -1 ;
	}




	Mat transformed_image = transform_perspective(src, bounding_lines_val) ;

	#ifdef USE_GUI
	if(!cmdopt_batch) {
		std::string transformed_window_title = std::string("Corrected on hue") ;
		imshow(transformed_window_title, scale_for_display(transformed_image)) ;

		//convert to color so we can draw in color on them
		cvtColor(img_val, img_val, COLOR_GRAY2BGR) ;
		cvtColor(img_hue, img_hue, COLOR_GRAY2BGR) ;

		std::cout << "Plotting bounds lines" << std::endl ;
		plot_bounds(img_hue, bounding_lines_hue, Scalar(255, 127, 200)) ;	//purple
		plot_bounds(img_val, bounding_lines_val, Scalar(127, 200, 255)) ;	//orange
		imshow("Value (orange)", scale_for_display(img_val)) ;
		imshow("Hue (purple)" , scale_for_display(img_hue)) ;
		waitKey() ;
	}
	#endif

	//TODO: move this out
	if(!cmdopt_nowrite) {
		if(cmdopt_verbose) {
			std::cout << "Writing to:[" << dest_file << "]" << std::endl ;
		}
		try {
			//imwrite(dest_file, transformed_image) ;
			#ifdef USE_EXIV2
			copy_exif(filename, dest_file) ;
			#endif
		} catch (std::runtime_error& ex) {
		fprintf(stderr, "Could not write output file: %s\n", ex.what());
			return 1;
		}
	}



	return 0 ;
}


/**
 * @brief Detect the maximum bounding significant Hough lines
 * 
 * @param src Source image
 * @param hough_threshold For HoughLinesP
 * @return std::vector<Vec4i> 
 */
std::vector<Vec4i> detect_bounding_lines(Mat src, int hough_threshold) {
	std::vector<Vec4i> lines, vertical_lines, horizontal_lines ;
	std::vector<Vec4i> boundary_lines ;

	//TODO: change return value to array<Vec4i, 4>
	//min length of Hough lines based on image size
	const int min_vertical_length = src.rows / 4 ; //6
	const int min_horizontal_length = src.cols / 6 ; //10
	const int min_length = min(min_vertical_length, min_horizontal_length) ;
	const int max_gap = 70 ; //100

	//canny parameters optimized for jihanki images?
	// last three are thresh1, thresh2, sobel_aperture

	Mat cann ;

	Canny(src, cann, 30, 450, 3) ;
	HoughLinesP(cann, lines, 1, CV_PI/180, hough_threshold * 3, min_length, max_gap) ;

	/*
	Let's completely rewrite this section.
	1. Divide the image into four outside strips (left, right, top bottom) from the start. 
	No point in looking for edges and lines that will be excluded anyway
	2. Combine all the detections - scaled, and channels (hue, values) - and then select the best
	3. If the number of detected lines is not too great, maybe look for collinears and join them. 
	*/

	//remove diagonal lines
	auto diag_iter = std::remove_if(lines.begin(), lines.end(),
		[](Vec4i lin){ return normalized_slope(lin) > 0.1 ; }) ;
	lines.erase(diag_iter, lines.end()) ;

	//collect the vertical lines from the ortho lines
	std::copy_if(lines.begin(), lines.end(), back_inserter(vertical_lines),
		[](const Vec4i lin) { return abs(lin[0] - lin[2]) < abs(lin[1] - lin[3]) ; }) ;
	std::cout << "Found " << vertical_lines.size() << " vertical lines" << std::endl ;

	//collect the horizontal lines from the ortho lines
	std::copy_if(lines.begin(), lines.end(), back_inserter(horizontal_lines),
		[](const Vec4i lin) { return abs(lin[0] - lin[2]) > abs(lin[1] - lin[3]) ; }) ;
	std::cout << "Found " << horizontal_lines.size() << " horizontal lines" << std::endl ;

	//Remove vertical lines near the center. We are aiming for the machine cabinet edges or the display window edges
	//Some machines have a greater inset on the right side to the display window, so we may want to add a bit to the right margin.
	int left_margin = src.cols / 4 ;
	int right_margin = src.cols - left_margin ;
	std::cout << "removing lines near center" << std::endl ;

	auto vert_center_iter = std::remove_if(vertical_lines.begin(), vertical_lines.end(),
		[left_margin, right_margin](const Vec4i lin){ return mid_x(lin) > left_margin && mid_x(lin) < right_margin ; }) ;
	vertical_lines.erase(vert_center_iter, vertical_lines.end());

	int center_col = src.cols / 2 ;
	std::vector<Vec4i> left_lines, right_lines ;

	// std::cout << "collecting left and right lines" << std::endl ;
	
	std::copy_if(vertical_lines.begin(), vertical_lines.end(), std::back_inserter(left_lines),
		[center_col](const Vec4i lin){ return mid_x(lin) < center_col ; }) ;
	
	if(left_lines.empty()) {
		std::cout << "No lines in left side." << std::endl ;
		// return boundary_lines ;
	}

	std::copy_if(vertical_lines.begin(), vertical_lines.end(), std::back_inserter(right_lines),
		[center_col](const Vec4i lin){ return mid_x(lin) > center_col ; }) ;

	if(right_lines.empty()) {
		std::cout << "No lines in right side." << std::endl ;
		// return boundary_lines ;
	}

	//Remove horizontal lines near the center. 
	int top_margin = src.rows / 6 ;
	int bottom_margin = src.rows - top_margin ;

	auto horiz_center_iter = std::remove_if(horizontal_lines.begin(), horizontal_lines.end(),
		[top_margin, bottom_margin](const Vec4i lin){ return mid_y(lin) > top_margin && mid_y(lin) < bottom_margin ; }) ;
	horizontal_lines.erase(horiz_center_iter, horizontal_lines.end());

	int center_row = src.rows / 2 ;
	std::vector<Vec4i> top_lines, bottom_lines ;

	// std::cout << "collecting top and bottom" << std::endl ;

	std::copy_if(horizontal_lines.begin(), horizontal_lines.end(), std::back_inserter(top_lines),
		[center_row](const Vec4i lin){ return mid_y(lin) < center_row ; }) ;

	if(top_lines.empty()) { 
		std::cout << "No lines in top side." << std::endl ;
	}

	std::copy_if(horizontal_lines.begin(), horizontal_lines.end(), std::back_inserter(bottom_lines),
		[center_row](const Vec4i lin){ return mid_y(lin) > center_row ; }) ;

	if(bottom_lines.empty()) {
		std::cout << "No lines in bottom side." << std::endl ;
		// return boundary_lines ;
	}

	//if any of the four line collections are empty, then bail


	/* There are two ways we may want to choose edges:
	1. The outermost regardless of length
	2. The longest in each half
	*/

	cv::Vec4i left_edge_line, top_edge_line, right_edge_line, bottom_edge_line ;

	if(left_lines.size() > 0) {
		auto left_edge_iter = std::min_element(left_lines.begin(), left_lines.end(), [](Vec4i l1, Vec4i l2){ return (mid_x(l1) < mid_x(l2)) ; }) ;
		left_edge_line = left_lines.at(std::distance(left_lines.begin(), left_edge_iter)) ;
		std::cout << "leftmost edge from " << left_lines.size() << ": " << left_edge_line << std::endl ;
	}

	if(right_lines.size() > 0) {
		auto right_edge_iter = std::max_element(right_lines.begin(), right_lines.end(), [](Vec4i l1, Vec4i l2){ return (mid_x(l1) < mid_x(l2)) ; }) ;
		right_edge_line = right_lines.at(std::distance(right_lines.begin(), right_edge_iter)) ;
		std::cout << "rightmost edge from " << right_lines.size() << ": " << right_edge_line << std::endl ;
	}

	if(top_lines.size() > 0) {
		auto top_edge_iter = std::min_element(top_lines.begin(), top_lines.end(), [](Vec4i l1, Vec4i l2){ return (mid_y(l1) < mid_y(l2)) ; }) ;
		top_edge_line = top_lines.at(std::distance(top_lines.begin(), top_edge_iter)) ;
		std::cout << "topmost edge from " << top_lines.size() << ": " << top_edge_line << std::endl ;
	}
	
	if(bottom_lines.size() > 0) {
		auto bottom_edge_iter = std::max_element(bottom_lines.begin(), bottom_lines.end(), [](Vec4i l1, Vec4i l2){ return (mid_y(l1) < mid_y(l2)) ; }) ;
		bottom_edge_line = bottom_lines.at(std::distance(bottom_lines.begin(), bottom_edge_iter)) ;
		std::cout << "bottommost edge " << bottom_lines.size() << ": " << bottom_edge_line << std::endl ;
	}

	//auto top_edge_line    = horizontal_lines[std::distance(horizontal_lines.begin(), horiz_lines_iter.first)] ;
	//auto bottom_edge_line = horizontal_lines[std::distance(horizontal_lines.begin(), horiz_lines_iter.second)] ;

	bool is_detect_longest_edge = false ;
	is_detect_longest_edge = true ;

	if(is_detect_longest_edge) {
		std::cout << "longest line in left area from " << left_lines.size() << ": " << left_edge_line << std::endl ;
		if(left_lines.size() > 0) {
			auto left_iter = std::max_element(left_lines.begin(), left_lines.end(), [](Vec4i l1, Vec4i l2){ 
				return (len_sq(l1) < len_sq(l2)) ; 
			}) ;
			left_edge_line = left_lines.at(std::distance(left_lines.begin(), left_iter)) ;
		}

		std::cout << "longest line in right area from " << right_lines.size() << std::endl ;
		if(right_lines.size() > 0) {
			auto right_iter = std::max_element(right_lines.begin(), right_lines.end(), [](Vec4i l1, Vec4i l2){ 
				return (len_sq(l1) < len_sq(l2)) ; 
			}) ;
			right_edge_line = right_lines.at(std::distance(right_lines.begin(), right_iter)) ;
			std::cout << right_edge_line << std::endl ;
		}

		std::cout << "longest line in top area from " << top_lines.size() << std::endl ;
		if(top_lines.size() > 0) {
			auto top_iter = std::max_element(top_lines.begin(), top_lines.end(), [](Vec4i l1, Vec4i l2){ 
				return (len_sq(l1) < len_sq(l2)) ; 
			}) ;
			top_edge_line = top_lines.at(std::distance(top_lines.begin(), top_iter)) ;
			std::cout << top_edge_line << std::endl ;
		}
		std::cout << "finding longest line in bottom area from " << bottom_lines.size() << std::endl ;
		if(bottom_lines.size() > 0) {		
			auto bottom_iter = std::max_element(bottom_lines.begin(), bottom_lines.end(), [](Vec4i l1, Vec4i l2){ 
				return (len_sq(l1) < len_sq(l2)) ; 
			}) ;
			bottom_edge_line = bottom_lines.at(std::distance(bottom_lines.begin(), bottom_iter)) ;
			std::cout << bottom_edge_line << std::endl ;
		}

	}

	//we could also al
	boundary_lines.push_back(top_edge_line) ;
	boundary_lines.push_back(bottom_edge_line) ;
	boundary_lines.push_back(left_edge_line) ;
	boundary_lines.push_back(right_edge_line) ;

	return boundary_lines ;
}

/**
 * @brief Find the best bounding lines by detecting at successive scales.
 * 
 * @param img_src  Source image
 * @return std::vector<Vec4i>  Vector of four lines. If any are empty, a set of four good bounding lines was not detected.
 */
std::vector<Vec4i> detect_bounding_lines_iterate_scale(const Mat img_src) {
	auto scale = 1 ;
	const auto SCALE_DOWN_MAX = 4 ;
	bool is_bounding_lines_detected = false ;
	Mat img_working = img_src.clone() ;

	std::cout << "detect_bounding_lines_iterate_scale" << std::endl ;

	std::vector<Vec4i> bounding_lines ;
	
	for(scale = 1 ; scale <= SCALE_DOWN_MAX ; scale *= 2) {
		if(cmdopt_verbose) {
			std::cout << "Trying scale " << scale << " for image size: " << img_working.size() << std::endl  ;
		}

		bounding_lines = detect_bounding_lines(img_working) ;

		// A zero line (initial value for a Vec4i) indicates that no good lines were found for that one
		is_bounding_lines_detected = std::none_of(bounding_lines.begin(), bounding_lines.end(),
			[](Vec4i lin) { return is_zero_line(lin) ;}) ;

		if (is_bounding_lines_detected) {
			if(cmdopt_verbose) {
				std::cout << "... bounding lines detected." << std::endl ;
			}
			break ;
		}
		
		if (cmdopt_verbose) {
			std::cout << "... bounding lines NOT detected at scale " << scale << "." << std::endl ;
		}
		
		if(scale < SCALE_DOWN_MAX) {
			pyrDown(img_working, img_working) ;
		}
	}

	if (is_bounding_lines_detected) {
		//scale the lines back up to full size so we can use them on the full size image
		std::transform(bounding_lines.begin(), bounding_lines.end(), bounding_lines.begin(),
			[scale](Vec4i pt) -> Vec4i { return pt * scale ; }) ;

		return bounding_lines ;
	} else {
		Vec4i v ;
		return std::vector<Vec4i> {{v, v, v, v}} ;
	}
}

#ifdef USE_EXIV2
bool copy_exif(std::string src_path, std::string dest_path) {	    
	/*
	Exiv2::ExifData::const_iterator end = exifData.end();
	for (Exiv2::ExifData::const_iterator i = exifData.begin(); i != end; ++i) {
		const char* tn = i->typeName();
		std::cout << tn << " " << i->key() << std::endl ;
	}
	*/	

	//copy exif data from filename to dest_file 
	//Save the exiv2 data before writing
	Exiv2::Image::AutoPtr src_image = Exiv2::ImageFactory::open(src_path);
	assert(src_image.get() != 0);
	src_image->readMetadata();

	Exiv2::ExifData &exifData = src_image->exifData();

	if (exifData.empty()) {
		std::cout << "No EXIF data in image" << std::endl ;
	}

	//open dest_file for writing exif data
	Exiv2::Image::AutoPtr dest_image = Exiv2::ImageFactory::open(dest_path);
  assert(dest_image.get() != 0);
	 
	dest_image->setExifData(exifData);
	dest_image->writeMetadata();

	return true ;
}
#endif

/**/
Mat transform_perspective(Mat img, Vec4i top_line, Vec4i bottom_line, Vec4i left_line, Vec4i right_line) {
	//  std::cout << "Tranforming perspective" << std::endl ;

	// std::array<Point2f, 4> roi_corners = line_corners(top_line, bottom_line, left_line, right_line) ;
	auto roi_corners = line_corners(top_line, bottom_line, left_line, right_line) ;
	std::vector<Point2f> dst_corners;
	// std::vector<Point2f> roi_corners_v(roi_corners.begin(), roi_corners.end()) ;
	
	/*
	  There are many instances when the lines are legitimately horizontal or vertical,
	  and even sufficiently close to the edges of the frame, but they are within the bounds of
	  of the vending machine's merchandise display window. 
	  If we do a transform, those portions of the display window outside
	  the source quad will be clipped.
	  We can use the quad even though it clips the window, as long as we apply the transformation
	  to a larger portion of the source image, which contains the window.
	  I don't know how to proportionally expand the source quad out
	  while maintaining the same transform. But the destination quad is always a rectangle. So
	  So we place a margin in the destination rectangle.
	  That way, the edges of the source quad will be map to vertical/horizontal lines within
	  the destination.
	  The portion of the source image outside the source quad will appear within the margin.
	  
	  For each edge,we chose the _shorter_ of the two margins so that we don't get off-image black regions with
	  sharp edges which could confuse the next cropper.
	  Or we could fill those in with a key color like magenta
	*/
	
	auto pt_tl = line_intersection(top_line, left_line) ;
	auto pt_br = line_intersection(bottom_line, right_line) ;
	//auto pt_tr = line_intersection(top_line, right_line) ;
	//auto pt_bl = line_intersection(bottom_line, left_line) ;
	
	// float top_left_margin = line_length(top_line_left_intercept, pt_tl) ;
	// float left_margin, top_margin, right_margin, bottom_margin ;
	
	/*
	  The left margin in the
	*/
	//The norm value of a complex number is its squared magnitude,
	//defined as the addition of the square of both its real and its imaginary part (without the imaginary unit).
	//This is the square of abs(x).
	
	
	//the longer of lengths of the the top and bottom lines
	float nm = (float)norm(roi_corners[0] - roi_corners[1]) ;
	
	if(cmdopt_verbose) {
		std::cout << "diff of roi_corners[0] and [1]: " << (roi_corners[0] - roi_corners[1]) << std::endl ;
		std::cout << "norm(diff roi_corners[0] and [1]): " << nm << std::endl ;
	}
	
	float dst_width  = (float)std::max(norm(roi_corners[0] - roi_corners[1]), norm(roi_corners[2] - roi_corners[3]));
	//the longer of lengths of the the left and right lines
	float dst_height = (float)std::max(norm(roi_corners[1] - roi_corners[2]), norm(roi_corners[3] - roi_corners[0]));
	
	// float aspect_ratio = dst_height / dst_width ;
	
	auto left_margin = pt_tl.x ;
	auto top_margin = pt_tl.y ;
	auto right_margin = img.cols - pt_br.x ;
	auto bottom_margin = img.rows - pt_br.y ;
	//  bottom_margin = (aspect_ratio * (left_margin + dst_width + right_margin)) - top_margin - dst_height ;
	/*
	  Bottom margin should be calculated such that
	  (left_margin + dst_width + right_margin) / (top_margin + dst_height + bottom_margin) =
	  dst_width / dst_height
	*/
	
	if(cmdopt_verbose) { 
		std::cout << "left margin: " << left_margin << std::endl ;
		std::cout << "top margin: " << top_margin << std::endl ; 
		std::cout << "right margin: " << right_margin << std::endl ; 
		std::cout << "bottom margin: " << bottom_margin << std::endl ; 
	}
	
	//these points define a rectangle in the destination that we want to map roi_corners to.
	//
	dst_corners.push_back(Point2f(left_margin, top_margin)) ;	//TL
	dst_corners.push_back(Point2f(dst_width + left_margin, top_margin)) ; //TR
	dst_corners.push_back(Point2f(dst_width + left_margin, dst_height + top_margin)) ; //BR
	dst_corners.push_back(Point2f(left_margin, dst_height + top_margin)) ;//BL
	
	if(cmdopt_verbose) { 
		std::cout << "dst_corners: " ; 
		for(auto corner: dst_corners) {
			std::cout << corner << " " ;
		}
		std::cout << std::endl ;
	}
	std::cout << "about to get homography" << std::endl ;

	auto hom = cv::findHomography(roi_corners, dst_corners);
	
	std::cout << "Got homography" << std::endl ;
	auto corrected_image_size =	Size(cvRound(dst_corners[2].x + right_margin), cvRound(dst_corners[2].y + bottom_margin));
	Mat corrected_image ;//= img.clone() ;
	
	// do perspective transformation
	warpPerspective(img, corrected_image, hom, corrected_image_size, INTER_LINEAR, BORDER_CONSTANT, Scalar(255, 0, 255));
	std::cout << "returned from warpPerspective" << std::endl ;

	//clip out the blank background areas
	if (cmdopt_clip) {
		Rect clip_frame = rect_within_image(img, corrected_image, roi_corners, dst_corners) ;
		return corrected_image(clip_frame) ;
	}
	
	return corrected_image ;
}

/**
 * @brief Calculate the rectangle which would contain only image areas, no filled background
	This is used to clip out the blank (outside of the image) margins.
 * 
 * @param img Uncorrected source image
 * @param corrected_img 
 * @param detected_corners 
 * @param dst_corners 
 * @return Rect 
 */
Rect rect_within_image(Mat img, Mat corrected_img, std::vector<Point2f> detected_corners, std::vector<Point2f> dst_corners) {
	std::vector<Point2f> corners_v(detected_corners.begin(), detected_corners.end()) ;
	Mat trans = getPerspectiveTransform(corners_v, dst_corners) ;

	std::vector<Point2f> img_frame(4), transformed ;

	img_frame[0] = Point2f(0.0, 0.0) ;
	img_frame[1] = Point2f(img.cols, 0.0) ;
	img_frame[2] = Point2f(img.cols, img.rows) ;
	img_frame[3] = Point2f(0.0, img.rows) ;
	
	perspectiveTransform(img_frame, transformed, trans) ;

	Point2f clip_tl = transformed[0] ;
	Point2f clip_tr = transformed[1] ;
	Point2f clip_br = transformed[2] ;
	Point2f clip_bl = transformed[3] ;

	float clip_left   = max<float>(((clip_tl.x > clip_bl.x) ? clip_tl.x : clip_bl.x), 0) ;
	float clip_right  = min<float>(((clip_tr.x < clip_br.x) ? clip_tr.x : clip_br.x), corrected_img.cols) ;  
	float clip_top    = max<float>(((clip_tl.y > clip_tr.y) ? clip_tl.y : clip_tr.y), 0) ;
	float clip_bottom = min<float>(((clip_bl.y < clip_br.y) ? clip_bl.y : clip_br.y), corrected_img.rows) ;

	Rect clip_frame = Rect(Point(clip_left, clip_top), Point(clip_right, clip_bottom)) ;

	//rectangle(corrected_image, clip_frame, Scalar(255, 255, 0), 10) ;
	if(cmdopt_verbose) {
		std::cout << "Transformed image corners: " << transformed << std::endl ;
		std::cout << "Clip frame:" << clip_frame << std::endl ;
	}
	return clip_frame ;
}


/* sort function for sorting horizontal line by y*/
bool cmp_horizontal_line_y(Vec4i l1, Vec4i l2) {
  int mid1 = (l1[1] + l1[3]) / 2 ;
  int mid2 = (l2[1] + l2[3]) / 2 ;

  return (mid1 < mid2) ;
}

/**
 * @brief Return the four points where the given four lines intersect.
 * 
 * @param top 
 * @param bottom 
 * @param left 
 * @param right
 * 
 * @return std::array<Point2f, 4> TL, TR, BR, BL - clockwise from TL
 */
std::vector<Point2f> line_corners(Vec4i top, Vec4i bottom, Vec4i left, Vec4i right) {
	std::vector<Point2f> points ;

	points.push_back(line_intersection(top, left)) ;
	points.push_back(line_intersection(top, right)) ;
	points.push_back(line_intersection(bottom, right)) ;
	points.push_back(line_intersection(bottom, left)) ;

	return points ;
}

std::pair<Vec4i, Vec4i> leftmost_rightmost_lines(std::vector<Vec4i> lines) {
	//get the min and max (leftmost, rightmost) line by midpoint
	auto lines_iter = std::minmax_element(lines.begin(), lines.end(), 
		[](Vec4i l1, Vec4i l2){	return (mid_x(l1) < mid_x(l2)) ; }) ;
	
	auto leftmost  = lines[std::distance(lines.begin(), lines_iter.first)] ;
	auto rightmost = lines[std::distance(lines.begin(), lines_iter.second)] ;

	return std::make_pair(leftmost, rightmost) ;
}

std::pair<Vec4i, Vec4i> topmost_bottommost_lines(std::vector<Vec4i> lines) {
	//get the min and max (leftmost, rightmost) line by centerpoint
	auto lines_iter = std::minmax_element(lines.begin(), lines.end(), 
		[](Vec4i l1, Vec4i l2){	return (mid_y(l1) < mid_y(l2)) ; }) ;
	
	auto topmost    = lines[std::distance(lines.begin(), lines_iter.first)] ;
	auto bottommost = lines[std::distance(lines.begin(), lines_iter.second)] ;

	return std::make_pair(topmost, bottommost) ;
}

std::pair<Vec4i, Vec4i> leftmost_rightmost_lines_iter(std::vector<Vec4i> lines, int img_cols) {
	Vec4i leftmost_line, rightmost_line ;

	//right edge is a bit farther in, because some machines have a greater inset on the right side
	//which we might want to catch as a line
	//These are the inner limits; we don't want lines farther inside than this
	const int left_limit  = img_cols / 6 ;
	const int right_limit = img_cols - left_limit ;
	//  int right_edge_min = src.cols - left_edge_max - left_edge_max ;

	//initialize the edges to the center
	int leftmost_x   = img_cols / 2 ;
	int rightmost_x  = img_cols / 2 ;

	for(const auto &line : lines) {
		int line_x = mid_x(line) ;
		if(line_x < leftmost_x && line_x < left_limit) {
			leftmost_line = line ;
			leftmost_x    = line_x ;
		}
		if(line_x > rightmost_x && line_x > right_limit) {
			rightmost_line = line ;
			rightmost_x    = line_x ;
		}
	}

	return std::make_pair(leftmost_line, rightmost_line) ;
}

std::pair<Vec4i, Vec4i> topmost_bottommost_lines_iter(std::vector<Vec4i> lines, int img_rows) {
	Vec4i topmost_line, bottommost_line ;

	const int top_limit = img_rows / 6 ; 
	const int bottom_limit = img_rows - top_limit ;

	//initialize all the edges to the center
	int topmost_y    = img_rows / 2 ;
	int bottommost_y = img_rows / 2 ;

	for(const auto &line : lines) {
		int line_y = mid_y(line) ;
		if(line_y < topmost_y && line_y < top_limit) {
			topmost_line = line ;
			topmost_y = line_y ;
		}
		if(line_y > bottommost_y && line_y > bottom_limit) {
			bottommost_line = line ;
			bottommost_y = line_y ;
		}
	}

	return std::make_pair(topmost_line, bottommost_line) ;
}