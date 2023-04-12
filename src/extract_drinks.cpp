/*****************
extract_drinks.cpp

Detects and extract images for drinks from an image of a Japanese soft-drink vending machine
 ****************/
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
#include <numeric>
#include <memory>
#include <fstream>
#include <algorithm>

#include <libgen.h>//basename
#include <unistd.h> //short opts
#include <getopt.h> //long opts

#include "lines.hpp"
#include "button_strip.hpp"
#include "run_length.hpp"
#include "trim_rect.hpp"
#include "threshold.hpp"

#include "extract_drinks_write.hpp"

#ifdef USE_GUI
#include "extract_drinks_draw.hpp"
#include "display.hpp"
#endif


using namespace cv ;

/* local function declarations */
int process_file(std::string infilepath, std::string dest_dir) ;
//const bool is_zero_line(Vec4i l) ;
//Mat transform_perspective(Mat img, Vec4i top, Vec4i bottom, Vec4i left, Vec4i right) ;
//inline Mat transform_perspective(Mat img, const std::vector<Vec4i>lines_tblr) {
//  return transform_perspective(img, lines_tblr[0], lines_tblr[1], lines_tblr[2], lines_tblr[3]) ;
//}
// std::vector<int> histogram(std::vector<int> vals) ;
std::vector<std::vector<Point> > detect_button_strips(Mat src_gray, int thresh_val) ;
bool drink_rect_is_valid(Rect rc) ;
std::string slot_image_filename(const std::string &basename, int row, int slot_num, const std::string prefix=std::string(""), Rect rc=Rect()) ;
// std::string price_image_filename(const std::string &basename, int row, int slot_num) ;
// std::vector<Rect> build_row_rects(std::vector<int> sep_lines, int height, Point pt_strip_tl) ;
void build_row_rects(const std::vector<int> sep_lines, int height, Point pt_strip_tl, std::vector<Rect> &rects_drinks, std::vector<Rect> &rects_prices) ;
void write_slot_image_files(Mat img, std::vector<std::vector<Rect> > slot_row_rects, std::string outfilepath, std::string prefix) ;
// std::vector<int> detect_drink_rows(Mat src, std::vector<Vec4i> horizontal_lines) ;
// std::vector<size_t> get_drink_columns(Mat img) ;
// void plot_hough_and_bounds(Mat src_bgr, std::vector<Vec4i> hough_lines, std::vector<Vec4i> bounds_tblr) ;
//Mat crop_orthogonal(Mat src) ;
//std::vector<Vec4i> detect_bounding_lines(Mat src, int hough_threshold = 50) ;
//std::vector<Vec4i> button_strip_lines(Mat src_gray) ;
int detect_button_strip_contours_stepped(Mat src, std::vector<std::vector<Point> >& contours,int initial_threshold) ;
void find_button_strip_contours(Mat src_gray, std::vector<std::vector<Point> >& contours, int thresh_val) ;
std::vector<int> adjusted_slot_counts(std::vector<std::shared_ptr<ButtonStrip> > strips) ;

//constants
const auto LABEL_ASPECT = 2.8 ;
int STRIP_DETECTION_THRESH_BIAS = 20 ;

// global filenames
std::string filename, dest_dir ;
std::string default_dest_dir = "." ;

const std::string UNDERSCORE = std::string("_") ;
const std::string PATH_SEPARATOR = std::string("/") ;
const std::string ext_sep = std::string(".") ;

std::vector<Mat> images_to_display ;


/*
static void onMouse(int event, int x, int y, int, void*) {
  if (event == EVENT_LBUTTONDOWN) {
    std::cout << "Mouse clicked at " << x << ":" << y << std::endl ;
  }
}
*/


void help() {
    std::cout << "extract_drinks" << std::endl ;
    std::cout << "  -b : batch mode (no display)" << std::endl ;
    std::cout << "  -c : print drink slot configuration for first file (no drink image files written)" << std::endl ;
    std::cout << "  -d [path] : batch mode target directory" << std::endl ;
    std::cout << "  -f : do not write output files" << std::endl ;
    std::cout << "  -h : help" << std::endl ;
    std::cout << "  -p : disable perspective correction" << std::endl ;
    std::cout << "  -t : disable trimming slots to container" << std::endl ;
    std::cout << "  -T [num]: highlight detection threshold" << std::endl ;
    std::cout << "  -v : verbose" << std::endl ;

    exit(0) ;
}

void test() {
    std::cout << "RUNNING TESTS ONLY" << std::endl ;
    std::vector <int> v = {1, 2, 1, 3, 3, 3, 5, 5, 2, 2, 2 } ;
    std::vector <size_t> rl = run_lengths(v, 3) ;
    for(auto const r : rl) {
        std::cout << r << "-" ;
    }
}

std::string window_name_canny = "Canny Edges" ;
std::string window_name_src = "Original" ;

bool cmdopt_batch = false ;
int cmdopt_verbose = 0 ;
bool cmdopt_configuration = false ;
bool cmdopt_perspective = true ;
bool cmdopt_trim_to_container = true ;
bool cmdopt_write_files = true ;
int cmdoptval_threshold = 0 ;

int handle_args(int argc, char **argv) {
 int c;

  while (1)
    {
      static struct option long_options[] =
        {
          /* These options set a flag. */
          {"verbose", no_argument,       &cmdopt_verbose, 1},
          {"brief",   no_argument,       &cmdopt_verbose, 0},
          /* These options don’t set a flag.
             We distinguish them by their indices. */
          {"add",     no_argument,       0, 'a'},
          {"append",  no_argument,       0, 'b'},
          {"delete",  required_argument, 0, 'd'},
          {"create",  required_argument, 0, 'c'},
          {"file",    required_argument, 0, 'f'},
          {0, 0, 0, 0}
        };
      /* getopt_long stores the option index here. */
      int option_index = 0;

      c = getopt_long (argc, argv, "abc:d:f:",
                       long_options, &option_index);

      /* Detect the end of the options. */
      if (c == -1)
        break;

      switch (c)
        {
        case 0:
          /* If this option set a flag, do nothing else now. */
          if (long_options[option_index].flag != 0)
            break;
          printf ("option %s", long_options[option_index].name);
          if (optarg)
            printf (" with arg %s", optarg);
          printf ("\n");
          break;

        case 'a':
          puts ("option -a\n");
          break;

        case 'b':
          puts ("option -b\n");
          break;

        case 'c':
          printf ("option -c with value `%s'\n", optarg);
          break;

        case 'd':
          printf ("option -d with value `%s'\n", optarg);
          break;

        case 'f':
          printf ("option -f with value `%s'\n", optarg);
          break;

        case '?':
          /* getopt_long already printed an error message. */
          break;

        default:
          abort ();
        }
    }

  /* Instead of reporting ‘--verbose’
     and ‘--brief’ as they are encountered,
     we report the final status resulting from them. */
  if (cmdopt_verbose)
    puts ("verbose flag is set");

  /* Print any remaining command line arguments (not options). */
  if (optind < argc)
    {
      printf ("non-option ARGV-elements: ");
      while (optind < argc)
        printf ("%s ", argv[optind++]);
      putchar ('\n');
    }

    return c ;
}

int main(int argc, char **argv) {
    //int flag_d = 0 ;
    char *cvalue = NULL ;
    int c ;
    
    //test() ; exit(0);

    if (argc < 2) {
        help() ;
    }

    dest_dir = default_dest_dir ;
    
    while((c = getopt(argc, argv, "bcd:fhptT:v")) != -1) {
        switch(c) {
        case 'b':
            cmdopt_batch = true ;
            break ;
        case 'c':
            cmdopt_configuration = true ;
            break ;
        case 'd':
            cvalue = optarg ;
            dest_dir = cvalue ;
            break ;
        case 'f':
            cvalue = optarg ;
            cmdopt_write_files = false ;
            break ;
        case 'h':
            help() ;
            exit(0) ;
        case 'p':
            cmdopt_perspective = false ;
            break ;
        case 't':
            cmdopt_trim_to_container = false ;
            break ;
        case 'T':
            cvalue = optarg ;
            cmdoptval_threshold = atoi(cvalue) ;
            break ;
        case 'v':
            cmdopt_verbose = true ;
            break ;
        }
    }

    #ifdef USE_GUI
    if(!cmdopt_batch && cmdopt_verbose) {   
        auto dims = screen_dims() ;
        std::cout << "Screen dims: " << dims.first << "x" << dims.second << std::endl ;
    }
    #endif

    int result = 0 ;
    
    for (int idx = optind ; idx < argc ; idx++) {
        filename = argv[idx] ;
        
        std::ifstream ifile(filename) ;
        if(!ifile) {
            std::cerr << "File does not exist: " << filename << std::endl ;
            continue ;
        }

        if(cmdopt_verbose) {
            std::cout << "Start processing " << filename << std::endl ;
        }
        
        result = process_file(filename, dest_dir) ;
        
        if(cmdopt_verbose) {
            std::cout << "Finished processing " << filename << std::endl ;
        }
        
        #ifdef USE_GUI
        if(!cmdopt_batch) {    
            for(const auto &img : images_to_display) {
                imshow("All strips", scale_for_display(img)) ;
            }
        }
        #endif
    }
    
    return result ;
}

/*
filename must not be const for POSIX version of basename()
*/
int process_file(std::string infilepath, const std::string dest_dir) {
    Mat src  = imread(infilepath, 1) ; //color

    if(src.empty()) {
        std::cerr << "File is not a valid image: " << infilepath << std::endl ;
        return -1 ;
    }
        
    Mat src_gray ;
    cvtColor(src, src_gray, COLOR_BGR2GRAY) ;
    
    //detect the highlights on the button strips, which are the purest white in the image
    //Mat src_thresh ;

    //this will receive the detected contours
    std::vector<std::vector<Point> > button_strip_contours ;
    
    unsigned int strip_detection_thresh = 0 ;
    // bool is_strip_detected = false ;

    if(cmdoptval_threshold > 0) {
        strip_detection_thresh = cmdoptval_threshold ;
    } else {
        strip_detection_thresh = detect_threshold(src) - STRIP_DETECTION_THRESH_BIAS;
    }

    find_button_strip_contours(src_gray, button_strip_contours, strip_detection_thresh) ;

    if(button_strip_contours.size() > 1) {
        if(cmdopt_verbose) {
            std::cout << "Detected button strips using detected or specified threshold:" << strip_detection_thresh << std::endl ;
        }
    } else {
        if(cmdopt_verbose) {
            std::cout << "Could not detected button strips using detected or specified threshold:" << strip_detection_thresh << std::endl ;
            std::cout << "Trying stepping through predefined threshold range." << std::endl ;
        }
        strip_detection_thresh = detect_button_strip_contours_stepped(src_gray, button_strip_contours, strip_detection_thresh) ;
    }

    if(button_strip_contours.size() < 2) {
        std::cerr << "Failed at detecting button strips; bailing. " << strip_detection_thresh << std::endl ;
        #ifdef USE_GUI
        if(!cmdopt_batch) {
            imshow("Original image, could not detect button strips", scale_for_display(src_gray)) ;
            waitKey() ;
        }
        #endif
        return -1;
    }
    
    
    //Sort the button-strip countours vertically
    std::sort(button_strip_contours.begin(), button_strip_contours.end(),
	      [](const std::vector<Point> c1, const std::vector<Point> c2) {
		  int y1 = boundingRect(c1).y ;
		  int y2 = boundingRect(c2).y ;
		  return y1 < y2 ;
	      }) ;


    
    //create ButtonStrip objects from the possibly merged contours
    //we pass the strip detection threshold, which was used to detect the strips, to then extend the button image
    auto strips = merged_button_strips(button_strip_contours, strip_detection_thresh) ;
    if(cmdopt_verbose) {
        std::cout << "Created " << strips.size() << " ButtonStrip objects." << std::endl ;
    }
    //////////////////


    //detect the straight bottom edge of the button strip
    std::vector<Mat> imgs_filled_strips ; 


    #ifdef USE_GUI
    Mat img_thresh ;
    threshold(src_gray, img_thresh, strip_detection_thresh, 255, THRESH_BINARY) ;
    cvtColor(img_thresh, img_thresh, COLOR_GRAY2BGR) ;
 
    std::stringstream strtitle ;
    strtitle << "Threshold image: " << strip_detection_thresh ;

    for(const auto &strip : strips) {
        draw_strip_boundary(img_thresh, strip) ;
    }
    #endif

    int stripnum = 0 ;

    std::vector<Rect> trimmed_rects ;

    // stripnum = 0 ;

    // for(auto img : imgs_filled_strips) {
    //     std::stringstream title ;
    //     title << "Strip " << ++stripnum ;
    //     imshow(title.str(), scale_for_display(img)) ;    
    // }


    
    /* Now that we have the button strip objects holding images, we can process them to detect the slots
     * Actually this should all be done via the constructor
     */
    // auto it_img = 
    int idx = 0 ;
    for(auto strip : strips) {
        // strip->setImage(imgs_filled_strips.at(idx)) ;
        // Rect rc = strip->rc() ;
        // rc = Rect(Point(rc.x, rc.y - rc.height), rc.br()) ;
        // strip->setRect(rc) ;
        //strip->extendButtonImageUp(src_gray) ;

        // strip->extendButtonImage(src_gray) ; //if the strips are too thin

        Mat img_filled ;
        //crop_to_lower_edge(strip->image(), img_filled) ; //PNR2022
        img_filled = strip->image().clone() ;

        if(cmdopt_verbose) {
            // std::cout << "Filled image:" << img_filled.size() << std::endl ;
        }
        
        /*
        //not very useful

        #ifdef USE_GUI
        if (!cmdopt_batch) {
            std::stringstream ss ;
            ss << "Filled: " << idx ;
            imshow(ss.str(), img_filled) ;
        }
        #endif
        */

        idx++ ;

        //continue ;

        strip->generateIntensityValuesLine() ;
        strip->generateSlotsRunLength() ;
        strip->generateDrinkSlots() ;
    }

    #ifdef USE_GUI
    //waitKey() ;   //for the strips above
    //return 0 ;
    #endif
    
    //draw the trimmed rects
    #ifdef USE_GUI
    for(auto rc : trimmed_rects) {
        rectangle(img_thresh, rc, Scalar(220, 250, 92), 4) ;
    }
    if(!cmdopt_batch) {
        imshow(strtitle.str(), scale_for_display(img_thresh)) ;
    }
    #endif
    
    if(cmdopt_verbose) {
        std::cout << "All detected strips after extending images: " << std::endl ;
        for(const auto &strip : strips) {
            std::cout << strip->rc() ;

            // std::cout << ", slot height:" << strip->slots_height() ;
            
            float periodicity = run_length_second_diff(strip->runs()) ;
            std::cout << ", P:" << periodicity ;
            if(periodicity < 200) {
                std::cout << ", slots:" << strip->slot_separators().size() - 1 ;
            }
            std::cout << std::endl ;
        }
    }

    /* At this point we should see if there are any low-P (high periodicity) strips.
    If so, we can discard any high-P (>200) strips
    */

   /*
     Iterate over the button strips to obtain an accurate detection of the configuration,
     location, and dimensions of the button strips.
     The goal is not to extract from all strips as detected, it is to determine if we have
     an accurate detection of the configuration.
     */

    /* Sort the button strips by RSLD. This is a bit inefficient, since we calculate it on every comparison
       but is not worth creating a struct for. 
       It would be so easy to do if we could just create tuples with different types
    */

    /*
    std::vector<std::shared_ptr<ButtonStrip> > sorted_strips = strips ;
    std::sort(sorted_strips.begin(), sorted_strips.end(),
	      [](const std::shared_ptr<ButtonStrip> s1, const std::shared_ptr<ButtonStrip> s2) {
		  float r1 = run_length_second_diff(s1->runs()) ;
		  float r2 = run_length_second_diff(s2->runs()) ;
		  return r1 < r2 ;
	      }) ;
    */
    
    /* As long as the rlsd is reliable, a wider strip is more likely to be a full strip.
       So take the widest strip with a reliable rlsd

       We analyze just the strips with low RLSD. We determine the width of of a full strip,
       The mean slot width, and the number of slots.
     */

    /* New scheme: Instead of analyzing the periodicity of individual strips, merge them 
    */
   std::vector<Mat> thresh_images ;
   for(auto strip : strips) {
       thresh_images.push_back(strip->img_thresh()) ;
   }

    Mat img_merged_thresh = merge_thresh_images(thresh_images) ;
    resize(img_merged_thresh, img_merged_thresh, Size(1000, img_merged_thresh.rows * 8)) ;
    // imshow("Merged thresh", img_merged_thresh) ;

    const auto RLSD_THRESHOLD = 10.0 ;//5.0
    std::vector<std::shared_ptr<ButtonStrip> > strips_with_good_rlsd ;
    std::copy_if(strips.begin(), strips.end(), std::back_inserter(strips_with_good_rlsd),
	    [RLSD_THRESHOLD](const std::shared_ptr<ButtonStrip> strip) {
		return (run_length_second_diff(strip->runs()) < RLSD_THRESHOLD) ;
	    }) ;

    if(strips_with_good_rlsd.size() < 1) {
    	std::cerr << "No strips with good stable periodicity to analyze, bailing. File:" << infilepath << std::endl ;
        #ifdef USE_GUI
        if(!cmdopt_batch) {
    	    show_result_images(src, strips, {}, {}, {}) ;
        }
        #endif
	return -2 ;	
    }

    //not sure how to use this
    if(false) {
        auto adjusted_slots = adjusted_slot_counts(strips_with_good_rlsd) ;

        if(cmdopt_verbose) {
            for(int i = 0 ; i < strips_with_good_rlsd.size() ; i++) {
                std::cout << strips_with_good_rlsd[i]->rc() << " " ;
                std::cout << strips_with_good_rlsd[i]->slot_separators().size() ;
                std::cout << ", " << adjusted_slots[i] << std::endl ;
            }
        }
    }




    Rect widest_strip_rect = Rect(0, 0, 0, 0) ;
    for(const auto &strip : strips_with_good_rlsd) {
		if(strip->rc().width > widest_strip_rect.width) {
			widest_strip_rect = strip->rc() ;
		}
		if(cmdopt_verbose) {
		//	std::cout << strip->rc() << ", Periodicity:" << run_length_second_diff(strip->runs()) << std::endl ;
		}
    }

    //TODO: check if the strip has wide endcaps, so we can adjust the width

    /* now we check the dimensions of all the strips against the widest one
       to select all accurate ones regardless of RLSD

       If there are two or more strips with all variances < 0.1, then we should discard all strip with 
       two or more variances > 0.2
     */
    
    if(cmdopt_verbose) {
	//	std::cout << "--- The widest strip rect " << widest_strip_rect << std::endl ;
		std::cout << "Filtering strips for good width or alignment with the widest one." << std::endl ;
    }

    std::vector<std::shared_ptr<ButtonStrip> > strips_with_good_width ;
    const auto width_var_threshold = 0.05 ;
    const auto pos_threshold = 0.08 ;
    const auto pos_threshold_extreme = 0.008 ;
    int extremely_aligned = 0 ;

    for(const auto &strip: strips) {
		const auto width_variance = abs(1 - (strip->rc().width * 1.0 / widest_strip_rect.width)) ;
		const auto left_position_variance  = abs(strip->rc().x - widest_strip_rect.x) * 1.0 / src.cols ;
		const auto right_position_variance = abs(strip->rc().br().x - widest_strip_rect.br().x) * 1.0 / src.cols ;


        char closeness_indicator = ' ' ;
        
        //TODO : change this so it is good only if it passes as least two criteria, instead of one.

        int closeness_counts = 0 ;

        if(width_variance < width_var_threshold) {
            closeness_counts++ ;
        }
		if (left_position_variance < pos_threshold) {
            closeness_counts++ ;
        }
        if(right_position_variance < pos_threshold) {
			closeness_counts++ ;
		}

        if(closeness_counts > 1){
            strips_with_good_width.push_back(strip) ;
            closeness_indicator = '+' ;
        }


        if(strip->rc() == widest_strip_rect) {
            closeness_indicator = '0' ; //the widest strip, which we are comparing against
        } else {		
            if((left_position_variance < pos_threshold_extreme) && (right_position_variance < pos_threshold_extreme)) {
                closeness_indicator = '!' ;
                extremely_aligned++ ;
            }
        }

		if(cmdopt_verbose) {
            std::cout << closeness_indicator ;
            std::cout << strip->rc() ;
            std::cout << "; W: " << width_variance ;
            std::cout << ", L: " << left_position_variance ;
            std::cout << ", R: " << right_position_variance << std::endl ;
		}
    }

/*
    if(cmdopt_verbose) {
        std::cout << "Strips with good width or position:" << std::endl ;
        for(auto strip: strips_with_good_width) {
            std::cout << strip->rc() << " : " << strip->rc().x << " <--> " << strip->rc().br().x << std::endl ;
        }
    }
*/

    if(cmdopt_verbose) {
        std::cout << "Totaling widths and counts to calculate a mean..."  ;
    }
    
    //calculate the mean slot width from the good slots in the good RLSD strips
    //TODO: Do this with a histogram
    int total_slot_widths = 0 ;
    int total_slot_count = 0 ;

    for(const auto &strip : strips_with_good_rlsd) {
		const auto max_span = strip->rc().width / 5 ;
		const auto min_span = strip->rc().width / 15 ; 

		const auto sep_lines = strip->slot_separators() ;
		for(size_t i = 1 ; i < sep_lines.size() ; i++) {
			int span = sep_lines[i] - sep_lines[i -1] ;
			if((span < max_span) && (span > min_span)) {
                total_slot_count++ ;
                total_slot_widths += span ;
			}
		}
    }
    if(cmdopt_verbose) {
        std::cout << "done, " << total_slot_count << " slots." << std::endl ;
    }

    //Should we bail?

    if(total_slot_count < 1) {
        std::cerr << "No good slots, bailing." << std::endl ;
        #ifdef USE_GUI
        if(!cmdopt_batch) {
            show_result_images(src, strips, {}, {}, {}) ;
        }
        #endif
        return -2 ;
    }

    auto mean_slot_width = total_slot_widths / total_slot_count ;
    auto fnum_slots = widest_strip_rect.width * 1.0 / mean_slot_width ;
    size_t num_slots = round(fnum_slots) ;

    if(cmdopt_verbose) {
        std::cout << "Mean slot width: " << mean_slot_width << std::endl ;
        std::cout << "fnum_slots: " << fnum_slots << std::endl ;
        std::cout << "---Detected configuration: " << num_slots << "x" << strips_with_good_width.size()  << std::endl ;
    }
    //apply the detected slot count and width to each strip with good width
    
    //generate new lines bases solely on the best width and slot count
    float f_mean_slot_width = widest_strip_rect.width * 1.0 / num_slots ;
    float slot_pos = 0.0 ;

    std::vector<int> standard_separators ;

    standard_separators.push_back(0) ;

    for(size_t i = 0; i < num_slots ; i++) {
        slot_pos += f_mean_slot_width ;
        auto i_pos = int(slot_pos) ;
        standard_separators.push_back(i_pos) ;
    }

    if(cmdopt_perspective) {
        const auto perspective_factor = 0.95 ;
        auto midpoint = src_gray.cols / 2 ;
        
        std::transform(standard_separators.begin(), standard_separators.end(), standard_separators.begin(),
            [perspective_factor, midpoint](int m) -> int {
                auto offset = midpoint - m ;
                auto new_offset = offset * perspective_factor ;
                return midpoint - new_offset ;
            }) ;
    }

    //Build the rects for each strip, from the calculated separators
    std::vector<std::vector<Rect> > drink_rect_rows, price_rect_rows ;
    std::vector<Rect> pricetag_strips ;

    const auto rc_image = Rect(Point(0, 0), src.size()) ;
    
    //HACK
    // const int pricetag_height = 32 ;
    int pricetag_height = f_mean_slot_width / LABEL_ASPECT ;

    for(const auto &strip : strips_with_good_width) {
        //set the left edge of the row rect to the widest detected strip, not this strip
        Point row_corner = Point(widest_strip_rect.x, strip->rc().y) ;
        std::vector <Rect> rects_drinks, rects_prices ;
        build_row_rects(standard_separators, strip->slots_height(), row_corner, rects_drinks, rects_prices) ;	

        Rect rc_price_strip = Rect(row_corner, Size(strip->rc().width, pricetag_height)) - Point(0, pricetag_height) ;
        pricetag_strips.push_back(rc_price_strip) ;

        transform(rects_drinks.begin(), rects_drinks.end(), rects_drinks.begin(),
            [rc_image](Rect rc) { return (rc & rc_image) ; }) ;
        drink_rect_rows.push_back(rects_drinks) ;
        price_rect_rows.push_back(rects_prices) ;
    }
    
    //now use the price tags to measure the spacing(
    std::vector<Mat> price_strip_images ;

    for(const auto rc : pricetag_strips) {
        Mat strip_image = src(rc) ;
        cvtColor(strip_image, strip_image, COLOR_BGR2GRAY) ;
        Mat intensity_image, thresh_image ;
        reduce(strip_image, intensity_image, 0, REDUCE_AVG) ; 
	    blur(intensity_image, intensity_image, Size(3, 3)) ;

        std::vector<int> levels = intensity_image.row(0) ;

        auto result = std::minmax_element(levels.begin(), levels.end()) ;
        int min_idx = result.first - levels.begin() ;
        int max_idx = result.second - levels.begin() ;
        int max_level = levels[max_idx] ;
        int min_level = levels[min_idx] ;

        int slot_separator_threshold = (max_level + min_level) / 2  ;
        if(cmdopt_verbose) {
            // std::cout << "Max:" << max_level << " Min:" << min_level << " Threshold: " << slot_separator_threshold << std::endl ;
        }

        threshold(intensity_image, thresh_image, slot_separator_threshold, 255, THRESH_BINARY) ;
        resize(thresh_image, thresh_image, Size(), 1, 32) ;
        price_strip_images.push_back(thresh_image) ;
    }


    if(cmdopt_trim_to_container) {
        std::vector<std::vector<Rect> > trimmed_drink_rect_rows ;

        //Trim the slot rectangles to the container based on the background and side edges
        //detected in the image
        for(const auto &rect_row : drink_rect_rows) {
            std::vector <Rect> rects_row ;
            for(auto rc : rect_row) {
                Mat img_slot = src_gray(rc) ;
                Mat detected_corners ;
                Rect rc_trimmed = get_trim_rect(img_slot, detected_corners) ;

                Rect rc_trimmed_on_full_image = 
                    rc_trimmed + rc.tl() ;
                rects_row.push_back(rc_trimmed_on_full_image) ;
            }
            trimmed_drink_rect_rows.push_back(rects_row) ;
        }            
        drink_rect_rows = trimmed_drink_rect_rows ;
    }


    if(cmdopt_verbose) {
        std::cout << "Primary processing done" << std::endl ;
        std::cout << "---------------------------" << std::endl  ; 
    }

    std::stringstream cnf ;
    cnf << num_slots << " x " << strips_with_good_width.size() ;
    const char *cnfstr = cnf.str().c_str() ; //yikes this is ugly

    //TODO: the price tags do not need to be sliced into rect, and would be more useful as a full strip.

    if(cmdopt_write_files) {
        if(cmdopt_verbose) {
            std::cout << "Writing container slot images" << std::endl ;
        }
        write_slot_image_files(src, drink_rect_rows, infilepath, "dr000_") ;

        if(cmdopt_verbose) {
            std::cout << "Writing price slot images" << std::endl ;
        }
        write_slot_image_files(src, price_rect_rows, infilepath, "pr000_") ;
    }

    // write_strip_image_file(src())

    //do all displaying here
    #ifdef USE_GUI
    if(!cmdopt_batch) {
        // show_result_images(src, strips, drink_rect_rows, price_rect_rows, price_strips, cnfstr) ;
        show_result_images(src, strips, drink_rect_rows, {}, pricetag_strips, cnfstr) ;
        imshow(strtitle.str(), scale_for_display(img_thresh)) ;

        int i = 0 ;
        for(auto img : price_strip_images) {
            std::stringstream title ;
            title << "Price tag " << i++ ;
            //imshow(title.str(), scale_for_display(img)) ;
        }
    }
    #endif

    //the return value contains the configuration
    std::cout << "Detected configuration: " << num_slots << "x" << strips_with_good_width.size()  << std::endl ;
    auto config_return_val = (10 * num_slots) + strips_with_good_width.size() ; 
    return config_return_val ;
    
} //end of process_file

// int strip_detect
void build_row_rects(const std::vector<int> sep_lines, int height, Point pt_strip_tl, std::vector<Rect> &rects_drinks, std::vector<Rect> &rects_prices) {
    int last_sep = sep_lines[0] ;
    
    // std::vector<Rect> row_rects = std::vector<Rect>() ;
    
    for(const auto xsep : sep_lines) {
        int width = xsep - last_sep ;
        if (width == 0) {
            continue ;
        }
        Point pt1 = Point(xsep + pt_strip_tl.x, 0 + pt_strip_tl.y) ;//bottom right
        Rect rc = Rect(pt1, Size(width, height)) + Point(-1 * width, -1 * height) ;

        //std::cout << "Created rect: " << rc << std::endl ;
        

        int bottom_label_height = rc.width / LABEL_ASPECT ;
        rc.height = rc.height - bottom_label_height ;
        
        Rect rc_label = Rect(pt1, Size(width, bottom_label_height)) + Point(-1 * width, -1 * bottom_label_height) ;
        
        //this shouldn't be necessary; the lines are within the bounds of the image
        //rc = rc & Rect(0, 0, src.size().width, src.size().height) ;
        
        if(!drink_rect_is_valid(rc)) {
            continue ;
        }
        rects_drinks.push_back(rc) ;
        rects_prices.push_back(rc_label) ;

        last_sep = xsep ;
    }

    // return row_rects ;
}


/* Various filters to determine if a detected rectangl accurately represents and actual drink
*/
bool drink_rect_is_valid(Rect rc) {
    if(rc.width < 20) { return false ; }
    if(rc.height > rc.width * 10) { return false ; }
    if(rc.width > rc.height * 2) { return false ; }
    if(rc.area() == 0) { return false ; }
   
    return true ;
}

/* Step decreasingly through threshold values until contours are detected
*/
int detect_button_strip_contours_stepped(Mat src, std::vector<std::vector<Point> >& contours, int initial_threshold) {
    if(cmdopt_verbose) {
        std::cout << "Trying to detect strips by stepping through threshold range." << std::endl ;
    }

    // const int STRIP_DETECTION_THRESH_MAX = 240 ;//higher that this and it will probably be too thin
    // const int STRIP_DETECTION_THRESH_MIN = 210 ;//give up if we get nothing even this low, the image is too dark overall
    int STRIP_DETECTION_THRESH_MAX = initial_threshold - 1 ;//higher that this and it will probably be too thin
    const int STRIP_DETECTION_THRESH_MIN = 192 ;//give up if we get nothing even this low, the image is too dark overall
    const int STRIP_DETECTION_THRESH_STEP = 5 ;


    int strip_detection_thresh = initial_threshold ;
    // bool is_strip_detected = false ;

    for(strip_detection_thresh  = STRIP_DETECTION_THRESH_MAX ;
        strip_detection_thresh >= STRIP_DETECTION_THRESH_MIN ;
        strip_detection_thresh -= STRIP_DETECTION_THRESH_STEP) {
        find_button_strip_contours(src, contours, strip_detection_thresh) ;
        if(contours.size() > 1) {
            // is_strip_detected = true ;

            //add bias and detect again
            strip_detection_thresh -= STRIP_DETECTION_THRESH_BIAS ;
            find_button_strip_contours(src, contours, strip_detection_thresh) ;

            if(cmdopt_verbose) {
                std::cout << "Detected " << contours.size() << "button strips using stepped threshold with bias " << strip_detection_thresh << std::endl ;
            }
            break ;
        }
        if(cmdopt_verbose) {
            std::cout << "Could not detect button strips at threshold " << strip_detection_thresh << std::endl ;
        }
    }

    return strip_detection_thresh ;
}
/* Find the button strip contours.
@param Mat image : to detect button contours on 
@param vector<vector<Point> >& button_strip_contours : destination for found contours
@param int thresh_val : threshold for detecting buttons
*/
void find_button_strip_contours(Mat src_gray, std::vector<std::vector<Point> >& button_strip_contours, int thresh_val) {
    Mat src_thresh ;
    std::vector<std::vector<Point> > found_contours;
    std::vector<Vec4i> hierarchy;

    threshold(src_gray, src_thresh, thresh_val, 255, THRESH_BINARY) ;
    //240

    auto strips_thresh = src_thresh.clone() ;
    strips_thresh = 0 ;
    
    //detect the contours
    
    findContours(src_thresh, found_contours, hierarchy, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE, Point(0, 0));
    
    // Mat img_contours = Mat::zeros(src_gray.size(), CV_8UC3);
    // std::vector <Rect> button_strip_bounds ;
        
    //select contours which resemble button strips
    std::copy_if(found_contours.begin(), found_contours.end(), back_inserter(button_strip_contours),
	    [src_thresh](const std::vector<Point> cont) {
            Rect bounds = boundingRect(cont) ;

            const int CONTOUR_MIN_POINTS = 200 ;
            const int CONTOUR_MIN_ASPECT = 3 ;
            const int CONTOUR_WIDTH_FRACTION = 10 ;//contour at least 1/10 width of image
            const int CONTOUR_DISTANCE_TOP_FRACTION = 15 ;//distance from top at least 1/15 of image height  
            
            return
                (cont.size() > CONTOUR_MIN_POINTS) &&
                (bounds.width > (bounds.height * CONTOUR_MIN_ASPECT)) &&
                (bounds.width > src_thresh.cols / CONTOUR_WIDTH_FRACTION) &&
                (bounds.y > src_thresh.rows / CONTOUR_DISTANCE_TOP_FRACTION) &&
                true ;
	    }) ;
}

/* This is for strips that are the full width but the slot separators fade away on the ends.
    We calculate the number of slots based on the strip width.
    It will add one more slot in that case.
    Not sure what we want to do with the results.
    */
std::vector<int> adjusted_slot_counts(std::vector<std::shared_ptr<ButtonStrip> > strips) {
    if(cmdopt_verbose) {
        std::cout << "Refine slot count based on high-Periodicity strips (detected, adjusted for strip width): " << std::endl  ;
    }

    std::vector<int> adjusted ;

    for(const auto &strip : strips) {
        auto sep_lines = strip->slot_separators() ;
        auto num_slots = sep_lines.size() - 1 ;
        
        auto slots_span = sep_lines[sep_lines.size() - 1] - sep_lines[0];
        int adjusted_num_slots = round(strip->rc().width * num_slots * 1.0 / slots_span) ;

        adjusted.push_back(adjusted_num_slots) ;
        
        // int mean_slot_width = slots_span / num_slots ;
    }

    return adjusted ;

}
