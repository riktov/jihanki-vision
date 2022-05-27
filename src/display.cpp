/* display.cpp 
Support functions for displaying imagess
*/
#include <utility>

#if CV_VERSION_MAJOR >= 4
    #include <opencv4/opencv2/highgui.hpp>
    #include <opencv4/opencv2/imgproc.hpp>
#else
    #include <opencv2/highgui/highgui.hpp>
    #include <opencv2/imgproc/imgproc.hpp>
#endif

#ifdef _WIN32
    #include <windows.h>
#else
    #include <X11/Xlib.h>
#endif

using namespace cv ;

std::pair<int, int> screen_dims() {
    #ifdef _WIN32
        int width  = (int) GetSystemMetrics(SM_CXSCREEN);
        int height = (int) GetSystemMetrics(SM_CYSCREEN);
    #else
        Display* disp = XOpenDisplay(NULL);
        Screen*  scrn = DefaultScreenOfDisplay(disp);
        int width  = scrn->width;
        int height = scrn->height;
    #endif

    return std::make_pair(width, height) ;
}

Mat scale_for_display(Mat src, int width, int height) {

    while(src.rows > height || src.cols > width) {
        pyrDown(src, src, Size(src.cols/2 , src.rows/2)) ;
    }
    return src ;
}
