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
inline Mat transform_perspective(Mat img, const std::array<Vec4i, 4>lines_tblr) {
  return transform_perspective(img, lines_tblr[0], lines_tblr[1], lines_tblr[2], lines_tblr[3]) ;
}
//Mat crop_orthogonal(Mat src) ;
std::array<Point2f, 4> line_corners(Vec4i top, Vec4i bottom, Vec4i left, Vec4i right) ;
std::array<Vec4i, 4> detect_bounding_lines(Mat src, int hough_threshold = 50) ;
std::array<Vec4i, 4> detect_bounding_lines_iterate_scale(const Mat img_src) ;
Rect rect_within_image(Mat img, Mat corrected_img, std::array<Point2f, 4> detected_corners, std::vector<Point2f> dst_corners) ;
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

	if (argc < 2) {
		help() ;
	}

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

  //convert to value or hue channel?
  //hue generally gives better results, but is weak if the machine is white.
	Mat img_val, img_hue, canny ;

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

	if(cmdopt_verbose) { std::cout << "Trying to detect on hue" << std::endl ; }
	auto bounding_lines_hue = detect_bounding_lines_iterate_scale(img_hue) ;
	if(cmdopt_verbose) { std::cout << "Trying to detect on val" << std::endl ; }
	auto bounding_lines_val = detect_bounding_lines_iterate_scale(img_val) ;

	Mat transformed_image = transform_perspective(src, bounding_lines_hue) ;
	// Mat transformed_image = transform_perspective(src, bounding_lines_val) ;

	#ifdef USE_GUI
	if(!cmdopt_batch) {
		std::string transformed_window_title = std::string("Corrected on hue") ;
		imshow(transformed_window_title, scale_for_display(transformed_image)) ;

		//convert to color so we can draw in color on them
		cvtColor(img_val, img_val, COLOR_GRAY2BGR) ;
		cvtColor(img_hue, img_hue, COLOR_GRAY2BGR) ;
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
 * @brief Find the best bounding lines by detecting at successive scales.
 * 
 * @param img_src Source image
 * @param bounding_lines Vector of lines to receive values
 * @return std::vector<Vec4i> Vector of four lines. If any are empty, a set of four good bounding lines was not detected.
 */
std::array<Vec4i, 4> detect_bounding_lines_iterate_scale(const Mat img_src) {
	auto scale = 1 ;
	const auto SCALE_DOWN_MAX = 4 ;
	bool is_bounding_lines_detected = false ;
	Mat img_working = img_src.clone() ;

	std::cout << "detect_bounding_lines_iterate_scale" << std::endl ;

	std::array<Vec4i, 4> bounding_lines ;
	
	for(scale = 1 ; scale <= SCALE_DOWN_MAX ; scale *= 2) {
		if(cmdopt_verbose) {
			std::cout << "Trying scale " << scale << " for image size: " << img_working.size() ;
		}

		bounding_lines = detect_bounding_lines(img_working) ;

		// A zero line (initial value for a Vec4i) indicates that no good lines were found for that one
		is_bounding_lines_detected = std::none_of(bounding_lines.begin(),
			bounding_lines.end(),
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
		return std::array<Vec4i, 4> {{v, v, v, v}} ;
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
	std::vector<Point2f> roi_corners_v(roi_corners.begin(), roi_corners.end()) ;

	/*
	  for(size_t i = 0 ; i < roi_corners.size() ; i++) {
	  Point2f corner = roi_corners[i] ;
	  circle(img, corner, 4, Scalar(255, 255, 127), 3) ;
	  }
	*/
	
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
	//These are unit-length segments, we just need to calculate the intercection.
	//auto left_edge = Vec4i(0, 0, 0, 1) ;
	//auto right_edge = Vec4i(img.cols, 0, img.cols, 1) ;
	//auto top_edge = Vec4i(0, 0, 1, 0) ;
	//auto bottom_edge = Vec4i(0, img.rows, 1, img.rows) ;
	
	// Point2f top_line_left_intercept  = line_intersection(top_line, left_edge) ;
	// Point2f top_line_right_intercept = line_intersection(top_line, right_edge) ;
	
	//only if not horizontal; unlikely given that the lines are close to ortho
	// Point2f top_line_top_intercept  = line_intersection(top_line, top_edge) ;
	
	// Point2f bottom_line_left_intercept  = line_intersection(bottom_line, left_edge) ;
	// Point2f bottom_line_right_intercept = line_intersection(bottom_line, right_edge) ;
	
	auto pt_tl = line_intersection(top_line, left_line) ;
	auto pt_tr = line_intersection(top_line, right_line) ;
	auto pt_br = line_intersection(bottom_line, right_line) ;
	auto pt_bl = line_intersection(bottom_line, left_line) ;
	
	// float top_left_margin = line_length(top_line_left_intercept, pt_tl) ;
	float left_margin, top_margin, right_margin, bottom_margin ;
	
	/*
	  The left margin in the
	*/
	//The norm value of a complex number is its squared magnitude,
	//defined as the addition of the square of both its real and its imaginary part (without the imaginary unit).
	//This is the square of abs(x).
	
	
	//the longer of lengths of the the top and bottom lines
	
	if(cmdopt_verbose) {
	  std::cout << "diff of roi_corners[0] and [1]: " << (roi_corners[0] - roi_corners[1]) << std::endl ;
	}
	
	float nm = (float)norm(roi_corners[0] - roi_corners[1]) ;
	
	if(cmdopt_verbose) {
	  std::cout << "norm(diff roi_corners[0] and [1]): " << nm << std::endl ;
	}
	
	float dst_width  = (float)std::max(norm(roi_corners[0] - roi_corners[1]), norm(roi_corners[2] - roi_corners[3]));
	//the longer of lengths of the the left and right lines
	float dst_height = (float)std::max(norm(roi_corners[1] - roi_corners[2]), norm(roi_corners[3] - roi_corners[0]));
	
	float aspect_ratio = dst_height / dst_width ;
	
	left_margin = pt_tl.x ;
	top_margin = pt_tl.y ;
	right_margin = img.cols - pt_tr.x ;
	bottom_margin = img.rows - pt_br.y ;
	//  bottom_margin = (aspect_ratio * (left_margin + dst_width + right_margin)) - top_margin - dst_height ;
	/*
	  Bottom margin should be calculated such that
	  (left_margin + dst_width + right_margin) / (top_margin + dst_height + bottom_margin) =
	  dst_width / dst_height
	*/
	
	if(cmdopt_verbose) { std::cout << "left margin: " << left_margin << std::endl ;}
	if(cmdopt_verbose) { std::cout << "top margin: " << top_margin << std::endl ; }
	if(cmdopt_verbose) { std::cout << "right margin: " << right_margin << std::endl ; }
	if(cmdopt_verbose) { std::cout << "bottom margin: " << bottom_margin << std::endl ; }
	
	//these points define a rectangle in the destination that we want to map roi_corners to.
	//
	//TL
	dst_corners[0] = Point2f(left_margin, top_margin) ;
	//TR
	dst_corners[1] = Point2f(dst_width + left_margin, top_margin) ;
	//BR
	dst_corners[2] = Point2f(dst_width + left_margin, dst_height + top_margin) ;
	//BL
	dst_corners[3] = Point2f(left_margin, dst_height + top_margin) ;
	
	if(cmdopt_verbose) { 
		std::cout << "dst_corners: " ; 
		for(size_t i = 0 ; i < dst_corners.size() ; i++) {
			if(cmdopt_verbose) {
				std::cout << dst_corners[i] << " " ;
			}
		}
		std::cout << std::endl ;
	}

	Mat H = findHomography(roi_corners_v, dst_corners);
	
	auto corrected_image_size =
	  Size(cvRound(dst_corners[2].x + right_margin),
		  cvRound(dst_corners[2].y + bottom_margin));
	Mat corrected_image ;//= img.clone() ;
	
	// do perspective transformation
	warpPerspective(img, corrected_image, H, corrected_image_size, INTER_LINEAR, BORDER_CONSTANT, Scalar(255, 0, 255));

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
Rect rect_within_image(Mat img, Mat corrected_img, std::array<Point2f, 4> detected_corners, std::vector<Point2f> dst_corners) {
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

/**
 * @brief Detect the maximum bounding significant hough lines
 * 
 * @param src Source image
 * @param hough_threshold For HoughLinesP
 * @return std::vector<Vec4i> 
 */
std::array<Vec4i, 4> detect_bounding_lines(Mat src, int hough_threshold) {
	std::vector<Vec4i> lines, vertical_lines, horizontal_lines ;

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

	// copy_if(lines.begin(), lines.end(), back_inserter(ortho_lines),
		// [](const Vec4i lin) { return normalized_slope(lin) < 0.1 ; }) ;

	//remove diagonal lines
	std::remove_if(lines.begin(), lines.end(),
		[](Vec4i lin){ return normalized_slope(lin) > 0.1 ; }) ;

	//collect the vertical lines from the ortho lines
	std::copy_if(lines.begin(), lines.end(), back_inserter(vertical_lines),
		[](const Vec4i lin) { return abs(lin[0] - lin[2]) < abs(lin[1] - lin[3]) ; }) ;

	//collect the horizontal lines from the ortho lines
	std::copy_if(lines.begin(), lines.end(), back_inserter(horizontal_lines),
		[](const Vec4i lin) { return abs(lin[0] - lin[2]) > abs(lin[1] - lin[3]) ; }) ;

	//Remove vertical lines near the center. We are aiming for the machine cabinet edges or the display window edges
	//Some machines have a greater inset on the right side to the display window, so we may want to add a bit to the right margin.
	int left_margin = src.cols / 6 ;
	int right_margin = src.cols - left_margin ;

	std::remove_if(vertical_lines.begin(), vertical_lines.end(),
		[left_margin, right_margin](const Vec4i lin){ return mid_x(lin) > left_margin && mid_x(lin) < right_margin ; }) ;


	//Remove horizontal lines near the center. 
	int top_margin = src.rows / 6 ;
	int bottom_margin = src.cols - left_margin ;

	std::remove_if(horizontal_lines.begin(), horizontal_lines.end(),
		[top_margin, bottom_margin](const Vec4i lin){ return mid_y(lin) > top_margin && mid_y(lin) < bottom_margin ; }) ;

	/* There are two ways we may want to choose edges:
	1. The outermost regardless of length
	2. The longest in each half
	*/

	//now choose the outermost
	//all the lines are initialized to [0, 0, 0, 0]
	Vec4i top_edge_line, bottom_edge_line ;

	//initialize all the edges to the center
	int top_edge = src.rows / 2 ;
	int bottom_edge = src.rows / 2 ;

	//  int right_edge_min = src.cols - left_edge_max - left_edge_max ;


	/* doing it functionally:
	*/

	/* std::vector<Vec4i> lines_left_half, lines_right_half, lines_top_half, lines_bottom_half ;

	int width_center = src.cols / 2 ;
	int height_center = src.rows / 2 ;

	//collect all vertical lines in the left half
	std::copy_if(vertical_lines.begin(), vertical_lines.end(), std::back_inserter(lines_left_half), 
		[width_center](Vec4i lin){ return ((lin[0] + lin[2]) / 2) < width_center ; }) ; 

	//collect all vertical lines in the right half
	std::copy_if(vertical_lines.begin(), vertical_lines.end(), std::back_inserter(lines_right_half), 
		[width_center](Vec4i lin){ return ((lin[0] + lin[2]) / 2) >= width_center ; }) ; 
	*/

	std::cout << "gathering outermost" << std::endl ;
	
	//first we should remove lines that are too close to the center

	int inner_limit_left = src.rows / 6 ;
	int inner_limit_right = src.rows - inner_limit_left ;

	std::remove_if(vertical_lines.begin(), vertical_lines.end(),
		[inner_limit_left, inner_limit_right](Vec4i lin){
			return mid_x(lin) > inner_limit_left && mid_x(lin) < inner_limit_right ; 
		}) ;
	auto left_and_right = leftmost_rightmost_lines(vertical_lines) ;

	//get the min and max (topmost, bottommost) line by centerpoint
	auto horiz_lines_iter = std::minmax_element(horizontal_lines.begin(), horizontal_lines.end(), 
		[](Vec4i l1, Vec4i l2){
			return ((l1[1] + l1[3]) / 2) < ((l2[1] + l2[3]) / 2) ;
		}) ;
	
	top_edge_line    = horizontal_lines[std::distance(horizontal_lines.begin(), horiz_lines_iter.first)] ;
	bottom_edge_line = horizontal_lines[std::distance(horizontal_lines.begin(), horiz_lines_iter.second)] ;

	std::array<Vec4i, 4> bound_lines ;
	bound_lines[0] = top_edge_line ;
	bound_lines[1] = bottom_edge_line ;
	bound_lines[2] = left_and_right.first ;
	bound_lines[3] = left_and_right.second ;

	return bound_lines ;
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
 * @return std::array<Point2f, 4> TL, TR, BR, BR - clockwise from TL
 */
std::array<Point2f, 4> line_corners(Vec4i top, Vec4i bottom, Vec4i left, Vec4i right) {
	std::array<Point2f, 4> points ;

	points[0] = line_intersection(top, left) ;
	points[1] = line_intersection(top, right) ;
	points[2] = line_intersection(bottom, right) ;
	points[3] = line_intersection(bottom, left) ;

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
	int left_limit  = img_cols / 6 ;
	int right_limit = img_cols - left_limit ;
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

	//initialize all the edges to the center
	int top_edge    = img_rows / 2 ;
	int bottom_edge = img_rows / 2 ;

	//right edge is a bit farther in, because some machines have a greater inset on the right side
	int top_edge_outer_limit = img_rows / 6 ; 
	int bottom_edge_min = img_cols  / 6 ;
	//  int right_edge_min = src.cols - left_edge_max - left_edge_max ;

	for(const auto &lin : lines) {
		int mid = mid_x(lin) ;
		if(mid < top_edge && mid < top_edge_max) {
			topmost = lin ;
			top_edge = mid ;
		}
		if(mid > bottom_edge && mid > right_edge_min) {
			bottommost = lin ;
			bottom_edge = mid ;
		}
	}

	return std::make_pair(topmost, bottommost) ;
}