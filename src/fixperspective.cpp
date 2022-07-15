/**
 * @file fixperspective.cpp
 * @author Paul Richter (paul@sagasoda.com)
 * @brief Main module for the fixperspective program.
 * @version 0.1
 * @date 2022-07-15
 * 
 * @copyright Copyright (c) 2022
 * 
 */

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
#include "perspective_lines.hpp"


#ifdef USE_GUI
#include "fixperspective_draw.hpp"
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
Mat find_dense_areas(Mat img_edges) ;
std::pair<Vec4i, Vec4i> best_vertical_lines(std::vector<Vec4i> lines, int gap) ;
std::pair<Vec4i, Vec4i> best_vertical_lines(std::vector<perspective_line> lines, int gap) ;
std::pair<Vec4i, Vec4i> best_horizontal_lines(std::vector<Vec4i> lines, int gap) ;
std::pair<Vec4i, Vec4i> best_horizontal_lines(std::vector<perspective_line> lines, int gap) ;

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

	// int num_file_args = argc - optind - 1 ;

	// if (num_file_args < 1) {
	// 	std::cout << "No input files" << std::endl ; 
	// 	help() ;
	// }
	
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
			std::cout << "Dimensions (cols x rows): " << src.cols << " : " << src.rows << std::endl ;
		}
	}

	//add images to this dict, we can show them at the end,
	//or write them out to files on a headless system
	std::map<std::string, Mat> feedback_image_of ;

	//prepare the images
  //convert to value or hue channel?
  //hue generally gives better results, but is weak if the machine is white.
	
	//gray is a little different from the value channel
	Mat img_hsv, img_gray ;
	cvtColor(src, img_gray, COLOR_BGR2GRAY) ;

	cvtColor(src, img_hsv, COLOR_BGR2HSV) ;	//we will overwrite with just the hue channel

	std::vector<Mat> hsv_planes, bgr_planes;

	split(img_hsv, hsv_planes );
	Mat img_hue = hsv_planes[0] ;
	Mat img_val = hsv_planes[2] ;

	split(src, bgr_planes) ;
	Mat img_blue = bgr_planes[0] ;
	Mat img_green = bgr_planes[1] ;
	Mat img_red = bgr_planes[2] ;

	Mat imgs[] = {
		img_gray,
		img_hue,
		// img_val, 
		// img_blue,
		// img_green,
		// img_red
	} ;

	// blur(img_gray, img_gray, Size(3, 3));
	// blur(img_hue, img_hue, Size(3, 3));

	//a test image for plotting lines
	Mat img_plot ;
	cvtColor(src, img_plot, COLOR_BGR2GRAY) ;
	cvtColor(img_plot, img_plot, COLOR_GRAY2BGR) ;

	//NEW FROM HERE
	int left_margin = src.cols / 4 ;	//4
	int right_margin = src.cols - left_margin ;
	int top_margin = src.rows / 6 ;
	int bottom_margin = src.rows - top_margin ;

	
	

	const char *channel_img_names[] = {
		"gray (C)", "hue(Y)", "val (M)", "blue", "green", "red"
	} ;

	const Scalar colors[] = {
		Scalar(255, 255, 0),	//cyan
		Scalar(255, 0, 255),	//magenta
		Scalar(0, 255, 255),	//yellow
		Scalar(255, 0, 0),
		Scalar(0, 255, 0),
		Scalar(0, 0, 255)
	} ;

	std::vector<Vec4i> left_lines, right_lines, top_lines, bottom_lines ;
	std::vector<Vec4i> lines ;
	
	// imshow("gray" , scale_for_display(img_gray)) ;
	// imshow("val" , scale_for_display(img_val)) ;
	// waitKey() ;
	// return 0 ;
	Mat img_dense_combined ;
	Mat img_edges_combined ;
	std::vector<Mat> channel_images_edges ;

	
	//Get canny corners for each channel, 
	//combine them, and create a dense block mask which can be applied to all channels.
	//Save the canny images for another pass to detect lines on each of them
	for(auto img: imgs) {
		Mat img_edges;
		Canny(img, img_edges, 30, 250, 3) ; //30, 250, 3

		img_edges.copyTo(img_edges_combined, img_edges) ;	//only for denseblock mask
		channel_images_edges.push_back(img_edges) ;
	}

	img_dense_combined = find_dense_areas(img_edges_combined) ;
	bitwise_not(img_dense_combined, img_dense_combined) ;

	feedback_image_of["Dense Block Mask"] = img_dense_combined ;

	//We use multiple channels to get the denseblock mask,
	//but use only the gray channel to detect lines 

	Mat img_edges_gray = channel_images_edges.at(0) ;

	channel_images_edges.clear() ;
	channel_images_edges.push_back(img_edges_gray) ;


	//declare the variables that will accumulate through the channels, below
	std::vector<perspective_line> plines_combined_horizontal, plines_combined_vertical ;
	
	int i = 0 ;
	
	for(auto img : channel_images_edges) {
		Mat img_edges_masked;
		
		img.copyTo(img_edges_masked, img_dense_combined) ;

		lines = detect_bounding_lines(img_edges_masked, 0) ;	//aready removes diagonals

		//now separate into vertical and horizontal
		std::vector<Vec4i> horizontal_lines, vertical_lines ;
		
		std::copy_if(lines.begin(), lines.end(), std::back_inserter(horizontal_lines), [](Vec4i lin){
			return std::abs(lin[0] - lin[2]) > std::abs(lin[1] - lin[3]) ;
		}) ;
		std::cout << "Horizontal: " << horizontal_lines.size() << std::endl ;

		std::copy_if(lines.begin(), lines.end(), std::back_inserter(vertical_lines), [](Vec4i lin){
			return std::abs(lin[0] - lin[2]) < std::abs(lin[1] - lin[3]) ;
		}) ;
		std::cout << "Vertical: " << vertical_lines.size() << std::endl ;


		
		//build vectors of perspective_lines, so we can merge and detect off-kilter lines
		std::vector<perspective_line> horizontal_plines, vertical_plines ;
		
		std::cout << "Filling plines: " << std::endl ;
		fill_perspective_lines(horizontal_plines, horizontal_lines) ;
		fill_perspective_lines(vertical_plines, vertical_lines) ;

		//accumulate the plines
		plines_combined_horizontal.insert(plines_combined_horizontal.end(), horizontal_plines.begin(), horizontal_plines.end()) ;
		plines_combined_vertical.insert(plines_combined_vertical.end(), vertical_plines.begin(), vertical_plines.end()) ;

		
		/*
		If, after the first pass, we get lots of bunched lines (such as the button strips), we might
		block those areas too and do another detection pass.
		Erode will eliminate all single-width lines but leave eroded lines from the bunched lines.
		*/



		//chop up the image into four side strips
		/*
		Mat left_side, right_side, top_side, bottom_side ;
		left_side   = img_masked(Rect(Point2i(0, 0), Point2i(left_margin, img.rows))) ;
		right_side  = img_masked(Rect(Point2i(right_margin, 0), Point2i(img.cols, img.rows))) ;
		top_side    = img_masked(Rect(Point2i(0, 0), Point2i(img.cols, top_margin))) ;
		bottom_side = img_masked(Rect(Point2i(0, bottom_margin), Point2i(img.cols, img.rows))) ;
		*/
		// Mat strips[] = {
		// 	left_side, right_side, top_side, bottom_side
		// } ;

	
		/*

		auto left_detected_lines   = detect_bounding_lines(left_side, 0) ;
		auto right_detected_lines  = detect_bounding_lines(right_side, right_margin) ;
		auto top_detected_lines    = detect_bounding_lines(top_side, 0) ;
		auto bottom_detected_lines = detect_bounding_lines(bottom_side, bottom_margin) ;

		left_lines.insert(left_lines.end(), left_detected_lines.begin(), left_detected_lines.end()) ;
		right_lines.insert(right_lines.end(), right_detected_lines.begin(), right_detected_lines.end()) ;
		top_lines.insert(top_lines.end(), top_detected_lines.begin(), top_detected_lines.end()) ;
		bottom_lines.insert(bottom_lines.end(), bottom_detected_lines.begin(), bottom_detected_lines.end()) ;
		*/
		
		/*
		Canny corners will reveal "dense" areas, which are usually areas with uniformly mottled textures,
		or the interior of the vending machine display window (From all the container graphics). 
		We use these to exclude those areas from line detection. 
		We could also use this in the display strip detection.

		1. Get Canny edges
		2. Find dense areas, distill them to smooth blocks
		3. Remove those blocked areas from the canny image itself.
		4. Canny now has no dense areas, so get Hough lines

		We can also get contours from the dense blocks. 
		We can assume that a large, somewhat rectangular block near the center of the image is the display window.

		If we use this dense block, we don't need to split up the image in to four images.
		
		Also, if there is a dense block that completely borders one wall, we can trim it off 
		(as deep as goes extends the full length of the wall) from the image entirely.
		
		*/

		
		// continue ;
		cvtColor(img, img, COLOR_GRAY2BGR) ;
	
		//the edge image
		cvtColor(img_edges_masked, img_edges_masked, COLOR_GRAY2BGR) ;
		plot_lines(img_edges_masked, horizontal_plines, Scalar(31, 227, 255)) ;
		plot_lines(img_edges_masked, vertical_plines, Scalar(31, 227, 255)) ;

		annotate_plines(img_edges_masked, horizontal_plines) ;
		annotate_plines(img_edges_masked, vertical_plines) ;

		std::string label_edges = "Edges " + std::string(channel_img_names[i]) ;
		imshow(label_edges , scale_for_display(img_edges_masked)) ;
		/*
		auto color = colors[i] ;
		plot_lines(img_plot, left_detected_lines, color) ;
		plot_lines(img_plot, right_detected_lines, color) ;
		plot_lines(img_plot, top_detected_lines, color) ;
		plot_lines(img_plot, bottom_detected_lines, color) ;
		*/
		i++ ;
	}

	if((plines_combined_horizontal.size() < 2) || (plines_combined_vertical.size() < 2)) {
		std::cout << "Not enough horiz and vert lines before merge and converge, bailing" << std::endl ;
		exit(-19) ;
	}

	// std::map<int, std::vector<Vec4i> > hline_bins, vline_bins ;
	std::cout << "Merging collinears" << std::endl ;
	auto merged_horizontal_plines = merge_lines(plines_combined_horizontal, true) ;
	auto merged_vertical_plines = merge_lines(plines_combined_vertical, false) ;

	std::cout << "Horiz plines after merged: " << merged_horizontal_plines.size() << std::endl ;
	std::cout << "Vert plines after merged: " << merged_vertical_plines.size() << std::endl ;
	
	std::cout << "Check convergence from transitions" << std::endl ;
	slope_transitions(merged_horizontal_plines) ;
	slope_transitions(merged_vertical_plines) ;
	

	//Check if we have at least two verticals and horzontals each, or bail
	if((merged_horizontal_plines.size() < 2) || (merged_vertical_plines.size() < 2)) {
		std::cout << "Not enough horiz and vert lines after merge and converge, bailing" << std::endl ;
		exit(-19) ;
	}


	//select the two best verticals and horizontals
	//sort by length, and step through until we find two that are sufficiently separated
	std::cout << "Getting best four bounding lines." << std::endl ;
	auto best_verticals   = best_vertical_lines(plines_combined_vertical, src.cols * 2 / 3) ;
	auto best_horizontals = best_horizontal_lines(plines_combined_horizontal, src.rows * 2 / 3) ;

	cvtColor(img_gray, img_gray, COLOR_GRAY2BGR) ;
	plot_lines(img_gray, best_verticals, Scalar(255, 31, 255)) ;
	plot_lines(img_gray, best_horizontals, Scalar(255, 255, 31)) ;

	cvtColor(img_dense_combined, img_dense_combined, COLOR_GRAY2BGR) ;
	plot_lines(img_dense_combined, lines, Scalar(127, 0, 255)) ;

	// return 0;

	imshow("Dense blocks combined" , scale_for_display(img_dense_combined)) ;
	// imshow("Edges combined" , scale_for_display(img_edges_combined)) ;
	plot_lines(img_plot, lines, Scalar(127, 0, 255)) ;
	
	
	imshow("Gray Original with best 4 bounds" , scale_for_display(img_gray)) ;
	// imshow("Test plot on strips" , scale_for_display(img_plot)) ;

	// waitKey() ;

	/////////////////////////////////// TEST RETURN //////////////////////////////////////
	// return 0 ;


	Mat transformed_image = transform_perspective(src, best_horizontals.first, best_horizontals.second, best_verticals.first, best_verticals.second) ;

	#ifdef USE_GUI
	if(!cmdopt_batch) {
		std::string transformed_window_title = std::string("Corrected Image") ;
		imshow(transformed_window_title, scale_for_display(transformed_image)) ;

		//convert to color so we can draw in color on them
		cvtColor(img_val, img_val, COLOR_GRAY2BGR) ;
		cvtColor(img_hue, img_hue, COLOR_GRAY2BGR) ;

		waitKey() ;
	}
	#endif

	//TODO: move this out
	if(!cmdopt_nowrite) {
		if(cmdopt_verbose) {
			std::cout << "Writing to:[" << dest_file << "]" << std::endl ;
		}
		try {
			imwrite(dest_file, transformed_image) ;
			#ifdef USE_EXIV2
			copy_exif(filename, dest_file) ;
			#endif
		} catch (std::runtime_error& ex) {
		fprintf(stderr, "Could not write output file: %s\n", ex.what());
			return 1;
		}
	}

	std::cout << "Finished processing: " << filename << std::endl ;

	return 0 ;
}

/**
 * @brief Detect good lines on a canny edge image
 * 
 * @param img_cann 
 * @param strip_offset 
 * @return std::vector<Vec4i> 
 */
std::vector<Vec4i> detect_bounding_lines(Mat img_cann, int strip_offset) {
	// Mat cann ;
	std::vector<Vec4i> lines ;
	
	const int hough_threshold = 250 ;	//150, 200
	//with new algorithm, raising max_gap to 100 helps
	const int max_gap = 200 ; //70

	bool is_vertical = img_cann.rows > img_cann.cols ;

	const int min_vertical_length = img_cann.rows / 4 ; //6
	const int min_horizontal_length = img_cann.cols / 6 ; //10

	auto min_length = is_vertical ? min_horizontal_length : min_vertical_length ;

	// HoughLinesP(img_cann, lines, 1, CV_PI/45, hough_threshold, min_length, max_gap) ;
	HoughLinesP(img_cann, lines, 1, CV_PI/180, hough_threshold, min_length, max_gap) ;

	int num_all_detected = -1 ;
	if(cmdopt_verbose) {
		num_all_detected = lines.size() ;
	}
	
	//remove diagonal lines

	auto slant_iter = std::remove_if(lines.begin(), lines.end(),
		[](Vec4i lin){ return slant(lin) > 0.1 ; }) ;
	lines.erase(slant_iter, lines.end()) ;

	if(cmdopt_verbose) {
		int num_filtered = lines.size() ;
		std::cout << num_all_detected - num_filtered << " diagonals removed." << std::endl ;
	}


	return lines ;

	
	

	if (is_vertical) {
		//collect the vertical lines from the ortho lines
		auto iter = std::remove_if(lines.begin(), lines.end(),
			[](const Vec4i lin) { return abs(lin[0] - lin[2]) > abs(lin[1] - lin[3]) ; }) ;
		lines.erase(iter, lines.end()) ;
		// std::cout << "Found " << lines.size() << " lines in " ;

		if(strip_offset != 0) {
			// std::cout << "right side" << std::endl ; 
			std::transform(lines.begin(), lines.end(), lines.begin(),
				/*
				[strip_offset](Vec4i lin) -> Vec4i { 
					return lin + Vec4i(0, 0, strip_offset, 0) ; 
				}) ;
				*/
			
				[strip_offset](Vec4i lin) -> Vec4i { 
					lin[0] = lin[0] + strip_offset ;
					lin[2] = lin[2] + strip_offset ;
					return lin ; 
				}) ;
		} else {
			// std::cout << "left side" << std::endl ; 
		}

	} else {
		//collect the horizontal lines from the ortho lines
		auto iter = std::remove_if(lines.begin(), lines.end(),
			[](const Vec4i lin) { return abs(lin[0] - lin[2]) < abs(lin[1] - lin[3]) ; }) ;
		lines.erase(iter, lines.end()) ;
		// std::cout << "Found " << lines.size() << " lines in " ;
		
		if(strip_offset != 0) {
			std::transform(lines.begin(), lines.end(), lines.begin(),
				[strip_offset](Vec4i lin) -> Vec4i { 
					lin[1] = lin[1] + strip_offset ;
					lin[3] = lin[3] + strip_offset ;
					return lin ; 
				}) ;
			// std::cout << "bottom side" << std::endl ; 
		} else {
			// std::cout << "top side" << std::endl ; 
		}
		
	}
	//filter out boundary lines
	return lines ;
}


/**
 * @brief Detect the maximum bounding significant Hough lines
 * 
 * @param src Source image
 * @param hough_threshold For HoughLinesP
 * @return std::vector<Vec4i> 
 */
std::vector<Vec4i> xxxdetect_bounding_lines(Mat src, int hough_threshold) {
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
			[scale](Vec4i lin) -> Vec4i { return lin * scale ; }) ;

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
	 
	std::cout << "Writing EXIF" << std::endl ;
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

/**
 * @brief Create an image showing dense areas which have lots of edge dots, and merge them into big blotches
 * 
 * @param img_edges Monochrome image with Canny edges
 * @return Mat 
 */
Mat find_dense_areas(Mat img_edges) {
	Mat img_working = img_edges.clone();


	const auto dilation_type = MORPH_ERODE ;
	auto dilation_size = 5 ;	//2
	Mat element = getStructuringElement(dilation_type,
						Size( 2*dilation_size + 1, 2*dilation_size+1 ),
						Point( dilation_size, dilation_size ) );

	dilation_size = 1 ;
	Mat element2 = getStructuringElement(dilation_type,
						Size( 2*dilation_size + 1, 2*dilation_size+1 ),
						Point( dilation_size, dilation_size ) );

	// dilate(img_edges, img_working, element );
	// threshold(img_edges, img_working, 31, 255, THRESH_BINARY) ;
	// erode(img_working, img_working, element );
	// erode(img_working, img_working, element );
	// erode(img_working, img_working, element );


	// bitwise_not(cann, cann) ;
	int pyr_factor = 3;	//4
	// pyrDown(cann, cann, Size(cann.cols / pyr_factor, cann.rows / pyr_factor));
	// pyrUp(cann, cann, Size(cann.cols, cann.rows)) ;

	// auto shrunksize = Size(img_working.cols / pyr_factor, img_working.rows / pyr_factor) ;
	// std::cout << shrunksize << std::endl ;

	// std::cout << "ssize.width: " << img_working.cols << std::endl ;
	// std::cout << "std::abs(dsize.width*2 - ssize.width): " << std::abs(shrunksize.width *2 - img_working.cols) << std::endl ;
	for(int i = 0 ; i <  pyr_factor ; i++) {
		pyrDown(img_working, img_working);
	}

	blur(img_working, img_working, Size(3, 3)) ;
	dilate(img_working, img_working, element );
	erode(img_working, img_working, element);
	dilate(img_working, img_working, element );
	erode(img_working, img_working, element);

	erode(img_working, img_working, element2);
	erode(img_working, img_working, element2);
	//OK results: blur, dilate, threshold, erode
	//erode, blur, dilate, threshold : nothing

	// threshold(img_working, img_working, 31, 255, THRESH_BINARY) ;

	std::cout << "Image size after " << pyr_factor << " pyrDown: " << img_working.size() << std:: endl ;  
	for(int i = 0 ; i <  pyr_factor ; i++) {
		pyrUp(img_working, img_working);
	}

	threshold(img_working, img_working, 31, 255, THRESH_BINARY) ;

/*	GOOD RESULTS:
	down, blur, dilate, erode, up, thresh
	down, blur, dilate, erode+, up, thresh
	down 4, blur 3, dilate 5, erode, dilate, erode, up, thresh 31!!!!

*/
	

	return img_working(Rect(0, 0, img_edges.cols, img_edges.rows)) ;
}

std::pair<Vec4i, Vec4i> best_vertical_lines(std::vector<Vec4i> lines, int min_space) {
	//sort from longest to shortest
	std::sort(lines.begin(), lines.end(), [](Vec4i l1, Vec4i l2){
		return len_sq(l1) > len_sq(l2) ;		
	}) ;

	Vec4i leftmost, rightmost ;
	
	if(mid_x(lines.at(0)) < mid_x(lines.at(1))) {
		leftmost = lines.at(0) ;
		rightmost = lines.at(1) ;
	} else {
		leftmost = lines.at(1) ;
		rightmost = lines.at(0) ;
	}

	int space ;

	for(auto lin : lines) {
		// std::cout << "Line length: " << len_sq(lin) << std::endl ;
		if(mid_x(lin) < mid_x(leftmost)) {
			leftmost = lin ;
		}

		if(mid_x(lin) > mid_x(rightmost)) {
			rightmost = lin ;
		}

		space = mid_x(rightmost) - mid_x(leftmost) ;
		if(space >= min_space) {
			return std::make_pair(leftmost, rightmost) ;
		}
	}
	return std::make_pair(leftmost, rightmost) ;
}

std::pair<Vec4i, Vec4i> best_horizontal_lines(std::vector<Vec4i> lines, int min_space) {
	//sorting from longest to shortest
	std::sort(lines.begin(), lines.end(), [](Vec4i l1, Vec4i l2){
		return len_sq(l1) > len_sq(l2) ;		
	}) ;

	Vec4i topmost, bottommost ;
	
	if(mid_y(lines.at(0)) < mid_y(lines.at(1))) {
		topmost = lines.at(0) ;
		bottommost = lines.at(1) ;
	} else {
		topmost = lines.at(1) ;
		bottommost = lines.at(0) ;
	}

	int space ;

	for(auto lin : lines) {
		// std::cout << "Line length: " << len_sq(lin) << std::endl ;
		if(mid_y(lin) < mid_y(topmost)) {
			topmost = lin ;
		}

		if(mid_y(lin) > mid_y(topmost)) {
			bottommost = lin ;
		}

		space = mid_y(bottommost) - mid_y(topmost) ;
		if(space >= min_space) {
			return std::make_pair(topmost, bottommost) ;
		}
	}
	return std::make_pair(topmost, bottommost) ;
}


std::pair<Vec4i, Vec4i> best_vertical_lines(std::vector<perspective_line> plines, int min_space) {
	std::vector<Vec4i> lines ;
	for(auto plin : plines) {
		lines.push_back(plin.line) ;
	}
	return best_vertical_lines(lines, min_space) ;
}

std::pair<Vec4i, Vec4i> best_horizontal_lines(std::vector<perspective_line> plines, int min_space) {
	std::vector<Vec4i> lines ;
	for(auto plin : plines) {
		lines.push_back(plin.line) ;
	}
	return best_horizontal_lines(lines, min_space) ;
}
