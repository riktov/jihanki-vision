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
#include <opencv4/opencv2/viz/types.hpp>
#else
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
// #include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/viz/types.hpp>
#endif

#include "detect.hpp" 
#include "fixperspective_draw.hpp"
#include "../display.hpp"

const char *src_dir = "/home/paul/Pictures/dcim/jihanki/" ;
// const char *src_dir = "/home/paul/Pictures/dcim/2021/04/" ;
const char *window_name = "Calibrator" ;

bool cmdopt_verbose = true ;

using namespace cv ;

Mat g_src, result ;

static Mat get_lines(Mat src, int canny1, int hough_threshold, int block_algo, Mat mask) {
    return Mat() ;
}

static Mat generate_image(Mat src, int canny1, int hough_threshold, int block_algo) {
    // std::cout << iterations << std::endl ;

    const int IMAGE_MIN_DIM_TYPICAL = 2500 ;
    const float CANNY_THRESH_RATIO = 2.5 ;

    //general data on the image
    float img_size_factor = min(src.rows, src.cols) * 1.0 / IMAGE_MIN_DIM_TYPICAL ;


    Mat img_gray, img_edges, img_dense, img_edges_masked, img_dense_combined ;

    cvtColor(src, img_gray, COLOR_BGR2GRAY) ;

    Mat vec_gray ;
    reduce(img_gray, vec_gray, 0, REDUCE_AVG) ;
    
    auto mean_img_value = mean(vec_gray)[0] ;
    std::cout << "grey image mean value:" << mean_img_value << std::endl ;


    // const int MAX_PASSES = 1 ;
    std::vector<Vec4i> lines_horiz, lines_vert ;

    // int pass = 0 ;

    auto canny2 = canny1 * CANNY_THRESH_RATIO ;

    blur(img_gray, img_gray, Size(3, 3)) ;

    //this is just for the dense mask
    Canny(img_gray, img_edges, 20, 60) ; 
    
    // img_dense = Mat(img_edges.size(), 0) ;

  // Start measuring time
  /*
    auto begin = std::chrono::high_resolution_clock::now();
 
    if(block_algo == 1) {
        detect_dense_areas(img_edges, img_dense) ;
    } else {
        detect_dense_areas_simple(img_edges, img_dense) ;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin);
    
    if(block_algo == 1) {
        printf("Blocking algorithm: erode/dilate\n");
    } else {
        printf("Blocking algorithm: simple\n" );
    }

    printf("%.3f seconds.\n", elapsed.count() * 1e-9);
    */

    detect_dense_areas(img_edges, img_dense) ;

    bitwise_not(img_dense, img_dense) ;

    //scale the hough_threshold to minimum dimension of image
    hough_threshold = hough_threshold * img_size_factor ;




    //PASS #1
    Canny(img_gray, img_edges, canny1, canny2) ; 

    //img_dense might be enlarged by the pyrUp/Down, so it should be trimmed somewhere before this.
    img_edges.copyTo(img_edges_masked, img_dense) ;
   
    auto lines = detect_lines(img_edges_masked, hough_threshold, 0) ;

    std::copy_if(lines.begin(), lines.end(), std::back_inserter(lines_horiz), [](Vec4i lin){
        return std::abs(lin[0] - lin[2]) > std::abs(lin[1] - lin[3]) ;
    }) ;

    std::copy_if(lines.begin(), lines.end(), std::back_inserter(lines_vert), [](Vec4i lin){
        return std::abs(lin[0] - lin[2]) < std::abs(lin[1] - lin[3]) ;
    }) ;
    
    //PASS 2
    //modify the parameters
    canny1 = canny1 * 5 / 6 ;
    canny2 = canny2 * 5 / 6 ;
    // hough_threshold = hough_threshold * 3 / 4 ;    

    Mat img_edges2, img_edges_masked2 ;

    //create a new edges image
    Canny(img_gray, img_edges2, canny1, canny2) ; 

    //add the areas under the first-pass lines to the existing dense mask
    for(auto lin: lines_horiz) {
        line(img_dense, Point(lin[0], lin[1]), Point(lin[2], lin[3]), Scalar(0), 4) ;
    }
    for(auto lin: lines_vert) {
        line(img_dense, Point(lin[0], lin[1]), Point(lin[2], lin[3]), Scalar(0), 4) ;
    }

    img_edges2.copyTo(img_edges_masked2, img_dense) ;

    lines = detect_lines(img_edges_masked2, hough_threshold, 0) ;

    std::vector<Vec4i> lines_horiz2, lines_vert2 ;
    std::copy_if(lines.begin(), lines.end(), std::back_inserter(lines_horiz2), [](Vec4i lin){
        return std::abs(lin[0] - lin[2]) > std::abs(lin[1] - lin[3]) ;
    }) ;

    std::copy_if(lines.begin(), lines.end(), std::back_inserter(lines_vert2), [](Vec4i lin){
        return std::abs(lin[0] - lin[2]) < std::abs(lin[1] - lin[3]) ;
    }) ;

    //end of pass


    //reverse the mask to show it 
    bitwise_not(img_dense, img_dense) ;
    
    cvtColor(img_edges_masked2, img_edges_masked2, COLOR_GRAY2BGR) ;

    
    //put the block mask in dark green
    Mat fill(Size(img_edges_masked.cols, img_edges_masked.rows), CV_8UC3, cv::Scalar(63, 127, 127));
    // fill.copyTo(img_edges_masked, img_dense) ;

    
    plot_lines(img_edges_masked2, lines_horiz, viz::Color::cyan()) ;
    plot_lines(img_edges_masked2, lines_vert, viz::Color::magenta()) ;
    plot_lines(img_edges_masked2, lines_horiz2, viz::Color::yellow()) ;
    plot_lines(img_edges_masked2, lines_vert2, viz::Color::green()) ;
    

 
    std::cout << canny1 ;
    std::cout << ", " << canny2  ;
    std::cout << ", " << hough_threshold  ;
    std::cout << ", " << lines_horiz.size() << " hlines" ;
    std::cout << ", " << lines_vert.size() << " vlines" ;
    std::cout << std::endl ;

	putText(img_edges_masked2, std::string("H:") + std::to_string(lines_horiz.size()), Point(100, 100), FONT_HERSHEY_SIMPLEX, 2, viz::Color::cyan(), 6) ;
	putText(img_edges_masked2, std::string("V:") + std::to_string(lines_vert.size()), Point(100, 180), FONT_HERSHEY_SIMPLEX, 2, viz::Color::magenta(), 6) ;
	putText(img_edges_masked2, std::string("H:") + std::to_string(lines_horiz2.size()), Point(100, 260), FONT_HERSHEY_SIMPLEX, 2, viz::Color::yellow(), 6) ;
	putText(img_edges_masked2, std::string("V:") + std::to_string(lines_vert2.size()), Point(100, 340), FONT_HERSHEY_SIMPLEX, 2, viz::Color::green(), 6) ;
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

