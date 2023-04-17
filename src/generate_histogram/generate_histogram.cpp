/**
generate_histogram.cpp
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

using namespace cv ;

#include <iostream>
#include <unistd.h>
#include <libgen.h>	//basename

#include "histogram.hpp"

int process_file(std::string filepath) ;
std::pair<std::string, int> parse_model_image_filename(std::string path) ;
std::string outfile_name(const struct model_data model) ;

// static bool cmdopt_generate_histograms = false;
bool cmdopt_verbose = false ;
std::string dest_dir = "";
// static bool cmdopt_best = false ;
// bool cmdopt_yaml = false ;q
// bool cmdopt_target_name = false ;

void help() {
	std::cout << "generate_histogram" << std::endl ;
	std::cout << "  -d dir : output directory" << std::endl ;
	std::cout << "  -o file : output file" << std::endl ;
	// std::cout << "  -r : retain name and volume from existing file" << std::endl ;
	std::cout << "  -v : verbose" << std::endl ;
}

int main(int argc, char *argv[]) {
   	if (argc < 2) {
		help() ;
		exit(0) ;
	}


	std::string dest_file ;
   	char *cvalue = NULL ;
   	char c ;

    while((c = getopt(argc, argv, "d:hv")) != -1) {
		switch(c) {
			case 'd':
				cvalue = optarg ;
				dest_dir = cvalue ;
				break ;
			case 'h':
				help() ;
				exit(0) ;
			case 'v':
				cmdopt_verbose = true ;
				break ;
		}
	}

	if(optind >= argc) {
		std::cerr << "No input files." << std::endl ;
		exit(-1) ;
	}

    for(int i = optind ; i < argc ; i++) {
		std::string argstr = argv[i] ;
        process_file(argstr) ;
    }

    return 0 ;
}

int process_file(std::string filepath) {
    Mat src ;
	src = imread(filepath, 1) ; 

	if(src.empty()) {
		std::cerr << "The file is empty: " << filepath << std::endl ;
	}

	std::vector<Mat> hist_set ;

	try{
		hist_set = generate_histogram_set(src, 10, 12) ;
	} catch (cv::Exception &e) {
		std::cout << "In the exception catch" << std::endl ;
		std::cout << e.msg << std::endl ;
		throw e ;
	}

	struct model_data model ;

	auto name_and_volume = parse_model_image_filename(filepath) ;

	model.drink_name   = name_and_volume.first ;
	model.drink_volume = name_and_volume.second ;
	model.histograms   = hist_set ;

	char *c_filepath = strdup(filepath.c_str()) ;
	model.source_image_path = basename(c_filepath) ;
	free(c_filepath) ;

	/*
	std::string dest_path = outfile_name(model) ;
	return -1; 


	char *c_file_basename = strdup(filepath.c_str()) ;
	std::string file_basename = basename(c_file_basename) ;
	free(c_file_basename) ;

	auto idx_last_dot = file_basename.find_last_of('.') ;
	std::string bnb = file_basename.substr(0, idx_last_dot) ;
	*/

	std::string dest_path = (dest_dir.empty() ? "" : dest_dir + "/") + outfile_name(model) ;

	if(cmdopt_verbose) {
		std::cout << "Output file: " << dest_path << std::endl ;
	}
	write_yaml_histogram(model, dest_path) ;

	/*
	int i = 0 ;

    for(auto label : yaml_labels) {
        std::cout << label << std::endl ;
		std::cout << hist_set[i++] << std::endl ;
    }
	*/
    return 0 ;
}

/**
 * Returns the name of the drink and container volume, from the filename
 * */
std::pair<std::string, int> parse_model_image_filename(std::string path) {
	char *c_path = strdup(path.c_str()) ;
	std::string bfext = basename(c_path) ;
	free(c_path) ;
	
	auto last_dot = bfext.find_last_of('.') ;
	auto bf = bfext.substr(0, last_dot) ;

	std::string first_string ;
	std::string second_string = "0" ;

	//TODO: strip extension
	auto first_sep =  bf.find('_') ;

	first_string = (first_sep < 0) ? bf : bf.substr(0, first_sep) ;

	auto second_sep = bf.find('_', first_sep + 1) ;
	if(second_sep >= 0) {
		second_string = bf.substr(first_sep + 1, second_sep - first_sep - 1) ;
	}

	return std::make_pair(first_string, atoi(second_string.c_str())) ;
}

std::string outfile_name(struct model_data model) {
	auto stamp_val_0 = model.histograms[0].at<float>(0) ;
	auto stamp_val_1 = model.histograms[0].at<float>(1) ;
	std::string stamp = std::to_string(stamp_val_0) + std::to_string(stamp_val_1) ;
	stamp.erase(std::remove(stamp.begin(), stamp.end(), '.'), stamp.end());

	int stamp_i = atoi(stamp.c_str()) ;
	std::stringstream strhex ;
	strhex << std::hex << stamp_i ;
	return model.drink_name + "_" + std::to_string(model.drink_volume) + "_" + strhex.str() + ".yaml" ;
}