/**
 * module for histogram functions common to identify and generate
 * */

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

#include <map>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

#include "histogram.hpp"

static bool cmdopt_verbose = false ;
/**
Generate a histogram from an image
*/
Mat generate_histogram(const Mat img, int h_bins, int s_bins) {
	// const int h_bins = 50 ; ;
	// const int s_bins = 60 ;
	const int histSize[] = { h_bins, s_bins } ;

	const float h_ranges[] = { 0, 180 } ;
	const float s_ranges[] = { 0, 256 } ;

	const float* ranges[] = { h_ranges, s_ranges } ;

	const int channels[] = {0, 1} ;

	Mat hsv_base ;
	cvtColor(img, hsv_base, COLOR_BGR2HSV) ;

	Mat hist_base ;

	calcHist(&hsv_base, 1, channels, Mat(), hist_base, 2, histSize, ranges, true, false) ;
	// normalize(hist_base, hist_base, 0, 1, NORM_MINMAX, -1, Mat()) ;
	normalize(hist_base, hist_base, 0, 256, NORM_MINMAX, -1, Mat()) ;

	return hist_base ;
}

Mat generate_histogram_int(const Mat img, int h_bins, int s_bins) {
	// const int h_bins = 50 ; ;
	// const int s_bins = 60 ;
	const int histSize[] = { h_bins, s_bins } ;

	const float h_ranges[] = { 0, 180 } ;
	const float s_ranges[] = { 0, 256 } ;

	const float* ranges[] = { h_ranges, s_ranges } ;

	const int channels[] = {0, 1} ;

	Mat hsv_base ;
	cvtColor(img, hsv_base, COLOR_BGR2HSV) ;

	Mat hist_base ;

	calcHist(&hsv_base, 1, channels, Mat(), hist_base, 2, histSize, ranges, true, false) ;
	normalize(hist_base, hist_base, 0, 1, NORM_MINMAX, -1, Mat()) ;

	return hist_base ;
}
/**
 * Return a vector of five histograms of the following regions of the image:
Entire image, top half, bottom half, left half, right half
*/
std::vector<Mat> generate_histogram_set(const Mat img, int h_bins, int s_bins) {
	std::vector<Mat> histogram_set(num_histogram_regions) ;
		
	Rect rc_top = Rect(Point(0, 0), Size(img.cols, img.rows/ 2)) ;
	Rect rc_bottom = rc_top + Point(0, img.rows / 2) ;
	Rect rc_middle = rc_top + Point(0, img.rows / 4) ;
		
	Rect rc_left  = Rect(Point(0, 0), Size(img.cols / 2, img.rows)) ;
	Rect rc_right = rc_left + Point(img.cols / 2, 0) ;

	//order must match the labels array
	histogram_set[0] = generate_histogram(img, h_bins, s_bins) ;
	histogram_set[1] = generate_histogram(img(rc_top), h_bins, s_bins) ;
	histogram_set[2] = generate_histogram(img(rc_bottom), h_bins, s_bins) ;
	histogram_set[3] = generate_histogram(img(rc_middle), h_bins, s_bins) ;
	histogram_set[4] = generate_histogram(img(rc_left), h_bins, s_bins) ;
	histogram_set[5] = generate_histogram(img(rc_right), h_bins, s_bins) ;

	return histogram_set ;    
}

struct model_data load_model_image(std::string img_path) {
	Mat img ;

	img = imread(img_path, 1) ;

	std::cout << img_path << std::endl ;

	Rect rc_center = inner_third(img) ;

	std::vector<Mat> hist_set = generate_histogram_set(img, 10, 12) ;

	std::string fnamestr = std::string(img_path) ;
	size_t lastindex = fnamestr.find_last_of(".") ;
	std::string fnoext = fnamestr.substr(0, lastindex) ;
	// std::string filename_extension = fnamestr.substr(lastindex + 1) ;
	
	//model_of[fnoext] = hist_set ;

	struct model_data model ;
	model.histograms = hist_set ;
	model.drink_name = fnoext ;
	model.source_image_path = img_path ;

	return model ;
}

Rect inner_third(const Mat m) {
	const int x_margin = m.cols / 6 ;
	const int y_margin = m.rows / 6 ;

	return Rect(Point(x_margin, y_margin), Size(m.cols - (x_margin * 2), m.rows - (y_margin * 2))) ;
}

/**
Writes YAML files from image histograms generated from model images. 
Returns number of files written

*/
void write_yaml_histograms(const HistogramDict model_of, const std::string hist_dir) {
	for(const auto &pair : model_of) {
		const std::string name = pair.first ;
		struct model_data model = pair.second ;

		std::stringstream yamlfile_ss ;
		yamlfile_ss << hist_dir << "/" << name << ".yaml" ;
		const std::string yamlfile = yamlfile_ss.str() ;

		write_yaml_histogram(model, yamlfile) ;
	}
}

void write_yaml_histogram(struct model_data model, const std::string yamlfile, bool is_retain) {
	//If the YAML file already exists, first extract the name and volume to preserve them
	struct stat path_stat;
	stat(yamlfile.c_str(), &path_stat);

	if(is_retain) {
		if(S_ISREG(path_stat.st_mode)) {
			//Read the name and volume fields
			FileStorage fs(yamlfile, FileStorage::READ) ;

			std::string dn ;
			dn = std::string(fs["name"]) ;
			fs["volume"] >> model.drink_volume ;
			model.drink_name = dn ;

			fs.release() ;
		}
	}

	if(cmdopt_verbose) {
		std::cout << "Writing histogram data to " << yamlfile << std::endl ;
	}

	FileStorage fs(yamlfile, FileStorage::WRITE) ;

	fs << "name" << model.drink_name ;
	fs << "volume" << model.drink_volume ;
	fs << "image_file" << model.source_image_path ;

	const std::vector<Mat> hist_set = model.histograms ;
	for(size_t i = 0 ; i < num_histogram_regions ; i++) {
		fs << yaml_labels[i] << hist_set[i] ;
	}

	fs.release() ;
}

HistogramDict load_model_images(std::string models_dir) {
	DIR *pdir ;
	struct dirent *entry ;
	char *fname ;
		
	HistogramDict model_of ;

	if((pdir = opendir(models_dir.c_str()))) {
		while((entry = readdir(pdir))) {
			fname = entry->d_name ;
			std::string path = std::string(models_dir) + std::string("/") + fname ;
			
			struct stat path_stat;
			int ret = stat(path.c_str(), &path_stat);

			if(0 != ret) {
				std::cout << "stat() returned error " << errno << " on " << path << std::endl ;
				std::cout << "Skipping this file." << std::endl ;
				continue ;
			}

			if(S_ISREG(path_stat.st_mode)) {
				if(cmdopt_verbose) {
					std::cout << "Loading model image: " << path << std::endl ;
				}
			
				Mat img = imread(path, 1) ;
			
				Rect rc_center = inner_third(img) ;

				std::vector<Mat> hist_set = generate_histogram_set(img, 10, 12) ;

				std::string fnamestr = std::string(fname) ;
				size_t lastindex = fnamestr.find_last_of(".") ;
				std::string fnoext = fnamestr.substr(0, lastindex) ;
				std::string filename_extension = fnamestr.substr(lastindex + 1) ;
				
				//model_of[fnoext] = hist_set ;

				struct model_data model ;
				model.histograms = hist_set ;
				model.drink_name = fnoext ;
				model.source_image_path = fname ;
				// model_histograms.push_back(hist_full) ;
				// model_histogram_sets.push_back(hist_set) ;
				// model_names.push_back(fnoext) ;
				//model_of[fname] = model ;
				model_of[fnoext] = model ;
			} else {
				// std::cout << fname << " is not a file?!" << std::endl ;
			}
		}	//ended stepping through directory
	} else {
		std::cerr << "Model images directory does not exist: " << models_dir << std::endl ;
	}
	return model_of ;
}

/* Ideally we would like to save the model histograms on disk
as histograms so we don't have to regenerate them every time
*/
HistogramDict load_model_histograms(const std::string &models_dir) {
	DIR *pdir ;
	struct dirent *entry ;
	char *fname ;
		
	HistogramDict model_of ;

	if((pdir = opendir(models_dir.c_str()))) {
		while((entry = readdir(pdir))) {
			fname = entry->d_name ;
			std::string path = std::string(models_dir) + std::string("/") + fname ;
			struct stat path_stat;
			stat(path.c_str(), &path_stat);
			
			if(S_ISREG(path_stat.st_mode)) {
				std::string fnamestr = std::string(fname) ;
				size_t lastindex = fnamestr.find_last_of(".") ;
				std::string fnoext = fnamestr.substr(0, lastindex) ;
				std::string filename_extension = fnamestr.substr(lastindex + 1) ;
				
				if(filename_extension == std::string("yaml")) {
					if(cmdopt_verbose) {
						std::cout << "Loading YAML histogram from " << path << std::endl ;
					}
					FileStorage fs(path.c_str(), FileStorage::READ) ;

					//first, create a local histogram set to load from the FileStorage object
					std::vector<Mat> hist_set(num_histogram_regions) ;
					//load it
					for(size_t i = 0 ; i < num_histogram_regions; i++) {
						const char *label = yaml_labels[i] ;
						if(cmdopt_verbose) {
							std::cout << "Read from YAML: "<< label << std::endl ;
						}
						fs[label] >> hist_set[i] ;
					}

					//model_of[fnoext] = hist_set ;
					//now create a struct to hold the histogram set and other attributes
					struct model_data model ;

					// std::string dn = std::string(fs["name"]) ;
					// std::string img_pth = std::string(fs["image_file"]) ;

					fs["name"] >> model.drink_name ;
					fs["image_file"] >> model.source_image_path ;
					fs["volume"] >> model.drink_volume ;

					fs.release() ;
					


					// model.drink_name = dn ;
					// model.source_image_path = img_pth ;
					model.histograms = hist_set ;


					model_of[fnoext] = model ;
					/*
					model_histograms.push_back(hist_set[0]) ;
					model_histogram_sets.push_back(hist_set) ;
					model_names.push_back(fnoext) ;
					*/
				}
			}
		}
	} else {
		std::cerr << "Directory does not exist: " << models_dir << std::endl ;
	} 
	
	return model_of ;
}
