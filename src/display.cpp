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
        XCloseDisplay(disp) ;
    #endif

    return std::make_pair(width, height) ;
}

Mat scale_for_display(Mat src, int screen_width, int screen_height) {
    //TODO: we don't need to actually pyrDown for each step.
    //just figure out how many times we need to then pass that power to resize
    int target_width = src.cols ;
    int target_height = src.rows ;
    int factor_width = 1 ;
    int factor_height = 1 ;

    while(target_height > screen_height || target_width > screen_width) {
        // factor_height *= 2 ;
        // factor_width *= 2 ;
        target_height /= 2 ;
        target_width /= 2 ;
    }

    resize(src, src, Size(target_width, target_height)) ;
    return src ;
}
