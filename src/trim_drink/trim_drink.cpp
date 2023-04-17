/*
trim_drink.cpp
Trim and image of a soft drink container to remove the background margin.
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

#include <string>
#include <iostream>
#include <unistd.h>
#include <libgen.h>
#include <fstream>

#include "lines.hpp"
#include "trim_rect.hpp"

using namespace cv;

int process_file(std::string filename);
void write_processed_file(Mat img, std::string filepath);
#ifdef USE_GUI
int show_result_images(Mat target, Mat corners, Rect rc_clip, bool isValid = true);
#endif

auto cmdopt_verbose = false;
auto cmdopt_batch = false;
auto cmdopt_equalizehist = false ;
auto thresh = 127;

auto dest_dir = std::string(".") ;

void help()
{
    std::cout << "trim_drink" << std::endl ;
    std::cout << "  -b : batch mode (no display)" << std::endl ;
    std::cout << "  -d dir : batch mode target directory" << std::endl ;
    std::cout << "  -e : equalize histogram" << std::endl ;
    std::cout << "  -h : help" << std::endl ;
    std::cout << "  -v : verbose" << std::endl ;

    exit(0) ;
}

int main(int argc, char **argv)
{
    if (argc < 2) { help(); }

    int c;
    char *cvalue;

    while ((c = getopt(argc, argv, "bd:ehv")) != -1)
    {
        switch (c)
        {
        case 'b':
            cmdopt_batch = true;
            break;
        case 'd':
            cvalue = optarg;
            dest_dir = cvalue;
            std::cout << "Dest dir:" << dest_dir << std::endl;
            break;
        case 'e':
            cmdopt_equalizehist = true;
            break;
        case 'h':
            help();
            exit(0);
            break;
        case 'v':
            cmdopt_verbose = true;
            break;
        }
    }

    for (auto idx = optind; idx < argc; idx++) {
        char *filename = argv[idx];

        std::ifstream ifile(filename);
        if (!ifile)
        {
            std::cerr << "File does not exist: " << filename << std::endl;
        }

        if (cmdopt_verbose)
        {
            std::cout << "Start processing " << filename << std::endl;
        }

        process_file(filename);

        if (cmdopt_verbose)
        {
            std::cout << "Finished processing " << filename << std::endl;
        }

    }

    return 0;
}

int process_file(std::string infilepath)
{
    Mat src, src_gray ;
    src = imread(infilepath, 1);
    cvtColor(src, src_gray, COLOR_BGR2GRAY);

    // img_corners = Mat::zeros( src.size(), CV_32FC1 );

    float threshold_ratio = 0.01;
    std::vector<size_t> marked_rows ;
    Mat detected_corners = src_gray.clone();

    Rect rc_clip = get_trim_rect(src_gray, detected_corners, threshold_ratio, cmdopt_equalizehist);

    //end of processing


    bool is_valid_rect = true;
    //TODO: Further checks to see if it is valid
    if (rc_clip.height < src.cols) {
        is_valid_rect = false;
    }

    #ifdef USE_GUI
    if (!cmdopt_batch) {
        int show_key = show_result_images(src_gray, detected_corners, rc_clip, is_valid_rect);
        if (cmdopt_verbose) {
            std::cout << "Window closed with keycode: " << show_key << std::endl;
        }
    }
    #endif

    //write to file
    if (is_valid_rect) {
       write_processed_file(src(rc_clip), infilepath);
    } else {
        if (cmdopt_verbose) {
            std::cout << "Invalid trim rectangle." << std::endl;
            return -1 ;
        }
    }

    return 0;
}


#ifdef USE_GUI
/*
Display visual feedback. Since the main purpose of the program only needs to detect rows with any corners,
it is only in this function that we determine the column positions of the corners.
*/
int show_result_images(Mat img_target, Mat img_corners, Rect rc_clip, bool is_valid)
{
    cvtColor(img_target, img_target, COLOR_GRAY2BGR);

    int circle_radius = 3;
    int circle_thickness = 1;
    Scalar circle_color = Scalar(244, 0, 200);
    Scalar rect_color ;


    if(is_valid) {
        rect_color = Scalar(0, 255, 127);
    } else {
        rect_color = Scalar(0, 0, 255);
    }
    rectangle(img_target, rc_clip, rect_color, 2);

    for(auto row = 0 ; row < img_corners.rows ; row++) {
        for(auto col = 0 ; col < img_corners.cols ; col++) {
            if(img_corners.at<int>(row, col) > 0) {

                auto pt = Point(col, row) ;
                circle(img_target, pt, circle_radius, circle_color, circle_thickness, 8, 0);
            }
        }
    }

    // cv::imshow("Original", src) ;
    cv::imshow("Detected Corners", img_target);
    int key = cv::waitKey();
    return key ;
}
#endif

void write_processed_file(Mat img, std::string infilepath)
{
    //Write out the drink images to files
    char *ifp = (char *)infilepath.c_str() ;
    std::string filenamestr = std::string(basename(ifp));

    std::stringstream outfilepath;
    outfilepath << dest_dir << "/" << filenamestr;

    if (cmdopt_verbose)
    {
        std::cout << "Writing trimmed image to " << outfilepath.str() << std::endl;
    }
    imwrite(outfilepath.str(), img);
}
