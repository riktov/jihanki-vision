/* button_strip.cpp
 * 
 * Implementation of the ButtonStrip class, which represents a strip of selection buttons
 * on a vending machine
 */

/*
Extend the detected top ridge of the buttons downward to grab the full height of the buttons.
The extended rect starts from 1/3 down from the top edge, and is 2.2 as tall  
*/

#include <iostream>
#include <numeric>
#include <memory>

#if CV_VERSION_MAJOR >= 4
#include <opencv4/opencv2/imgproc.hpp>
#else
#include <opencv2/imgproc/imgproc.hpp>
#endif

using namespace cv ;

#include "button_strip.hpp"
#include "run_length.hpp"
#include "threshold.hpp"

extern bool cmdopt_verbose ;

std::shared_ptr<ButtonStrip> strip_from_contours(std::vector<std::vector<Point> > contours);
bool is_valid_strip(std::shared_ptr<ButtonStrip> strip) ;

ButtonStrip::ButtonStrip(cv::Mat image, cv::Rect rc)
	: m_image(image), m_rect(rc)
{
	//extendButtonImage(
}

/* Extend the button image to adjust for tilt
*/
void ButtonStrip::extendButtonImage(const cv::Mat src) {	
	Mat img_strip_filled = this->image().clone();
	//2 runs with different spacing
	fill_bumpy_edge(this->image(), img_strip_filled, 7) ;
	fill_bumpy_edge(this->image(), img_strip_filled, 5) ;

	int width  = this->image().cols ;
	int height = this->image().rows ;
	Size img_size = this->image().size() ;
	
	//trim edges in
	/*
	int margin = width / 10 ;
	Rect rc_trim(strip->rc()) ;
	int left_edge = rc_trim.x + margin ;
	int right_edge = rc_trim.br().x - margin ;

	rc_trim = Rect(Point(left_edge, rc_trim.y), Point(right_edge, rc_trim.br().y)) ;
	trimmed_rects.push_back(rc_trim) ;
	*/
	// img_strip_filled = img_strip_filled(rc_trim) ;
	// imgs_filled_strips.push_back(img_strip_filled) ;
	// pyrUp(img_strip_filled, img_strip_filled) ;
	// continue ;
	/////////////////////////////////////////

	Mat img_scratch, img_stacked_filled ;
	
	//create a new blank image of twice the height by concat
	Mat img_blank = Mat(img_size, this->image().type()) ;
	vconcat(std::vector<Mat>{ img_blank, img_blank}, img_scratch) ;
	//clip to lower half 
	Rect rc_lower = Rect(0, height, width, height) ;

	//smear the wavy image upward
	img_scratch = 0 ;
	Mat img_strip_wavy = this->image().clone() ;

	for(auto y = 0 ; y < height ; y++) {
		auto rc_target = Rect(Point(0, y), img_size);
		bitwise_or(img_strip_wavy, img_scratch(rc_target), img_scratch(rc_target)) ;
	}
	img_strip_wavy = img_scratch(rc_lower).clone() ;//now it is filled to the top edge

	//smear the filled image upward
	img_scratch = 0 ;
	for(auto y = 0 ; y < height ; y++) {
		auto rc_target = Rect(Point(0, y), img_size);
		bitwise_or(img_strip_filled, img_scratch(rc_target), img_scratch(rc_target)) ;
	}
	img_strip_filled = img_scratch(rc_lower).clone() ;//now it is filled to the top edge

	trim_sides_and_expand(img_strip_filled, img_strip_filled, 10) ;

	//invert
	threshold (img_strip_filled, img_strip_filled, 127, 255, THRESH_BINARY_INV);

	Mat img_stacked ;
	vconcat(std::vector<Mat>{ img_strip_filled, img_strip_wavy }, img_stacked) ;

	//overlay the original
	Mat fill = Mat(this->rc().size(), CV_8UC3) ;
	fill = Scalar(127, 105, 0) ;//flood the overlay image with the color

	Rect rc_stacked = Rect(Point(this->rc().x, this->rc().y - this->rc().height), this->rc().br()) ;

	// std::cout << "stacked rect: " << rc_stacked << std::endl ;
	
	img_stacked = remove_solid_rows(img_stacked) ;

	Rect rc_bounds = boundingRect(img_stacked)  ;
	// std::cout << "Bounding rect: " << rc_bounds << std::endl ;
		
	img_stacked = img_stacked(rc_bounds) ;
	rc_stacked = rc_bounds + rc_stacked.tl() ;

	this->setImage(img_stacked) ;
	this->setRect(rc_stacked) ;
}

void ButtonStrip::xxxextendButtonImageDown(const cv::Mat src) {
	//the extended rect
	const float HEIGHT_FACTOR = 1 ; //2.2 ;
	
	auto rc_bottom = Rect(m_rect.tl() + Point(0, m_rect.height / 3), Size(m_rect.width, m_rect.height * HEIGHT_FACTOR)) ;
	//clip to within the bounds of the image
	rc_bottom = rc_bottom & Rect(Point(0, 0), src.size()) ;
	
	Rect rc_combined = m_rect | rc_bottom ;
	
	//create a new, larger image and replace the old one
	Mat img_expanded = Mat::zeros(rc_combined.size(), m_image.type()) ;

	/*
	if(cmdopt_verbose) {	
	std::cout << "The combined rect: " << rc_combined << std::endl ;
	std::cout << "strip image size: " << m_image.size() ;
	std::cout << " img_expanded size: " << img_expanded.size() << std::endl ;
	}
	*/

	Mat img_bottom_thresh ;
	inRange(src(rc_bottom), 127, m_strip_detection_threshold, img_bottom_thresh) ;

	//blit both onto the expanded image
	auto rc_target = Rect(Point(0, 0), m_image.size());
	bitwise_or(m_image, img_expanded(rc_target), img_expanded(rc_target)) ;
	
	rc_target = Rect(Point(0, m_rect.height / 3), img_bottom_thresh.size()) ;
	bitwise_or(img_bottom_thresh, img_expanded(rc_target), img_expanded(rc_target)) ;
	
	m_image = img_expanded ;
	m_rect = rc_combined ;
	//std::cout << __LINE__ << std::endl ;
}

void ButtonStrip::extendButtonImageUp(const Mat src) {   
	m_img_filled = this->image().clone();
	//2 runs with different spacing
	fill_bumpy_edge(this->image(), m_img_filled, 7) ;
	fill_bumpy_edge(this->image(), m_img_filled, 5) ;
}

/*
void ButtonStrip::xxxextendButtonImageUp(const Mat src) {   
	Mat img_extended  = Mat::zeros(Size(m_rect.width, m_rect.height * 2), m_image.type()) ;
	
	//smear the image upward by its own height
	for(auto y = 0 ; y < m_rect.height ; y++) {
		auto rc_target = Rect(Point(0, y), m_image.size());
		bitwise_or(m_image, img_extended(rc_target), img_extended(rc_target)) ;
	}
	m_image = img_extended(Rect(0, m_rect.height, m_rect.width, m_rect.height)) ;
}
*/

Mat ButtonStrip::generateIntensityValuesLine() {
	reduce(image(), m_img_intensity, 0, REDUCE_AVG) ; //0 means vertical, to one horizontal line
	blur(m_img_intensity, m_img_intensity, Size(3, 3)) ;
	return m_img_intensity ;
}

void ButtonStrip::generateSlotsRunLength() {
	Mat img_strip_avg_line ;

	//figure out a suitable threshold by creating an array of the average intensies along th strip's length
	//first generate the single-pixel-height average image
	Mat intensity = generateIntensityValuesLine() ;
	std::vector<int> levels = intensity.row(0) ;

	auto result = std::minmax_element(levels.begin(), levels.end()) ;
	int min_idx = result.first - levels.begin() ;
	int max_idx = result.second - levels.begin() ;
	int max_level = levels[max_idx] ;
	int min_level = levels[min_idx] ;

	m_slot_separator_threshold = (max_level + min_level) / 2  ;
	//save the thresholded image so they can be merged.
	threshold(intensity, m_img_thresh, m_slot_separator_threshold, 255, THRESH_BINARY) ;
	m_runs = run_lengths(levels, m_slot_separator_threshold) ;
}

/**
 * generateDrinkSlots()
 * We are using the mid-range value.
 * Should we use the median instead?
 */
void ButtonStrip::generateDrinkSlots() {
	Mat img_thresh ;

	auto intensity = generateIntensityValuesLine() ;
	std::vector<int> levels = intensity.row(0) ;

	//std::cout << "generateDrinkSlots() - threshold" << std::endl ;
	//figure out a suitable threshold
	auto result = std::minmax_element(levels.begin(), levels.end()) ;
	int min_idx = result.first - levels.begin() ;
	int max_idx = result.second - levels.begin() ;
	int max_level = levels[max_idx] ;
	int min_level = levels[min_idx] ;

	int separator_threshold = (max_level + min_level) / 2  ;

	threshold(intensity, img_thresh, separator_threshold, 255, THRESH_BINARY) ;
	
	std::vector<int> threshed_levels = img_thresh.row(0) ;

	//now we smear it sideways by a few pixels to fill in gaps 
	const int width = 20 ; //5
 
	  //smear to the right by ORing j pixels ahead
	for(size_t i = 0 ; i < threshed_levels.size() - width ; i++) {
		for(auto j = 1 ; j <= width ; j++) {
			threshed_levels[i] |= threshed_levels[i + j] ;
		}
	}
	  //smear to the left by ORing j pixels ahead
	for(size_t i = threshed_levels.size() -1  ; i > width ; i--) {
		for(size_t j = 1 ; j <= width ; j++) {
			threshed_levels[i] |= threshed_levels[i - j] ;
		}
	}

	float rlsd ;

	bool is_starts_high = threshed_levels[0] > 0 ;


	// const char *col = is_starts_high ? "H" : "L" ;
	
	m_runs = binary_run_lengths(threshed_levels) ;
	rlsd = run_length_second_diff(m_runs) ;

	const auto DENOISE_ITERATIONS = 2 ;
	int denoise_runs = 0 ;
	
	for(auto i = 0 ; i < DENOISE_ITERATIONS ; i++) {
		if(rlsd < 5) { break ;}
		
		m_runs = denoise_run_length(m_runs) ;
		rlsd = run_length_second_diff(m_runs) ;
		denoise_runs++ ;
	}
	
	if(cmdopt_verbose) {
	//std::cout << col ;
	//print_vector(m_runs) ;
	//std::cout << std::endl ;
	//	std::cout << denoise_runs << " iterations of denoise." << std::endl ;
	}

	if(m_runs.size() < 1) {
	//std::cout << "no run at all." << std::endl ;
	return ;
	}
	//Get midpoints, of which every other one marks a separator between drinks.
	//The dark areas are buttons, which are centered on the drink, so the lightest
	//areas are the dividers
	std::vector<int> mids = run_length_midpoints(m_runs) ;
	
	//  std::vector<int> separators ;
	
	int last_mark ;
	
	//take every midpoint, which is a separator
	bool odd = is_starts_high ;
	
	for(const auto mark : mids) {
		last_mark = mark ;
		if(odd) {
			m_slot_separators.push_back(mark) ;
		}
		odd = !odd ;
	}
	
	if(odd) {
		m_slot_separators.pop_back() ;
		m_slot_separators.push_back(last_mark) ;
	}
	//  strip->setSlotSeparators(slot_separators) ;
}

/*
  Returns a vector of ButtonStrip objects created from the contours.
contours are assumed to be sorted based by y-position of top of rect
*/
std::vector<std::shared_ptr<ButtonStrip> > xxxmerged_button_strips(std::vector<std::vector<Point> > contours, int threshold) {
	std::vector<std::shared_ptr<ButtonStrip> > strips ;
	//TODO: Merge horizontally joined strips by checking if they almost intersect
	// Check if any corner, displaced outward, is contained by the other
	
	auto is_last_merged = false ;
	Rect rc_prev, rc_this, rc_contours ;
	Mat img_contours ;
	
	if(cmdopt_verbose) {
	std::cout << "merged_button_strips(): " << contours.size() << " contours found." << std::endl ;
	}  
	
	if(contours.size() < 1) {
	if(cmdopt_verbose) {
		std::cout << "No contours." << std::endl ;
	}
	return strips ;
	}
	
	//this is for the case when there is only one contour
	//it will skip the loop, but pick up the single contours as the "last hanging one"
	rc_this = boundingRect(contours[0]) ;
	
	//TODO: fix so we can handle unlimited chains instead of just two.
	for(std::size_t i = 1 ; i < contours.size() ; i++) {
	//    std::cout << "Contour:" << boundingRect(contours[i]) << std::endl ;
	if(cmdopt_verbose) {
		std::cout << "Checking intersection for contour " << i << " ..." ;
	}
	auto prev_contour = contours[i - 1] ;
	auto this_contour = contours[i] ;
	
	rc_prev = boundingRect(prev_contour) ;
	rc_this = boundingRect(this_contour) ;

	/* create a new rect by applying a margin around the previous contour rect.
	   We will then check if this contour overlaps it.
	 */
	Point tld, trd, bld, brd ;
	int margin = rc_prev.width / 10 ;
	
	auto tr = Point(rc_prev.br().x, rc_prev.tl().y) ;
	auto bl = Point(rc_prev.tl().x, rc_prev.br().y) ;
	
	tld = rc_prev.tl() + Point(-1 * margin, -1 * margin) ;
	trd = tr + Point(margin, -1 * margin) ;
	bld = bl + Point(-1 * margin, margin) ;
	brd = rc_prev.br() + Point(margin, margin) ;
	
	auto rcd = Rect(tld, brd) ;

	if((rc_this & rcd).area() != 0) {
		int gap = rc_this.tl().x - rc_prev.br().x ;
		if(cmdopt_verbose) {
			std::cout << "Contour " << i << " at y:" << rc_this.y <<  " is probably a continuation of "  ;
			std::cout << "contour " << i -1  << " at y:" << rc_prev.y ;
			std::cout << ", there is a horizontal gap of " << gap << std::endl ;
		}
		//if gap is negative, then last is to the right of this, and the strip slopes up to the right
		
		/* Filling in the gap:
		   The gap will always be white, so just fill it in the threshold bitmap line
		*/
		/*
		  Figure out the rect to fill in when merging
		  get the last column of the contour on the left, join it with the first column of the contour on the right,
		  then stretch it out to the fill the gap.
		  Prepare a canvas wide for both, render them butted up against each other,
		  then take a two-pixel with vertical strip where the join, then stretch it out to the
		  actual gap betwen them
		*/
		
		rc_contours = rc_prev | rc_this ;
		
		if(cmdopt_verbose) {
			std::cout << "About to create the image for the merged contour." << std::endl ;
		}
		//      img_contours = Mat(rc_contours.size(), CV_8UC3) ;
		img_contours = Mat::zeros(rc_contours.size(), CV_8U) ;
		//      img_contours = 0 ;
		
		//      Mat img_scratch = src_gray.clone() ;
		//img_scratch = Scalar(0, 0, 0) ;
		
		//cvtColor(img_scratch, img_scratch, COLOR_GRAY2BGR) ;
		
		//int ydiff = rc_this.y - rc_prev.y ;
		//int xdiff = abs(rc_this.x - rc_prev.x) ;
		
		/*
		  drawContours(img_contours, contours, (int)(i - 1), Scalar(0, 0, 255), FILLED, LINE_8, noArray(), 0, Point(-1 * rc_contours.tl())) ;
		  drawContours(img_contours, contours, (int)i,       Scalar(0, 255, 0), FILLED, LINE_8, noArray(), 0, Point(-1 * rc_contours.tl())) ;
		*/
		
		drawContours(img_contours, contours, (int)(i - 1), 255, -1, 8, noArray(), 0, Point(-1 * rc_contours.tl())) ;
		drawContours(img_contours, contours, (int)i,       255, -1, 8, noArray(), 0, Point(-1 * rc_contours.tl())) ;
		
		blur(img_contours, img_contours, Size(3, 3));
		
		std::shared_ptr<ButtonStrip> strip(new ButtonStrip(img_contours, rc_contours));
		
		strips.push_back(strip) ;
		is_last_merged = true ;
	} else {
		if(cmdopt_verbose) {
			std::cout << "Contour " << i << " does not intersect with contour " << i - 1 << std::endl ;
		}
		if(!is_last_merged) {
			rc_contours = rc_prev ;
			img_contours = Mat::zeros(rc_contours.size(), CV_8U) ;
			//	std::cout << "Created image for single unmerged contour." << std::endl ;
			//for OpenCV 3.0+, we can use FILLED and LINE_8; for 2.x, use -1 and 8
			drawContours(img_contours, contours, (int)(i - 1), 255, -1, 8, noArray(), 0, Point(-1 * rc_contours.tl())) ;
			//drawContours(img_contours, contours, (int)(i - 1), Scalar(255, 255, 255), -1, 8) ;
			//std::cout << "Successfully drew contour." << std::endl ;
			
			std::shared_ptr<ButtonStrip> strip(new ButtonStrip(img_contours, rc_contours));
			
			strips.push_back(strip) ;
			if(cmdopt_verbose) {
				std::cout << "Successfully pushed ButtonStrip" << std::endl ;
			}
		}
		is_last_merged = false ;
	}
	}
	
	//pick up the last hanging one
	if(!is_last_merged) {
	rc_contours = rc_this ;
	img_contours = Mat::zeros(rc_contours.size(), CV_8U) ;
	//    img_contours = 0 ;
	
	drawContours(img_contours, contours, (int)(contours.size() - 1),
			 255, -1, 8, noArray(), 0, Point(-1 * rc_contours.tl())) ;
	
	std::shared_ptr<ButtonStrip> strip(new ButtonStrip(img_contours, rc_contours));
	strips.push_back(strip) ;
	if(cmdopt_verbose) {
		std::cout << "Successfully pushed last ButtonStrip" << std::endl ;
	}
	}
	
	//calculate the drink container height from row above
	for(size_t i = 1 ; i < strips.size() ; i++) {
	Rect rcTop = strips[i - 1]->rc() ;
	Rect rcBot = strips[i]->rc() ;
	int height = rcBot.y - rcTop.y - (rcTop.height * 2) ;
	
	strips[i]->setSlotsHeight(height) ;
	
	if(i == 1) {
		strips[0]->setSlotsHeight(height * 1.5) ;
	}
	}
	
	if(strips.size() > 2) {
	if(strips[1]->slots_height() > strips[2]->slots_height() * 1.4)
		{
		strips[0]->setSlotsHeight(strips[1]->slots_height()) ;
		}
	}
	
	
	if(strips.size() < 2) {
	strips[0]->setSlotsHeight(400) ;
	}
	
	
	//set the strip detection threshold to use when extending the image
	for(const auto &strip : strips) {
		strip->setStripDetectionThreshold(threshold) ;
	}
	
	return strips ;
}

/*
  Returns a vector of ButtonStrip objects created from the contours.
contours are assumed to be sorted based by y-position of top of rect
*/
std::vector<std::shared_ptr<ButtonStrip> > merged_button_strips(std::vector<std::vector<Point> > contours, int threshold) {
	std::vector<std::shared_ptr<ButtonStrip> > strips ;
	//TODO: Merge horizontally joined strips by checking if they almost intersect
	// Check if any corner, displaced outward, is contained by the other
	
	//bool is_last_merged = false ;
	Rect rc_prev, rc_this ;
	//    Mat img_contours ;
	
	if(contours.size() < 1) {
		return strips ;
	}
	
	std::vector<std::vector<Point> > strip_contours ;
	
	for(std::size_t i = 0 ; i < contours.size() ; i++) {
		auto this_contour = contours[i] ;
		
		if(strip_contours.empty()) {
			strip_contours.insert(strip_contours.begin(), this_contour) ;
			continue ;
		}

		//assert that contours[i -1] is at the head of strip_contours
		rc_prev = boundingRect(contours[i - 1]) ;
		rc_this = boundingRect(this_contour) ;

		/* create a new rect by applying a margin around the previous contour rect.
		We will then check if this contour overlaps it.
		*/
		int margin = rc_prev.width / 10 ;//10 this seems to have no effect
		auto tld = rc_prev.tl() + Point(-1 * margin, -1 * margin) ;
		auto brd = rc_prev.br() + Point(margin, margin) ;	
		auto rcd = Rect(tld, brd) ;

		//std::cout << "Original rect:" << rc_prev << std::endl ;
		//std::cout << "Margined rect:" << rcd << std::endl ; 
		if((rc_this & rcd).area() == 0) {
			//this contour is part of a new strip, so build a new ButtonStrip from what's saved in the contours vector
			auto strip = strip_from_contours(strip_contours);
			if(is_valid_strip(strip)) {
				if(cmdopt_verbose) {
					//std::cout << "Created a strip from contours up to " << i << std::endl ;
				}
				strips.push_back(strip) ;
			}
			strip_contours.clear() ;
		}
		strip_contours.insert(strip_contours.begin(), this_contour) ;
	}

	auto last_strip = strip_from_contours(strip_contours);
	if(is_valid_strip(last_strip)) {
		if(cmdopt_verbose) {
			//std::cout << "Created a strip from last contour." << std::endl ;
		}
		strips.push_back(last_strip) ;
	}
	
	//calculate the slot height from row above
	//A negative slot height indicates that the strip is actually
	//horizontally connected to the one "above"

	for(size_t i = 1 ; i < strips.size() ; i++) {
		auto rcTop = strips[i - 1]->rc() ;
		auto rcBot = strips[i]->rc() ;
		int height = rcBot.y - rcTop.y - (rcTop.height * 2) ;
		
		strips[i]->setSlotsHeight(height) ;
		
		if(i == 1) {
			strips[0]->setSlotsHeight(height * 1.5) ;
		}
	}

	
	if(strips.size() > 2) {
		if(strips[1]->slots_height() > strips[2]->slots_height() * 1.4) {
			strips[0]->setSlotsHeight(strips[1]->slots_height()) ;
		}
	}
	
	
	if(strips.size() < 2) {
		strips[0]->setSlotsHeight(400) ;
	}
	
	
	//set the strip detection threshold to use when extending the image
	for(const auto &strip: strips) {
		strip->setStripDetectionThreshold(threshold) ;
	}
	
	return strips ;
}

std::shared_ptr<ButtonStrip> strip_from_contours(std::vector<std::vector<Point> > contours) {
	std::vector<Rect> strip_rects ;
	
	if(contours.size() < 1) {
	  return nullptr ;
	}

	Rect rc_combined = boundingRect(contours[0]) ;
	
	/* On OpenCV3, zeroRect | boundingRect returns boundingRect, but on 
	   OpenCV2, it returns zeroRect. So don't initialize rc_combined with
	   a zeroRect.
	 */
	for(auto contour: contours) {
	if(cmdopt_verbose) {
		//	    std::cout << "Contour rect: " << boundingRect(contour) << std::endl ;
	}
	rc_combined |= boundingRect(contour) ;
	}

	//make an image of that size
	Mat img_contours = Mat::zeros(rc_combined.size(), CV_8U) ;

	//the contours must be drawn individually, not in a batch (with index -1)
	for(size_t idx = 0 ; idx < contours.size() ; idx++) {
	drawContours(img_contours, contours, idx, 255, -1, 8, noArray(), 0, Point(-1 * rc_combined.tl())) ;
	}
	//drawContours(img_contours, contours, -1, 255, -1, 8, noArray(), 0, Point(-1 * rc_combined.tl())) ;

	blur(img_contours, img_contours, Size(3, 3));
	
	std::shared_ptr<ButtonStrip> strip(new ButtonStrip(img_contours, rc_combined));
	return strip ;
}

bool is_valid_strip(std::shared_ptr<ButtonStrip> strip) {
	Rect rc = strip->rc() ;

	float aspect = rc.height / rc.width ;
	if(aspect > 0.5) {
	return false ;
	}
	return true ;
}
