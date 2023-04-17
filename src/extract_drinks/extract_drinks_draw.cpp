/*
Functions used by the extract_drinks program to draw for user feedback
to check how it is working. These would not be needed for the actual
batch-processing program as run on a server 
 */
#include <iostream>
#include <memory>

#if CV_VERSION_MAJOR >= 4
    #include <opencv4/opencv2/highgui.hpp>
    #include <opencv4/opencv2/imgproc.hpp>
#else
    #include <opencv2/highgui/highgui.hpp>
    #include <opencv2/imgproc/imgproc.hpp>
#endif



using namespace::cv ;

#include "button_strip.hpp"
#include "extract_drinks_draw.hpp"
#include "display.hpp"
#include "threshold.hpp"

void draw_transitions(Mat img, std::shared_ptr<ButtonStrip> strip) ;


void show_result_images(
        Mat src, 
        std::vector<std::shared_ptr<ButtonStrip> > button_strips, 
        std::vector<std::vector<Rect> > drink_rects, 
        std::vector<std::vector<Rect> > price_rects, 
        std::vector<Rect> price_strips, 
        const char *config_string
    ) 
    {
    //draw the slot rectagles on the image
    //prepare an image of the photo in gray but that we can draw on in color
    RNG rng(12435) ; //for colors

    Mat img_detected_features = Mat::zeros(src.size(), CV_8UC3);

    //draw button strips
    for(std::size_t i = 0 ; i < button_strips.size() ; i++) {
        auto strip = button_strips[i] ;
	    const Scalar color = Scalar(rng.uniform(64, 255), rng.uniform(64,255), rng.uniform(64,255));
        
        draw_button_strip(img_detected_features, strip, color, std::to_string(i + 1)) ;
    }
    //draw lines
    for(auto strip : button_strips) {
        draw_markers(img_detected_features, strip->rc(), strip->slot_separators(), strip->slots_height()) ;
    }

    //draw transition strips
    for(auto strip : button_strips) {
        draw_transitions(img_detected_features, strip) ;
    }

    
    Mat img_orig_with_rects ;
    cvtColor(src, img_orig_with_rects, COLOR_BGR2GRAY) ;
    cvtColor(img_orig_with_rects, img_orig_with_rects, COLOR_GRAY2BGR) ;
    
    const int LINE_WIDTH = 4 ;
    for(auto row_rects : drink_rects) {
        for(auto rc : row_rects) {
            rectangle(img_orig_with_rects, rc, Scalar(200, 0, 255), LINE_WIDTH) ;
        }
    }

    for(auto row_rects : price_rects) {
        for(auto rc : row_rects) {
            rectangle(img_orig_with_rects, rc, Scalar(255, 200, 63), LINE_WIDTH) ;
        }
    }

    //draw price_strips
    for(auto rc: price_strips) {
        rectangle(img_orig_with_rects, rc, Scalar(100, 200, 63), LINE_WIDTH) ;
    }

    std::stringstream detected_title ;
    detected_title << "Detected features " << config_string ;
 
    // imshow("Original photo with rects", img_orig_with_rects) ;
    imshow("Original photo with rects", scale_for_display(img_orig_with_rects)) ;
    imshow(detected_title.str(), scale_for_display(img_detected_features)) ;
    // setWindowProperty("Original photo with rects", WND_PROP_AUTOSIZE, WINDOW_NORMAL) ;
    
    waitKey() ;
}

void draw_threshold_crossings(Mat img, Rect rc_target, std::vector<int> runs) {
  //draw markers for the runs, which represent the threshold crossings.
  int x = 0 ;

  const Scalar color = Scalar(0, 127, 255) ;
  Point pt1, pt2 ;

  for(auto ln : runs) {
    pt1 = Point(x + rc_target.x, rc_target.y + 0) ;
    pt2 = Point(x + rc_target.x, rc_target.y + 100) ;
    
    line(img, pt1, pt2, color, 4) ;
    x += ln ;
  }
}

void draw_markers(Mat img, Rect rc_target, std::vector<int> marks, int height) {
    const Scalar color = Scalar(127, 255, 0) ;
    Point pt1, pt2 ;
    
    for(auto mk : marks) {
	pt1 = Point(rc_target.x + mk, rc_target.y + 0) ;
	pt2 = Point(rc_target.x + mk, rc_target.y - height) ;
	
	line(img, pt1, pt2, color, 4) ;
    }
}


void xxxdraw_button_strips(Mat img, const std::vector <std::shared_ptr<ButtonStrip> >strips) {
    //Render the button strips on the full-size images
    RNG rng(12345);//for colors
    
    for( size_t idx_bs = 0; idx_bs < strips.size(); idx_bs++ ) {
        const std::shared_ptr<ButtonStrip> strip = strips[idx_bs] ;
        
        if(strip->rc().size().height == 0) {
            std::cout << "Empty rect; skipping" << std::endl ;
            continue ;
        }
        
        const Scalar color = Scalar(rng.uniform(128, 255), rng.uniform(128,255), rng.uniform(128,255) );
        Mat fill = Mat(strip->rc().size(), CV_8UC3) ;
        
        //todo number
        fill = color ;//Scalar(0, 255, 0) ;//flood the overlay image with the color
        fill.copyTo(img(strip->rc()), strip->image());
        std::cout << "Printing the number" << std::endl ;
        putText(img, "Number", strip->rc().tl() - Point(-10, -10), FONT_HERSHEY_COMPLEX_SMALL, 20, Scalar(255, 255, 220), 5) ;
        // std::string title = "Button Strip Fill #" + std::to_string(idx_bs) ;
    }
}

void draw_button_strip(Mat img, const std::shared_ptr<ButtonStrip> strip, Scalar color, std::string text_label) {
    //Render the button strips on the full-size images
    // std::cout << "draw_button_strip()" << std::endl ;
 
    Mat fill = Mat(strip->rc().size(), CV_8UC3) ;
    
    fill = color ;
    fill.copyTo(img(strip->rc()), strip->image());
    rectangle(img, strip->rc(), Scalar(240, 100, 63), 4) ;
    // std::cout << "Printing the number" << std::endl ;
    putText(img, text_label, strip->rc().tl() - Point(-10, -10), FONT_HERSHEY_COMPLEX_SMALL, 4, Scalar(255, 255, 220), 5) ;

}

void draw_transitions(Mat img, const std::shared_ptr<ButtonStrip> strip) {
    Mat gradation_strip ;
    int y_offset_gradation = strip->rc().height + 10 ;
    auto rc_image = Rect(Point(0, 0), img.size()) ;
    
    resize(strip->img_intensity(), gradation_strip, Size(), 1, 16) ;
	    
    //figure out where to paste the grayscaled strip
    Rect rc_dest ;
    rc_dest = Rect(Point(strip->rc().x, strip->rc().y + y_offset_gradation), gradation_strip.size()) ;
    rc_dest &= rc_image ;//clip to within entire image
    
    if(rc_dest.area() != 0) {
        cvtColor(gradation_strip, gradation_strip, COLOR_GRAY2BGR) ;
        // gradation_strip.copyTo(img(rc_dest)) ;
    }

    //We are doing the same thresholding in run_lengths()
    //We should just pick off the values of img_thresh_line
    Mat threshold_strip ;
    int y_offset_threshold = strip->rc().height + 30 ;
    
    threshold(strip->img_intensity(), threshold_strip, strip->slot_separator_threshold(), 255, THRESH_BINARY) ;
    resize(threshold_strip, threshold_strip, Size(), 1, 8) ;
    
    //figure out where to paste the thresholded strip
    rc_dest = Rect(Point(strip->rc().x, strip->rc().y + y_offset_threshold), threshold_strip.size()) ;
    rc_dest &= rc_image ;
    
    if(rc_dest.area() != 0) {
	Mat roi = threshold_strip(Rect(Point(0, 0), rc_dest.size())) ;		
	cvtColor(roi, roi, COLOR_GRAY2BGR) ;
	roi.copyTo(img(rc_dest)) ;
    }
}

void draw_strip_boundary(Mat& img, const std::shared_ptr<ButtonStrip> strip) {
    auto rc = strip->rc() ;
    auto color = Scalar(200, 45, 200) ;
    auto bottom_edge = boundary(strip->image(), 1) ;

    rectangle(img, rc, Scalar(100, 244, 251), 8) ;
    Point pt_last = Point(0, bottom_edge[0]) + rc.tl() ;

    int left_edge = first_trough(bottom_edge) ;
    line(img, Point(left_edge, -10) + rc.tl(), Point(left_edge, rc.height + 10) + rc.tl(), Scalar(0, 255, 200), 4) ;

    for(int i = 1 ; i < rc.width ; i++) {
        Point pt_this = Point(i, bottom_edge[i]) + rc.tl() ;
        line(img, pt_this, pt_last, color, 16) ;
        pt_last = pt_this ;
    }
}
