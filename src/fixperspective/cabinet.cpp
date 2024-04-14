#include <iostream>

#if CV_VERSION_MAJOR >= 4
#include <opencv4/opencv2/core.hpp>
#include <opencv4/opencv2/imgproc.hpp>
#else
#include <opencv2/core/core.hpp>
#endif

#include <lines.hpp>
#include "perspective_lines.hpp"

using namespace cv ;

void test_cabinet() {
    std::cout << "Test cabinet." << std::endl ;
}

/**
 * @brief Return a strip of the image along both sides of the line, corrected for tilt
 * 
 * @param img source image
 * @param lin line
 * @return Mat 
 */
Mat side_strips(Mat img, Vec4i lin) {
    int strip_width = img.rows / 30 ;
    auto rc_img = Rect(Point(0, 0), img.size()) ;

    auto olin = ortho_line(lin) ;
    std::vector<Point2f> src_tri_points;
    std::vector<Point2f> dst_tri_points;
    Mat img_straightened ;

    if(is_horizontal_generally(lin)) {

    } else {
        //vertical lines
        //take the image in rect for both strips on both sides, 
        //skew it, then split it down the middle.
        
        auto width = olin.wc() + 2 * strip_width ;
        auto height = olin.lc() ;

        auto rc = Rect(min_x(olin.line), min_y(olin.line), width, height) & rc_img ;

        auto strips = img(rc).clone() ;

        //The triangle will (obviously) have two points along one edge on the shear axis (separated by 2 * strip_width)
        //and one point on the other parallel edge. This one point will be offsset out (+ / right / down) by 
        //olin.wc(). We then pull that point in (- / left / down) so that it aligns vertically with the other point
        //above/below it.
        //base on angle of the line, we flip this triangle vertically (because a line with a positive slope that is flipped
        //vertically will have a negative slope.)

        Scalar fill_color ;

        if(olin.angle < 0) {
            //image should be sheared to the right
            src_tri_points.push_back(Point2f(rc.tl())) ; //TL
            src_tri_points.push_back(Point2f(rc.tl() + Point(2 * strip_width, 0))) ; //TR, subtract wc from width
            src_tri_points.push_back(Point2f(rc.tl() + Point(olin.wc(), height))) ; //BL, offset to right by wc, pull this back
            fill_color = Scalar(255, 255, 0) ;
        } else {
            //image should be sheared to the left
            src_tri_points.push_back(Point2f(rc.tl() + Point(0, height))) ; //BL
            src_tri_points.push_back(Point2f(rc.tl() + Point(2 * strip_width, height))) ; //BR, subtract wc from width
            src_tri_points.push_back(Point2f(rc.tl() + Point(olin.wc(), 0))) ; //TL, offset to right by wc()
            fill_color = Scalar(255, 0, 255) ;
        }

        //in both cases, (2) is the corner, which we pull in to the left
        dst_tri_points.push_back(src_tri_points.at(0)) ; 
        dst_tri_points.push_back(src_tri_points.at(1)) ;
        dst_tri_points.push_back(src_tri_points.at(2) - Point2f(olin.wc(), 0)) ;

        auto aff = getAffineTransform(src_tri_points, dst_tri_points) ;

        warpAffine(strips, img_straightened, aff, Size(2 * strip_width, height), 1, BORDER_CONSTANT, fill_color) ;
        return img_straightened ;
    }

    return Mat() ;
}

/**
 * @brief Analyze a matrix to see if it is a uniform fill of a single color (hue? value?)
 * 
 * @return float 
 */
float color_uniformity(Mat) {
    //possibilities:
    // simple mean value
    // histogram, mean of top values
    return 0.0 ;

    /*
    auto left_margin = min_x(lin) ;
    auto right_margin = max_x(lin) ;
    auto rc_left = Rect(left_margin - strip_width, min_y(lin), strip_width, max_y(lin) - min_y(lin)) ;
    auto rc_right = Rect(right_margin, min_y(lin), strip_width, max_y(lin) - min_y(lin)) ;

    rc_left &= rc_img ;
    rc_right &= rc_img ;

    auto strip_left  = img(rc_left) ;
    auto strip_right = img(rc_right) ;
    
    //TODO: unskew

    Mat stripcol_left, stripcol_right ;
    reduce(strip_left, stripcol_left, 1, REDUCE_AVG) ;
    reduce(strip_right, stripcol_right, 1, REDUCE_AVG) ;
    // reduce(stripcol_left, stripcol_left, 0, REDUCE_AVG) ;
    // std::cout << "stripcol: " << stripcol_left << std::endl  ;

    auto mean_left = mean(stripcol_left) ; 
    auto mean_right = mean(stripcol_right) ; 

    std::cout << "pix: " << mean_left << std::endl  ;

    
    Mat fill_left = strip_left.clone() ;
    Mat fill_right = strip_right.clone() ;
    
    fill_left.setTo(mean_left) ;
    fill_right.setTo(mean_right) ;
    
    Mat diff_left, diff_right ;

    subtract(strip_left, mean_left, diff_left) ;
    subtract(strip_right, mean_right, diff_right) ;

    return diff_left;
    return fill_left ;
    */

}