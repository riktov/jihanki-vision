/** extract_drinks_write.cpp
*/
#if CV_VERSION_MAJOR >= 4
    #include <opencv4/opencv2/imgproc.hpp>
    #include <opencv4/opencv2/imgcodecs.hpp>
#else
    #include <opencv2/imgproc/imgproc.hpp>
    #include <opencv2/imgcodecs/imgcodecs.hpp>
#endif

using namespace::cv ;

#include <iostream>
#include "extract_drinks_write.hpp"

extern std::string dest_dir ;
extern int cmdopt_verbose ;

std::string slot_image_filename(const std::string &basename, int row, int slot, const std::string prefix, Rect rc) ;


void write_slot_image_files(
    Mat src,
    std::vector<std::vector<Rect> > slot_image_rows, 
    std::string outfilepath,
    const std::string prefix
    ) 
    {
    //Write out the drink images to files
	char *path = (char *)outfilepath.c_str() ;	//POSIX basename must be non-const
    auto filenamestr = std::string(basename(path)) ;

    for(size_t idx_row = 0 ; idx_row < slot_image_rows.size() ; idx_row++) {
        std::vector<Rect> rects = slot_image_rows.at(idx_row) ;
        for(size_t idx_slot = 0 ; idx_slot < rects.size() ; idx_slot++) {
            Rect rc = rects.at(idx_slot) ;
            std::stringstream drink_filepath ;
            drink_filepath << dest_dir << "/" << slot_image_filename(filenamestr, idx_row + 1, idx_slot + 1, prefix, rc) ;
            if(cmdopt_verbose) {
                // std::cout << drink_filepath.str() << std::endl ;
            }
            imwrite(drink_filepath.str(), src(rc)) ;
        }
    }
}

void write_strip_image_files(Mat src, std::vector<Rect> strips, std::string outfilepath) {
    std::stringstream img_filepath ;
    img_filepath << dest_dir << "/" << "price_strip_" << outfilepath ;
    if(cmdopt_verbose) {
        std::cout << img_filepath.str() << std::endl ;
    }
    imwrite(img_filepath.str(), src) ;
}

std::string slot_image_filename(const std::string &basename, int row, int slot, const std::string prefix, Rect rc) {
    auto ext_sep = std::string(".") ;
    auto underscore = std::string("_") ;
    auto lastindex = basename.find_last_of(ext_sep) ;
    auto f_name = basename.substr(0, lastindex) ;
    auto f_ext  = basename.substr(lastindex + 1) ;

    std::stringstream slot_str ;
    if(slot < 10) {
        slot_str << "0" ;
    }
    slot_str << slot ;
    
    std::stringstream ss_rect ;
    ss_rect << rc.x << "@" << rc.y ;

    auto drink_filename =
        std::string(prefix) +
        std::string(f_name) + underscore +
        "r" + std::to_string(row) + underscore + 
        "s" + slot_str.str() + "_" + ss_rect.str() + 
        ext_sep + f_ext ;

    return drink_filename ;
}
