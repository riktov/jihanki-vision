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

#include "../lines.hpp"
#include "perspective_lines.hpp"


#ifdef USE_GUI
#include "fixperspective_draw.hpp"
#include "../display.hpp"
#endif



//#define VERBOSE_MESSAGE(msg) if(cmdopt_verbose) { std::cout << msg ;}

using namespace cv ;



/* local function declarations */
int process_file(char *src_file, const char *dest_file) ;
Mat transform_perspective(Mat img, Vec4i top, Vec4i bottom, Vec4i left, Vec4i right) ;
inline Mat transform_perspective(Mat img, const std::vector<Vec4i>lines_tblr) {
  return transform_perspective(img, lines_tblr[0], lines_tblr[1], lines_tblr[2], lines_tblr[3]) ;
}
std::vector<Point2f> line_corners(Vec4i top, Vec4i bottom, Vec4i left, Vec4i right) ;
std::vector<Vec4i> detect_bounding_lines(Mat src, int hough_threshold = 50) ;
std::vector<Vec4i> detect_bounding_lines_iterate_scale(const Mat img_src) ;
Rect rect_within_image(Mat img, Mat corrected_img, std::vector<Point2f> src_quad_pts, std::vector<Point2f> dst_rect_pts) ;
Mat find_dense_areas(Mat img_edges) ;
std::pair<Vec4i, Vec4i> best_vertical_lines(std::vector<Vec4i> lines, int gap) ;
std::pair<Vec4i, Vec4i> best_vertical_lines(std::vector<ortho_line> lines, int gap) ;
std::pair<Vec4i, Vec4i> best_horizontal_lines(std::vector<Vec4i> lines, int gap) ;
std::pair<Vec4i, Vec4i> best_horizontal_lines(std::vector<ortho_line> lines, int gap) ;
Rect trim_dense_edges(Mat src) ;

#ifdef USE_EXIV2
bool copy_exif(std::string src_path, std::string dest_path) ;
#endif


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

	if (optind == argc) {
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
	
		// std::cout << filename << std::endl ;
		process_file(filename, dest_path.str().c_str()) ;
	}
}

int process_file(char *filename, const char *dest_file) {
	// std::stringstream ss_filename ;

	// const auto filenamestr = std::string(basename(filename)) ;
	// const auto lastindex = filenamestr.find_last_of(".") ;
	// const auto fnoext = filenamestr.substr(0, lastindex) ;
	// const auto filename_extension = filenamestr.substr(lastindex + 1) ;
	
	//for marking up and displaying feedback images
	const char *channel_img_names[] = {
		"gray (C)", "hue(Y)", "val (M)", "blue", "green", "red"
	} ;

	/*
	const Scalar colors[] = {
		Scalar(255, 255, 0),	//cyan
		Scalar(255, 0, 255),	//magenta
		Scalar(0, 255, 255),	//yellow
		Scalar(255, 0, 0),
		Scalar(0, 255, 0),
		Scalar(0, 0, 255)
	} ;
	*/

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

	//a test image for plotting lines. We want a gray image on which we can plot colored lines
	Mat img_plot ;
	cvtColor(src, img_plot, COLOR_BGR2GRAY) ;
	cvtColor(img_plot, img_plot, COLOR_GRAY2BGR) ;

	//NEW FROM HERE
	// std::vector<Vec4i> left_lines, right_lines, top_lines, bottom_lines ;
	
	// imshow("gray" , scale_for_display(img_gray)) ;
	// imshow("val" , scale_for_display(img_val)) ;
	// waitKey() ;
	// return 0 ;
	Mat img_dense_combined ;
	Mat img_edges_combined ;

	std::vector<Mat> channel_images_edges ;

	
	//Get canny edges for each channel, 
	//combine them, and create a dense block mask which can be applied to all channels.
	//Save the canny images for another pass to detect lines on each of them
	for(auto img: imgs) {
		Mat img_edges;
		Canny(img, img_edges, 30, 250, 3) ; //30, 250, 3
		channel_images_edges.push_back(img_edges) ;
	}


	for(auto img : channel_images_edges) {
		img.copyTo(img_edges_combined, img) ;	//only for denseblock mask
	}
	img_dense_combined = find_dense_areas(img_edges_combined) ;
	bitwise_not(img_dense_combined, img_dense_combined) ;


	feedback_image_of["Dense Block Mask"] = img_dense_combined ;

	//We use multiple channels to get the denseblock mask,
	//but use only the gray channel to detect lines 

	Mat img_edges_gray = channel_images_edges.at(0) ;

	channel_images_edges.clear() ;
	channel_images_edges.push_back(img_edges_gray) ;


	//line collections that will accumulate through the channels
	std::vector<ortho_line> plines_combined_horizontal, plines_combined_vertical ;
	
	int i = 0 ;
	
	for(auto img : channel_images_edges) {
		Mat img_edges_masked;
		
		//place the dense mask over the corner image to erase the dense parts
		img.copyTo(img_edges_masked, img_dense_combined) ;

		auto lines = detect_bounding_lines(img_edges_masked, 0) ;	//aready removes diagonals

		//Separate into vertical and horizontal
		std::vector<Vec4i> horizontal_lines, vertical_lines ;
		
		std::copy_if(lines.begin(), lines.end(), std::back_inserter(horizontal_lines), [](Vec4i lin){
			return std::abs(lin[0] - lin[2]) > std::abs(lin[1] - lin[3]) ;
		}) ;

		std::copy_if(lines.begin(), lines.end(), std::back_inserter(vertical_lines), [](Vec4i lin){
			return std::abs(lin[0] - lin[2]) < std::abs(lin[1] - lin[3]) ;
		}) ;

		if(cmdopt_verbose) {
			std::cout << "Horizontal: " << horizontal_lines.size() << std::endl ;
			std::cout << "Vertical: " << vertical_lines.size() << std::endl ;
		}
		
		//build vectors of perspective_lines, so we can merge and detect off-kilter lines
		std::vector<ortho_line> horizontal_plines, vertical_plines ;
		
		if(cmdopt_verbose) {
			std::cout << "Filling plines: " << std::endl ;
		}
		for(auto lin : horizontal_lines) { horizontal_plines.push_back(ortho_line(lin)) ; }
		for(auto lin : vertical_lines) { vertical_plines.push_back(ortho_line(lin)) ; }
		// fill_perspective_lines(horizontal_plines, horizontal_lines) ;
		// fill_perspective_lines(vertical_plines, vertical_lines) ;

		//accumulate the plines from this channel image
		plines_combined_horizontal.insert(plines_combined_horizontal.end(), horizontal_plines.begin(), horizontal_plines.end()) ;
		plines_combined_vertical.insert(plines_combined_vertical.end(), vertical_plines.begin(), vertical_plines.end()) ;

		cvtColor(img, img, COLOR_GRAY2BGR) ;
	
		//the edge image for this channel
		cvtColor(img_edges_masked, img_edges_masked, COLOR_GRAY2BGR) ;

		#ifdef USE_GUI
		plot_lines(img_edges_masked, horizontal_plines, Scalar(31, 227, 255)) ;
		plot_lines(img_edges_masked, vertical_plines, Scalar(31, 227, 255)) ;

		annotate_plines(img_edges_masked, horizontal_plines, Scalar(255, 255, 127)) ;
		annotate_plines(img_edges_masked, vertical_plines, Scalar(127, 255, 255)) ;

		std::string label_edges = "Edges " + std::string(channel_img_names[i]) ;
		imshow(label_edges , scale_for_display(img_edges_masked)) ;
		#endif

		i++ ;
	}


	#ifdef USE_GUI
	if(!cmdopt_batch) {
		cvtColor(img_dense_combined, img_dense_combined, COLOR_GRAY2BGR) ;
		plot_lines(img_dense_combined, plines_combined_horizontal, Scalar(127, 0, 255)) ;
		plot_lines(img_dense_combined, plines_combined_vertical, Scalar(127, 0, 255)) ;

		std::string label = "Blocks with unmerged lines, H:" + 
			std::to_string(plines_combined_horizontal.size()) + ", V:" + 
			std::to_string(plines_combined_vertical.size());
		imshow(label , scale_for_display(img_dense_combined)) ;
	}
	#endif

	if((plines_combined_horizontal.size() < 2) || (plines_combined_vertical.size() < 2)) {
		std::cerr << "BAILING: Too few horiz and vert lines before merge and converge" << std::endl ;
		#ifdef USE_GUI
		if(!cmdopt_batch) {
			waitKey() ;
		}
		#endif
		exit(-19) ;
	}


	// std::map<int, std::vector<Vec4i> > hline_bins, vline_bins ;
	//Merge collinears before checking convergence because collinears will not have 
	//an accurate convergence point.

	std::cout << "Merging collinears" << std::endl ;
	std::cout << " - horizontal" << std::endl ;
	auto merged_horizontal_plines = merge_lines(plines_combined_horizontal, img_gray.cols / 2) ;
	std::cout << " - vertical" << std::endl ;
	auto merged_vertical_plines = merge_lines(plines_combined_vertical, img_gray.rows / 2) ;

	std::cout << "Horiz plines after merged: " << merged_horizontal_plines.size() << std::endl ;
	std::cout << "Vert plines after merged: " << merged_vertical_plines.size() << std::endl ;
	
	std::cout << "Filtering out skewed lines" << std::endl ;
	// merged_horizontal_plines = filter_skewed_lines(merged_horizontal_plines) ;
	// merged_vertical_plines = filter_skewed_lines(merged_vertical_plines) ;
	

	//Check if we have at least two verticals and horzontals each, or bail
	if((merged_horizontal_plines.size() < 2) || (merged_vertical_plines.size() < 2)) {
		std::cout << "BAILING: Too few horiz or vert lines after merge and converge" << std::endl ;
		#ifdef USE_GUI
		if(!cmdopt_batch) {
			waitKey() ;
		}
		#endif
		exit(-19) ;
	}


	//select the two best verticals and horizontals
	//sort by length, and step through until we find two that are sufficiently separated
	std::cout << "Getting best four bounding lines." << std::endl ;
	auto best_horizontals = best_horizontal_lines(merged_horizontal_plines, src.rows * 2 / 3) ;
	auto best_verticals   = best_vertical_lines(merged_vertical_plines, src.cols * 2 / 3) ;


	//transform
	Mat transformed_image = transform_perspective(src, best_horizontals.first, best_horizontals.second, best_verticals.first, best_verticals.second) ;

	/*
	std::cout << "Trimming away dense background." << std::endl ;
	Rect rc_dense_trim = trim_dense_edges(transformed_image) ;
	transformed_image = transformed_image(rc_dense_trim) ;
	*/

	#ifdef USE_GUI
	if(!cmdopt_batch) {
		std::string label ;
		Mat img_merged_lines = Mat::zeros(img_gray.size(), CV_8UC3) ;
		plot_lines(img_merged_lines, merged_horizontal_plines, Scalar(255, 127, 255)) ;
		plot_lines(img_merged_lines, merged_vertical_plines, Scalar(255, 255, 127)) ;
		
		auto img_merged_scaled = scale_for_display(img_merged_lines) ;
		float scale = img_merged_scaled.cols * 1.0 / img_merged_lines.cols ;

		annotate_plines(img_merged_scaled, merged_horizontal_plines, Scalar(127, 255, 127), scale) ;
		annotate_plines(img_merged_scaled, merged_vertical_plines, Scalar(127, 255, 255), scale) ;
		

		imshow("Merged (only) lines", img_merged_scaled) ;		

		std::vector<Vec4i> full_lines_horizontal, full_lines_vertical ;
		for(auto lin: plines_combined_horizontal) {
			full_lines_horizontal.push_back(lin.full_line(img_gray.cols)) ;
		}
		/*
		for(auto lin: plines_combined_vertical) {
			full_lines_horizontal.push_back(lin.full_line()) ;
		}
		*/
		// imshow("Edges combined" , scale_for_display(img_edges_combined)) ;
		// plot_lines(img_plot, lines, Scalar(127, 0, 255)) ;
		cvtColor(img_gray, img_gray, COLOR_GRAY2BGR) ;
		plot_lines(img_gray, best_verticals, Scalar(255, 31, 255)) ;
		plot_lines(img_gray, best_horizontals, Scalar(255, 255, 31)) ;
		imshow("Gray Original with best 4 bounds" , scale_for_display(img_gray)) ;

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

	//HoughLinesP(img_cann, lines, 1, CV_PI/45, hough_threshold, min_length, max_gap) ;
	HoughLinesP(img_cann, lines, 1, CV_PI/180, hough_threshold, min_length, max_gap) ;

	int num_all_detected = -1 ;
	if(cmdopt_verbose) {
		num_all_detected = lines.size() ;
	}
	
	//remove diagonal lines

	auto slant_iter = std::remove_if(lines.begin(), lines.end(),
		[](Vec4i lin){ return slant(lin) > 0.1 ; }) ;	// about 10 degrees
	lines.erase(slant_iter, lines.end()) ;

	if(cmdopt_verbose) {
		int num_filtered = lines.size() ;
		std::cout << num_all_detected - num_filtered << " diagonals removed." << std::endl ;
	}

	return lines ;
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

	
	Exiv2::Exifdatum orient = exifData["Exif.Image.Orientation"] ;

	std::cout << "orient:" << orient << std::endl ;

	exifData["Exif.Image.Orientation"] = 0;

	std::cout << "fixed:" << orient << std::endl ;

	/*
	for (auto i = exifData.begin(); i != exifData.end(); ++i) {
		const char* tn = i->typeName();
		std::cout << tn << " " << i->key() << ": " << i->value() << std::endl ;
	}
	*/

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
	  
	*/
	//  std::cout << "Tranforming perspective" << std::endl ;

	// std::array<Point2f, 4> roi_corners = line_corners(top_line, bottom_line, left_line, right_line) ;
	std::cout << "top line:" << top_line << std::endl ;
	std::cout << "bottom_line:" << bottom_line << std::endl ;
	std::cout << "left_line:" << left_line << std::endl ;
	std::cout << "right_line:" << right_line << std::endl ;
	
	std::vector<Point2f> src_quad_points = line_corners(top_line, bottom_line, left_line, right_line) ;
	std::vector<Point2f> dst_rect_points;
	// cv:Rect2f dst_rect ; 
	// std::vector<Point2f> roi_corners_v(roi_corners.begin(), roi_corners.end()) ;
	
	
	auto pt_tl = src_quad_points.at(0) ; //line_intersection(top_line, left_line) ;
	auto pt_br = src_quad_points.at(2) ; //line_intersection(bottom_line, right_line) ;
	//auto pt_tr = line_intersection(top_line, right_line) ;
	//auto pt_bl = line_intersection(bottom_line, left_line) ;
	
	// float top_left_margin = line_length(top_line_left_intercept, pt_tl) ;
	// float left_margin, top_margin, right_margin, bottom_margin ;
	

	/*	
	For each edge,we chose the _shorter_ of the two margins so that we don't get off-image black regions with
	sharp edges which could confuse the next cropper.
	Or we could fill those in with a key color like magenta
	
	The norm value of a complex number is its squared magnitude,
	defined as the addition of the square of both its real and its imaginary part (without the imaginary unit).
	This is the square of abs(x).
	*/

	//the longer of lengths of the the pairs of top/bottom and left/right lines
	float len_src_quad_top    = norm(src_quad_points[0] - src_quad_points[1]) ;
	float len_src_quad_bottom = norm(src_quad_points[2] - src_quad_points[3]) ;
	float len_src_quad_left   = norm(src_quad_points[3] - src_quad_points[0]) ;
	float len_src_quad_right  = norm(src_quad_points[1] - src_quad_points[2]) ;

	float dst_width  = std::max<float>(len_src_quad_top, len_src_quad_bottom);
	float dst_height = std::max<float>(len_src_quad_left, len_src_quad_right);
	
	// float aspect_ratio = dst_height / dst_width ;
	
	//it would be more accurate to take the length along the line instead of the axes. But probably not much difference.
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
	

	//these points define a rectangle in the destination that we want to map the quadrilateral of roi_corners to.
	//it is initially pinned to the source quad at the intersection of the two longest edges.
	Rect2f dst_rect = Rect2f(left_margin, top_margin, dst_width, dst_height) ;
	std::cout << "dst_rect:" << dst_rect << std::endl ;

	std::cout << "Pinned to " ;
	if(len_src_quad_top < len_src_quad_bottom) {
		dst_rect = dst_rect + Point2f(len_src_quad_bottom - len_src_quad_top, 0) ;
		std::cout << "bottom " ;
	} else {
		std::cout << "top " ;
	}

	if(len_src_quad_left < len_src_quad_right) {
		dst_rect = dst_rect + Point2f(0, len_src_quad_right - len_src_quad_left) ;
		std::cout << "right" ;
	} else {
		std::cout << "left" ;
	}
	std::cout << std::endl ;
/*
	dst_rect_points[0] = Point2f(left_margin, top_margin) ;	//TL
	dst_rect_points[1] = Point2f(left_margin + dst_width, top_margin) ; //TR
	dst_rect_points[2] = Point2f(left_margin + dst_width, top_margin + dst_height) ; //BR
	dst_rect_points[3] = Point2f(left_margin, top_margin + dst_height) ;//BL
*/	
	dst_rect_points.push_back(dst_rect.tl()) ;	//TL
	dst_rect_points.push_back(Point2f(dst_rect.x + dst_rect.width, dst_rect.y)) ; //TR
	dst_rect_points.push_back(dst_rect.br()) ; //BR
	dst_rect_points.push_back(Point2f(dst_rect.x, dst_rect.y + dst_rect.height)) ;//BL


	// dst_rect = Rect2f(left_margin, top_margin, dst_width, dst_height) ;

	
	if(cmdopt_verbose) { 
		std::cout << "Source quad margins:" << std::endl ;
		std::cout << "left: " << left_margin << std::endl ;
		std::cout << "top: " << top_margin << std::endl ; 
		std::cout << "right: " << right_margin << std::endl ; 
		std::cout << "bottom: " << bottom_margin << std::endl ; 


		std::cout << "src_quad_points: " << std::endl ;
		for(auto pt : src_quad_points) {
			std::cout << pt << std::endl ;
		}
		std::cout << std::endl ;

		std::cout << "dst_rect_points: " << std::endl ;
		for(auto pt : dst_rect_points) {
			std::cout << pt << std::endl ;
		}
		std::cout << std::endl ;
	}

	auto hom = cv::findHomography(src_quad_points, dst_rect_points);
	
	auto corrected_image_size =	Size(cvRound(dst_width + left_margin + right_margin), cvRound(dst_height + top_margin + bottom_margin));

	Mat warped_image ;//= img.clone() ;
	
	// do perspective transformation
	warpPerspective(img, warped_image, hom, corrected_image_size, INTER_LINEAR, BORDER_CONSTANT, Scalar(255, 0, 255));

	//clip out the blank background areas
	if (cmdopt_clip) {
		Rect clip_frame = rect_within_image(img, warped_image, src_quad_points, dst_rect_points) ;
		return warped_image(clip_frame) ;
	}
	
	return warped_image ;
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
Rect rect_within_image(Mat img, Mat corrected_img, std::vector<Point2f> src_quad_pts, std::vector<Point2f> dst_rect_pts) {
	// std::vector<Point2f> corners_v(src_quad.begin(), src_quad.end()) ;

	Mat trans = getPerspectiveTransform(src_quad_pts, dst_rect_pts) ;

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

/**
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
*/

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

	// std::cout << "Image size after " << pyr_factor << " pyrDown: " << img_working.size() << std:: endl ;  
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


std::pair<Vec4i, Vec4i> best_vertical_lines(std::vector<ortho_line> plines, int min_space) {
	std::vector<Vec4i> lines ;
	for(auto plin : plines) {
		lines.push_back(plin.line) ;
	}
	return best_vertical_lines(lines, min_space) ;
}

std::pair<Vec4i, Vec4i> best_horizontal_lines(std::vector<ortho_line> plines, int min_space) {
	std::vector<Vec4i> lines ;
	for(auto plin : plines) {
		lines.push_back(plin.line) ;
	}
	return best_horizontal_lines(lines, min_space) ;
}

/**
 * @brief Return a rectangle that excludes busy areas on the sides
 * 
 * @param img 
 * @return Rect 
 */
Rect trim_dense_edges(Mat src) {  
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

	Mat img_dense_combined ;
	Mat img_edges_combined ;
	std::vector<Mat> channel_images_edges ;

	
	//Get canny corners for each channel, 
	//combine them, and create a dense block mask which can be applied to all channels.
	//Save the canny images for another pass to detect lines on each of them
	for(auto img: imgs) {
		Mat img_edges;
		Canny(img, img_edges, 30, 100, 3) ; //30, 250, 3
		channel_images_edges.push_back(img_edges) ;
	}


	for(auto img : channel_images_edges) {
		img.copyTo(img_edges_combined, img) ;	//only for denseblock mask
	}
	img_dense_combined = find_dense_areas(img_edges_combined) ;
	bitwise_not(img_dense_combined, img_dense_combined) ;


	Mat img_strip_horiz, img_strip_vert ;
	reduce(img_dense_combined, img_strip_horiz, 0, REDUCE_AVG) ;
	reduce(img_dense_combined, img_strip_vert, 1, REDUCE_AVG) ;
	threshold(img_strip_horiz, img_strip_horiz, 75, 255, THRESH_BINARY) ;
	threshold(img_strip_vert,  img_strip_vert,  75, 255, THRESH_BINARY) ;

	std::vector<int> levels_horiz = img_strip_horiz.row(0) ;
	//63
	std::vector<int> levels_vert = img_strip_vert.col(0) ;

	int left_edge = 0 ;
	while(levels_horiz[left_edge] == 0 && left_edge < src.cols) { left_edge++ ; }

	int top_edge = 0 ;
	while(levels_vert[top_edge] == 0 && top_edge < src.rows) { top_edge++ ; }

	int right_edge = src.cols - 1 ;
	while(levels_horiz[right_edge] == 0 && right_edge > 0 ) { right_edge-- ; }

	int bottom_edge = src.rows - 1 ;
	while(levels_vert[bottom_edge] == 0  && bottom_edge > 0) { bottom_edge-- ; }

	Rect rc_trim = Rect(left_edge, top_edge, right_edge - left_edge, bottom_edge - top_edge) ;

	std::cout << "left edge: " << left_edge << std::endl ;
	std::cout << "top edge: " << top_edge << std::endl ;
	std::cout << "right edge: " << right_edge << std::endl ;
	std::cout << "bottom edge: " << bottom_edge << std::endl ;

	std::cout << "Rect: " << rc_trim << std::endl ;

    resize(img_strip_horiz, img_strip_horiz, img_dense_combined.size()) ;
    resize(img_strip_vert, img_strip_vert, img_dense_combined.size()) ;

	Mat mask ;
	bitwise_not(img_strip_vert, mask) ;
	img_strip_vert.copyTo(img_strip_horiz, mask) ;

	#ifdef USE_GUI
	imshow("Dense block on transformed image", scale_for_display(img_dense_combined)) ;	
	imshow("Horiz strip", scale_for_display(img_strip_horiz)) ;		
	#endif

	return rc_trim ;
	//reduce the 
	
}