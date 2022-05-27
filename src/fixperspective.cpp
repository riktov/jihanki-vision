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
#include "display.hpp"

//#define VERBOSE_MESSAGE(msg) if(cmdopt_verbose) { std::cout << msg ;}

using namespace cv ;

/* local function declarations */
int process_file(char *src_file, const char *dest_file) ;
const bool is_zero_line(Vec4i l) ;
Mat transform_perspective(Mat img, Vec4i top, Vec4i bottom, Vec4i left, Vec4i right) ;
inline Mat transform_perspective(Mat img, const std::vector<Vec4i>lines_tblr) {
  return transform_perspective(img, lines_tblr[0], lines_tblr[1], lines_tblr[2], lines_tblr[3]) ;
}
//Mat crop_orthogonal(Mat src) ;
std::vector<Point2f> line_corners(Vec4i top, Vec4i bottom, Vec4i left, Vec4i right) ;
std::vector<Vec4i> detect_bounding_lines(Mat src, int hough_threshold = 50) ;
Rect rect_within_image(Mat img, Mat corrected_img, std::vector<Point2f> detected_corners, std::vector<Point2f> dst_corners) ;

#ifdef USE_EXIV2
bool copy_exif(std::string src_path, std::string dest_path) ;
#endif

#ifdef USE_GUI
void plot_hough_and_bounds(Mat src_bgr, std::vector<Vec4i> hough_lines, std::vector<Vec4i> bounds_tblr) ;
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
	
	if(cmdopt_verbose) {
		std::cout << filename << std::endl ;
	}
	
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
  //help() ;
	return -1 ;
	} else {
	  if(cmdopt_verbose) {
			std::cout << std::endl << "Read image file: " << filename << std::endl ;
		}
	}

  if(!cmdopt_nowrite) {
	std::cout << "Destination file:[" << dest_file << "]" << std::endl ;
  }


  //convert to value or hue channel?
  //hue generally gives better results, but is weak if the machine is white.
	Mat src_target, src_gray, src_hsv, canny ;

	cvtColor(src, src_gray, COLOR_BGR2GRAY) ;
	cvtColor(src, src_hsv, COLOR_BGR2HSV) ;

	std::vector<Mat> hsv_planes;
	split(src_hsv, hsv_planes );
	Mat src_hue = hsv_planes[0] ;

	Mat imgs[] = {
		src_hue, 
		src_gray
	} ;

	bool is_bounding_lines_detected = false ;
	auto scale = 1 ;
	const auto SCALE_DOWN_MAX = 4 ;

	// for(auto const img : imgs ) {

	//Try to get the maximal quadrilateral bounds

	Mat img_gray_reduced, img_color_reduced ;
	img_gray_reduced = src_gray.clone() ;
	//img_gray_reduced = Scalar(0, 0, 0) ;
	std::vector<Vec4i> bounding_lines ;
	

	for(scale = 1 ; scale <= SCALE_DOWN_MAX ; scale *= 2) {
		if(cmdopt_verbose) {
			std::cout << "Trying scale " << scale << " for image size: " << img_gray_reduced.size() ;
		}

		bounding_lines = detect_bounding_lines(img_gray_reduced) ;

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
			pyrDown(img_gray_reduced, img_gray_reduced) ;
		}
	}
	// }
		
	if(!is_bounding_lines_detected) {
		std::cerr << filename << ": Failed to fix as bounding lines not detected" << std::endl ;
		return -1 ;
	}
	
	//scale the lines back up to full size so we can use them on the full size image
	std::transform(bounding_lines.begin(), bounding_lines.end(), bounding_lines.begin(),
		   [scale](Vec4i pt) -> Vec4i { return pt * scale ; }) ;
	
	Mat unwarped_image = transform_perspective(src, bounding_lines) ;
	
	/* Now detect vertical and horizontal lines

	
	*/
	//try to detect bounds again, not on corrected image
	/*
	  bounding_lines = detect_bounding_lines(unwarped_image) ;
	  
	  is_bounding_lines_detected = std::none_of(bounding_lines.begin(),
	  bounding_lines.end(),
	  [](Vec4i lin) { return is_zero_line(lin) ;}) ;
	  if (is_bounding_lines_detected) {
	  std::cout << "Got bounds of persp-fixed image " << std::endl ;
	  if(!cmdopt_batch) {
	  plot_hough_and_bounds(unwarped_image, bounding_lines, bounding_lines) ;
	  }
	  } else {
	  std::cout << "Bounding lines not detected in pers-fixed image " << std::endl ;
	  }
	*/

	//TODO: move this out

	if(!cmdopt_nowrite) {
		if(cmdopt_verbose) {
			std::cout << "Writing to:[" << dest_file << "]" << std::endl ;
		}
		try {
			imwrite(dest_file, unwarped_image) ;
			#ifdef USE_EXIV2
			copy_exif(filename, dest_file) ;
			#endif
		} catch (std::runtime_error& ex) {
		fprintf(stderr, "Could not write output file: %s\n", ex.what());
			return 1;
		}
	}

	#ifdef USE_GUI
	if(!cmdopt_batch) {
		std::string unwarped_window_title = std::string("Unwarped and Cropped") ;
		imshow(unwarped_window_title, scale_for_display(unwarped_image)) ;

		cvtColor(img_gray_reduced, img_color_reduced, COLOR_GRAY2BGR) ;
		plot_hough_and_bounds(img_color_reduced, bounding_lines, bounding_lines) ;
		imshow("Best at Hough on reduced image", scale_for_display(img_color_reduced)) ;

		waitKey() ;
	}
	#endif

	return 0 ;
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

std::vector<int> run_length_midpoints (std::vector<int> rl) {
	std::vector<int> rlmids ;
	
	rlmids.push_back(0) ;
	
	int total = rl[0] ;
	
	for(size_t i = 1 ; i < rl.size() ; i++) {
	int run = rl[i] ;
	int mid = total + (run / 2) ;
	rlmids.push_back(mid) ;
	total += run ;
	}
	return rlmids ;
}

/* Returns true if l is a zero-line: a degenerate line with all points zero 
	@param l Line, Vec4i
*/
const bool is_zero_line(Vec4i l) {
  return (l[0] == 0 && l[1] == 0 && l[2] == 0 && l[3] == 0) ;
}

/**/
Mat transform_perspective(Mat img, Vec4i top_line, Vec4i bottom_line, Vec4i left_line, Vec4i right_line) {
	//  std::cout << "Tranforming perspective" << std::endl ;

	std::vector<Point2f> roi_corners = line_corners(top_line, bottom_line, left_line, right_line) ;
	std::vector<Point2f> dst_corners(4);
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
	auto left_edge = Vec4i(0, 0, 0, 1) ;
	auto right_edge = Vec4i(img.cols, 0, img.cols, 1) ;
	auto top_edge = Vec4i(0, 0, 1, 0) ;
	// auto bottom_edge = Vec4i(0, img.rows, 1, img.rows) ;
	
	// Point2f top_line_left_intercept  = line_intersection(top_line, left_edge) ;
	// Point2f top_line_right_intercept = line_intersection(top_line, right_edge) ;
	
	//only if not horizontal; unlikely given that the lines are close to ortho
	// Point2f top_line_top_intercept  = line_intersection(top_line, top_edge) ;
	
	// Point2f bottom_line_left_intercept  = line_intersection(bottom_line, left_edge) ;
	// Point2f bottom_line_right_intercept = line_intersection(bottom_line, right_edge) ;
	
	Point2f pt_tl = line_intersection(top_line, left_line) ;
	Point2f pt_tr = line_intersection(top_line, right_line) ;
	Point2f pt_br = line_intersection(bottom_line, right_line) ;
	Point2f pt_bl = line_intersection(bottom_line, left_line) ;
	
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

	Mat H = findHomography(roi_corners, dst_corners);
	
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

/* Calculate the rectangle which would contain only image areas, no filled background
	This is used to clip out the blank (outside of the image) margins.

	@param img Uncorrected source image
	@param corrected_img image
*/
Rect rect_within_image(Mat img, Mat corrected_img, std::vector<Point2f> detected_corners, std::vector<Point2f> dst_corners) {
	Mat trans = getPerspectiveTransform(detected_corners, dst_corners) ;

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
/*
std::vector<Vec4i> orthogonal_lines(std::vector<Vec4i> lines, float max_slope) {
  std::vector<Vec4i> result_lines ;
  for(const auto &lin : lines) {
	int x1 = lin[0] ;
	int y1 = lin[1] ;
	int x2 = lin[2] ;
	int y2 = lin[3] ;

	//std::cout << x1 << std::endl ;

	float slope = normalized_slope(lin) ;

	if(slope < max_slope) {
	result_lines.push_back(lin) ;
	}
  }//end for loop through points

  return result_lines ;
}
*/

/**
	Detect the maximum bounding significant hough lines

	@param src Source image
	@param hough_threshold For HoughLinesP
*/
std::vector<Vec4i> detect_bounding_lines(Mat src, int hough_threshold) {
  std::vector<Vec4i> lines, ortho_lines, vertical_lines, horizontal_lines ;

  //std::cout << "Trying to find bounds of image of size " << src.rows << ":" << src.cols << std::endl ;

  //min length of Hough lines based on image size
  int min_vertical_length = src.rows / 4 ; //6
  int min_horizontal_length = src.cols / 6 ; //10
  int min_length = min(min_vertical_length, min_horizontal_length) ;
  int max_gap = 70 ; //100

  //canny parameters optimized for jihanki images?
  // last three are thresh1, thresh2, sobel_aperture

  Mat cann ;

  Canny(src, cann, 30, 450, 3) ;
  HoughLinesP(cann, lines, 1, CV_PI/180, hough_threshold * 3, min_length, max_gap) ;

  //collect the ortho lines from those lines
  copy_if(lines.begin(), lines.end(), back_inserter(ortho_lines),
	  [](const Vec4i lin) { return normalized_slope(lin) < 0.1 ; }) ;

  //collect the vertical lines from the ortho lines
  copy_if(ortho_lines.begin(), ortho_lines.end(), back_inserter(vertical_lines),
	  [](const Vec4i lin) { return abs(lin[0] - lin[2]) < abs(lin[1] - lin[3]) ; }) ;

  //collect the vertical lines from the ortho lines
  copy_if(ortho_lines.begin(), ortho_lines.end(), back_inserter(horizontal_lines),
	  [](const Vec4i lin) { return abs(lin[0] - lin[2]) > abs(lin[1] - lin[3]) ; }) ;


  //now choose the outermost
  Vec4i left_edge_line, right_edge_line, top_edge_line, bottom_edge_line ;

  int left_edge = src.cols / 2 ;//initially centered
  int right_edge = src.cols / 2 ;
  int top_edge = src.rows / 2 ;
  int bottom_edge = src.rows / 2 ;

  //right edge is a bit farther in, because some machines have a greater inset on the right side
  int left_edge_max = src.cols / 2 ; //6
  int right_edge_min = src.cols - left_edge_max ;
  //  int right_edge_min = src.cols - left_edge_max - left_edge_max ;

  for(const auto &vlin : vertical_lines) {
	int mid_x = (vlin[0] + vlin[2]) / 2 ;
	if(mid_x < left_edge && mid_x < left_edge_max) {
	left_edge_line = vlin ;
	left_edge = mid_x ;
	}
	if(mid_x > right_edge && mid_x > right_edge_min) {
	right_edge_line = vlin ;
	right_edge = mid_x ;
	}
  }

  //  const int top_edge_max = src.rows / 4 ;
  //const int bottom_edge_min = src.rows - top_edge_max ;
  const int top_edge_max = src.rows / 2 ;
  const int bottom_edge_min = src.rows - top_edge_max ;

  for(const auto &hlin : horizontal_lines) {
	int mid_y = (hlin[1] + hlin[3]) / 2 ;
	if(mid_y < top_edge && mid_y < top_edge_max) {
	  top_edge_line = hlin ;
	  top_edge = mid_y ;
	}
	if(mid_y > bottom_edge && mid_y > bottom_edge_min) {
	  bottom_edge_line = hlin ;
	  bottom_edge = mid_y ;
	}
  }

  std::vector<Vec4i> bound_lines(4) ;
  bound_lines[0] = top_edge_line ;
  bound_lines[1] = bottom_edge_line ;
  bound_lines[2] = left_edge_line ;
  bound_lines[3] = right_edge_line ;

  const char *labels[] = {
	"Top", "Bottom", "Left", "Right"
  } ;

  for(unsigned int i = 0 ; i < bound_lines.size() ; i++) {
  //    std::cout << labels[i] << ": " << bound_lines[i] << std::endl  ;
  }
  return bound_lines ;
}

/**
Detect the horizontal lines created by the button strips
The button strip is distinctive because it is the most intense white and exactly horizontal with respect to
the vending machine. Unfortunately, most button strips have a wavy top edge, which makes it difficult
to detect as a line. Here we use standard Hough, not probabilistic, as it is easier to work with angles
for selecting horizontal lines.
 **/

/*
std::vector<Vec4i> button_strip_lines(Mat src_gray) {
  Mat thresh, canny ;
  threshold(src_gray, thresh, 240, 255, THRESH_BINARY) ;
  Canny(thresh, canny, 30, 450, 3) ;

  std::vector<Vec2f> lines ;
  std::vector<Vec4i> button_lines ;

  //min length based on image size
  int min_length = src_gray.cols / 4 ;

  HoughLines(canny, lines, 1, CV_PI/180, 450) ;

  const float theta_threshold = 0.01 * CV_PI ;

  for(const auto lin : lines) {
	float rho = lin[0] ;
	float theta = lin[1] ;

	//    float y_off = std::min(theta, float(CV_PI - theta)) ;
	//theta is measured from the y-axis (vertical)
	float x_axis_displacement = abs((CV_PI/2) - theta) ;

	if(x_axis_displacement < theta_threshold) {
	  Vec4i xyline = rt_to_pt(rho, theta, src_gray.cols) ;
	  button_lines.push_back(xyline) ;
	}
  }
  return button_lines ;
}
*/


/* sort function for sorting horizontal line by y*/
bool cmp_horizontal_line_y(Vec4i l1, Vec4i l2) {
  int mid1 = (l1[1] + l1[3]) / 2 ;
  int mid2 = (l2[1] + l2[3]) / 2 ;

  return (mid1 < mid2) ;
}

void write_drink_rows(Mat src, std::vector<int> separators) {

}


/*
Input line order: top, bottom, left, right
Output point ordr, TL, TR, BR, BL (clockwise from TL)
*/
std::vector<Point2f> line_corners(Vec4i top, Vec4i bottom, Vec4i left, Vec4i right) {
  //std::cout << "line_corners" << std::endl ;
  std::vector<Point2f> points ;

  Point2f p_tl = line_intersection(top, left) ;
  Point2f p_tr = line_intersection(top, right) ;
  Point2f p_bl = line_intersection(bottom, left) ;
  Point2f p_br = line_intersection(bottom, right) ;

  points.push_back(p_tl) ;
  points.push_back(p_tr) ;
  points.push_back(p_br) ;
  points.push_back(p_bl) ;
  //std::cout << "Top left returned by line_intersection()" << p_tl << std::endl ;
  //std::cout << "Top right returned by line_intersection()" << p_tr << std::endl ;
  return points ;
}
