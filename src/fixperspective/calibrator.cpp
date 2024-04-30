/**
 * @file calibrator.cpp
 * @author Paul Richter (paul@sagasoda.com)
 * @brief Main module for calibrator program, for interactively tweaking parameters to get the best lines.
 * @version 0.1
 * @date 2024-04-12
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include <iostream>
#include <dirent.h>
#include <chrono>
#include <cstdlib>

#if CV_VERSION_MAJOR >= 4
#include <opencv4/opencv2/highgui.hpp>
#include <opencv4/opencv2/imgproc.hpp>
// #include <opencv4/opencv2/calib3d.hpp>
// #include <opencv4/opencv2/viz/types.hpp>
#else
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
// #include <opencv2/calib3d/calib3d.hpp>
// #include <opencv2/viz/types.hpp>
#endif

#include "detect.hpp" 
#include "fixperspective_draw.hpp"
#include "../display.hpp"

const char *src_dir = "/home/paul/Pictures/dcim/jihanki/" ;
// const char *src_dir = "/home/paul/Pictures/dcim/2021/04/" ;
const char *window_name = "Calibrator" ;

bool cmdopt_verbose = true ;

const int IMAGE_MIN_DIM_TYPICAL = 2500 ;
const float CANNY_THRESH_RATIO = 2.5 ;


using namespace cv ;

Mat g_src, result ;

static void get_lines(Mat img_gray, std::vector<Vec4i> &lines, int canny1, int hough_threshold, int block_algo, Mat &img_mask) {
    Mat img_edges, img_edges_masked ;
    int canny2 = canny1 * CANNY_THRESH_RATIO ;

    Canny(img_gray, img_edges, canny1, canny2) ; 

    //img_dense might be enlarged by the pyrUp/Down, so it should be trimmed somewhere before this.
    img_edges.copyTo(img_edges_masked, img_mask) ;
   
    /*
    auto lines = detect_lines(img_edges_masked, hough_threshold, 0) ;

    std::copy_if(lines.begin(), lines.end(), std::back_inserter(lines_horiz), [](Vec4i lin){
        return std::abs(lin[0] - lin[2]) > std::abs(lin[1] - lin[3]) ;
    }) ;

    std::copy_if(lines.begin(), lines.end(), std::back_inserter(lines_vert), [](Vec4i lin){
        return std::abs(lin[0] - lin[2]) < std::abs(lin[1] - lin[3]) ;
    }) ;
    */
}

static Mat generate_image(Mat src, int canny1, int hough_threshold, int block_algo) {
    //declarations of images we use
    Mat img_gray, img_edges, img_dense, img_edges_masked, img_dense_combined ;

    //adjustments for image size
    float img_size_factor = min(src.rows, src.cols) * 1.0 / IMAGE_MIN_DIM_TYPICAL ;

    cvtColor(src, img_gray, COLOR_BGR2GRAY) ;

    /*
    Mat vec_gray ;
    reduce(img_gray, vec_gray, 0, REDUCE_AVG) ;
    auto mean_img_value = mean(vec_gray)[0] ;
    std::cout << "grey image mean value:" << mean_img_value << std::endl ;
    */

    auto canny2 = canny1 * CANNY_THRESH_RATIO ;

    blur(img_gray, img_gray, Size(3, 3)) ;

    //this is just for the dense mask
    Canny(img_gray, img_edges, 20, 60) ; 
    
    detect_dense_areas_simple(img_edges, img_dense) ;
    // detect_dense_areas2(img_edges, img_dense) ;
    bitwise_not(img_dense, img_dense) ;

    //scale the hough_threshold to dimension of image
    hough_threshold = hough_threshold * img_size_factor ;


    //PASS #1

    Canny(img_gray, img_edges, canny1, canny2) ; 

    //mask the edges image with the dense block
    img_edges.copyTo(img_edges_masked, img_dense) ;
   
    std::vector<Vec4i>lines, lines_horiz, lines_vert ;
    detect_lines(img_edges_masked, lines, hough_threshold, 0) ;

    //add the areas under lines to the existing dense block, so subsequent passes can avoid them
    for(auto lin: lines) { line(img_dense, Point(lin[0], lin[1]), Point(lin[2], lin[3]), Scalar(0), 4) ; }

    separate_lines(lines, lines_horiz, lines_vert) ;
    
    //PASS # 2
    //modify the parameters
    canny1 = canny1 * 5 / 6 ;
    canny2 = canny2 * 5 / 6 ;
    // hough_threshold = hough_threshold * 3 / 4 ;    

    Mat img_edges2, img_edges_masked2 ;

    //create a new edges image
    Canny(img_gray, img_edges2, canny1, canny2) ; 

    img_edges2.copyTo(img_edges_masked2, img_dense) ;

    std::vector<Vec4i> lines2, lines_horiz2, lines_vert2 ;
    detect_lines(img_edges_masked2, lines2, hough_threshold, 0) ;

    separate_lines(lines2, lines_horiz2, lines_vert2) ;

    //end of line detection

    cvtColor(img_edges_masked2, img_edges_masked2, COLOR_GRAY2BGR) ;


    //put the block mask in dark yellow
    //reverse the mask to show it 
    bitwise_not(img_dense, img_dense) ;    
    Mat fill(Size(img_edges_masked.cols, img_edges_masked.rows), CV_8UC3, cv::Scalar(31, 63, 127));
    fill.copyTo(img_edges_masked2, img_dense) ;

    plot_lines(img_edges_masked2, lines_horiz, CYAN) ;
    plot_lines(img_edges_masked2, lines_vert, MAGENTA) ;
    plot_lines(img_edges_masked2, lines_horiz2, YELLOW) ;
    plot_lines(img_edges_masked2, lines_vert2, GREEN) ;
    
	//build vectors of ortho lines, so we can merge and detect skewed lines
	std::vector<ortho_line> olines_horiz, olines_vert ;
		
	for(auto lin : lines_horiz) { olines_horiz.push_back(ortho_line(lin)) ; }
	for(auto lin : lines_vert) { olines_vert.push_back(ortho_line(lin)) ; }
	for(auto lin : lines_horiz2) { olines_horiz.push_back(ortho_line(lin)) ; }
	for(auto lin : lines_vert2) { olines_vert.push_back(ortho_line(lin)) ; }

    std::vector<ortho_line> olines_merged_horiz, olines_merged_vert ;



    if(olines_horiz.size() < 2 || olines_vert.size() < 2) {
        std::cout << "Not enough lines before merge" << std::endl ;
    } else {
        
        merge_lines(olines_horiz, olines_merged_horiz, img_dense.rows / 2) ;
        merge_lines(olines_vert, olines_merged_vert, img_dense.cols / 2) ;

        plot_lines(img_edges_masked2, olines_merged_horiz, RED) ;
        plot_lines(img_edges_masked2, olines_merged_vert, BLUE) ;

        auto best_horiz = best_horizontal_lines(olines_horiz, src.cols * 2 / 3) ;
        auto best_vert  = best_vertical_lines(olines_vert, src.rows * 2 / 3) ;

        plot_lines(img_edges_masked2, best_horiz, YELLOW, 16) ;
        plot_lines(img_edges_masked2, best_vert, GREEN, 16) ;
        
    }
    
    


    


 
    std::cout << canny1 ;
    std::cout << ", " << canny2  ;
    std::cout << ", " << hough_threshold  ;
    std::cout << ", " << lines_horiz.size() << " hlines" ;
    std::cout << ", " << lines_vert.size() << " vlines" ;
    std::cout << std::endl ;

	putText(img_edges_masked2, std::string("H:") + std::to_string(lines_horiz.size()), Point(100, 100), FONT_HERSHEY_SIMPLEX, 2, CYAN, 6) ;
	putText(img_edges_masked2, std::string("V:") + std::to_string(lines_vert.size()), Point(100, 180), FONT_HERSHEY_SIMPLEX, 2, MAGENTA, 6) ;
	putText(img_edges_masked2, std::string("H:") + std::to_string(lines_horiz2.size()), Point(100, 260), FONT_HERSHEY_SIMPLEX, 2, YELLOW, 6) ;
	putText(img_edges_masked2, std::string("V:") + std::to_string(lines_vert2.size()), Point(100, 340), FONT_HERSHEY_SIMPLEX, 2, GREEN, 6) ;
	// putText(img_edges_masked, "HELLO", Point(100 , 100), FONT_HERSHEY_SIMPLEX, 2, viz::Color::yellow(), 4) ;

    return img_edges_masked2 ;
    // return img_dense ;
}

static void cb_image (int val, void *userdata) {
    auto pos_image = getTrackbarPos("Image", window_name) ;

    Mat img_displayed ;

    switch(pos_image) {
        case 1 :
            img_displayed = result ;
            break ;
        case 0 :
        default:
            img_displayed = g_src ;
            break ;
    }
    imshow(window_name, scale_for_display(img_displayed)) ;
}

static void cb_calib(int val, void *userdata) {
    auto pos_canny1 = getTrackbarPos("Canny1", window_name) ;
    auto pos_hacc = getTrackbarPos("HoughAcc", window_name) ;
    auto pos_block = getTrackbarPos("BlockAlg", window_name) ;

    result = generate_image(g_src, pos_canny1 * 5, pos_hacc * 50, pos_block)  ;
    cb_image(0, NULL) ;
}

Mat load_file_in_list(struct dirent **filelist, int index) {
    const auto filename = filelist[index]->d_name ;
    const auto targetpath = std::string(src_dir + std::string(filename)) ;
    std::cout << "Loading " << targetpath << std::endl ;
    setWindowTitle(window_name, std::string("Calibrator - ") + std::string(filename)) ;

    Mat src = imread(targetpath, 1) ;   

    return src ;
}

int is_good_file(const struct dirent *de) { 
    return de->d_type == DT_REG && de->d_name[0] != '.'; 
    }

int main(int argc, char *argv[]) {
    std::cout << "This is the jihanki-vision calibrator program" << std::endl ;
    std::cout << argc << " command line arguments:" << std::endl ;
    
    srand (time(NULL));


    struct dirent **filelist ;

    int num_dirents ;
    int i_current_file = 0 ;

    Mat m_src, m_result ;

    for(int a = 1 ; a < argc ; a++) {
        std::cout << argv[a] << std::endl ;
        g_src = imread(argv[a], 1) ;         
    }

    if(g_src.empty()) {
        num_dirents = scandir(src_dir, &filelist, is_good_file, NULL) ;
        g_src = load_file_in_list(filelist, 0) ;
    }

    Mat m_images[] = { m_src, m_result } ;

    namedWindow( window_name, WINDOW_AUTOSIZE );
    createTrackbar("Canny1", window_name, NULL, 10, cb_calib, (void *)m_images);
    createTrackbar("BlockAlg", window_name, NULL, 1, cb_calib, (void *)m_images);
    createTrackbar("HoughAcc", window_name, NULL, 10, cb_calib, (void *)m_images);
    createTrackbar("Image", window_name, NULL, 1, cb_image) ;
 
    // src = imread(targetfile, 1) ;

    //canny2 (hi threshold) should just be 3 times canny1

    // cb_calib(0, NULL) ;

    setTrackbarPos("Canny1", window_name, 5) ;
    setTrackbarPos("BlockAlg", window_name, 0) ;
    setTrackbarPos("HoughAcc", window_name, 5) ;
    setTrackbarPos("Image", window_name, 1) ;

    while(1) {
        auto key = pollKey() ;

        if(key == 27 || key == 'q') {
            break ;
        }

        if(key != -1) {
            std::cout << key << std::endl ;
            int pos_image ;
            // const char *filename ;

            switch(key) {
                case 'i':
                    pos_image = getTrackbarPos("Image", window_name) ;
                    setTrackbarPos("Image", window_name, pos_image == 0 ? 1 : 0) ;
                    break ;
                case 'z':
                case 'x':
                    if(key == 'z') {
                        i_current_file-- ;
                        if(i_current_file < 0) {
                            i_current_file = 0 ;
                            std::cout << "At beginning of file list" << std::endl ;
                        }
                    } else {
                        i_current_file++ ;
                        if(i_current_file == num_dirents) {
                            i_current_file = num_dirents - 1 ;
                            std::cout << "At end of file list" << std::endl ;
                        }
                    }

                    m_src = load_file_in_list(filelist, i_current_file) ;
                    g_src = m_src ;
                    cb_calib(0, m_images) ;
                    break ;
                case ' ':
                case 'r':
                    m_src = load_file_in_list(filelist, rand() % num_dirents) ;
                    g_src = m_src ;
                    cb_calib(0, m_images) ;
                    break ;
                default:
                    break ;
            }
        }
    }
//    auto key = waitKey(0);

    //cleanup
    for(int i = 0 ; i < num_dirents ; i++) {
        free(filelist[i]) ;
    }
    // free(namelist) ;

    // closedir(dp) ;
    destroyAllWindows() ;
    exit(0) ;
}

