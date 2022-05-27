#if CV_VERSION_MAJOR >= 4
#include <opencv4/opencv2/highgui.hpp>
#include <opencv4/opencv2/imgproc.hpp>
#include <opencv4/opencv2/calib3d.hpp>
#else
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#endif

#include <iostream>
#include <fstream>
//#include <filesystem>


// #include <sys/stat.h>

#include <unistd.h>

#include <map>

#include "histogram.hpp"

using namespace cv ;

void process_file(std::string filename, HistogramDict hist_of) ;

Rect inner_third(Mat m) ;

double combined_correlation(std::vector<Mat> hist_set_base, std::vector<Mat> hist_set_model) ;

static const int region_weights[] = {
	2,
	1,
	1,
	2,
	1,
	1 
};

//global variables
char *infile ;
// const char *DEFAULT_MODEL_IMAGES_DIR = "images_drinks" ;
// const char *DEFAULT_MODEL_HISTOGRAMS_DIR = "images_drinks/models/histograms" ;
std::string models_dir = "images_drinks" ;
static const char *model_histograms_subdir = "histograms" ;
static const char *model_images_subdir = "images" ;
// const char *model_histograms_dir = DEFAULT_MODEL_HISTOGRAMS_DIR ;
// const char *model_images_dir = DEFAULT_MODEL_IMAGES_DIR ;
void print_csv(double corr, const struct model_data &model, std::string &filename) ;
void print_json(double corr, const struct model_data &model, std::string &filename) ;




static bool cmdopt_generate_histograms = false;
static bool cmdopt_verbose = false ;
static bool cmdopt_best = false ;
bool cmdopt_yaml = false ;
bool cmdopt_target_name = false ;
double min_correlation_to_display = -1.0 ;

void help() {
	std::cout << "identify_drinks" << std::endl ;
	std::cout << "  -b : show only best (maximum correlation)" << std::endl ;
	std::cout << "  -c correlation : minimum correlation to display" << std::endl ;
	std::cout << "  -g : generate histograms" << std::endl ;
	std::cout << "  -d dir : model images directory" << std::endl ;
	std::cout << "  -j dir : model histogram data subdirectory" << std::endl ;
	std::cout << "  -n : print target drink name in output line" << std::endl ;
	std::cout << "  -v : verbose" << std::endl ;
	std::cout << "  -y : use YAML histogram files" << std::endl ;
}

int main(int argc, char *argv[]) {
	char *cvalue = NULL ;
	
	char *filename ;
	char c ;

	if (argc < 2) {
		help() ;
		exit(0) ;
	}

	while((c = getopt(argc, argv, "bc:ghd:j:nvy")) != -1) {
		switch(c) {
			case 'b':
				cmdopt_best = true ;
				break ;
			case 'c':
				cvalue = optarg ;
				min_correlation_to_display = std::stod(cvalue) ;
				break ;
			case 'g':
				cmdopt_generate_histograms = true ;
				break;
			case 'h':
				help() ;
				exit(0) ;
			case 'd':
				cvalue = optarg ;
				models_dir = cvalue ;
				break ;
			case 'j':
				cvalue = optarg ;
				model_histograms_subdir = cvalue ;
				break ;
			case 'n':
				cmdopt_target_name = true ;
				break ;				
			case 'v':
				cmdopt_verbose = true ;
				break ;
			case 'y':
				cmdopt_yaml = true ;
				break ;
		}
	}
		
	HistogramDict model_of ;

	std::string histograms_dir, images_dir ;

	if(models_dir.back() != '/') {
		models_dir.append("/") ;
	}
	histograms_dir.append(models_dir).append(model_histograms_subdir) ;

	images_dir.append(models_dir).append(model_images_subdir) ;

	//just generate YAML histograms from image model files, no input
	if(cmdopt_generate_histograms) {
		model_of = load_model_images(images_dir) ;
		write_yaml_histograms(model_of, histograms_dir) ;
		exit(0) ;
	}

	if(optind >= argc) {
		std::cerr << "No input files." << std::endl ;
		exit(-1) ;
	} else {
		// std::cout << "Number of input files:" << (argc - optind) << std::endl ;
	}

	if(cmdopt_yaml) {
		model_of = load_model_histograms(histograms_dir) ;
		// model_of = load_model_histograms(histograms_dir, images_dir) ;
	} else {
		model_of = load_model_images(images_dir) ;
	}

	for (int idx = optind ; idx < argc ; idx++) {
		filename = argv[idx] ;

		// std::cout << filename << std::endl ;
		std::ifstream ifile(filename) ;
		if(ifile) {
			if(cmdopt_verbose) {
				std::cout << "Processing file:" << filename << std::endl ;
			}
			process_file(filename, model_of) ;    
		} else {
			std::cerr << "File does not exist: " << filename << std::endl ;
		}
	}

	return 0 ;
}




/**
 * Process file
 */
void process_file(std::string filename, const HistogramDict model_of) {
	const Mat src = imread(filename, 1) ; 
	if(src.empty()) {
		std::cerr << "Image is empty: " << filename << std::endl ;
	}

	if(cmdopt_verbose) {
		std::cout << "Loading target image: " << filename << std::endl ;
	}

	const auto any_model = model_of.begin()->second ;
	auto mat_rows = any_model.histograms[0].rows ;
	auto mat_cols = any_model.histograms[0].cols ;

	std::vector<Mat> target_histograms = generate_histogram_set(src, mat_rows, mat_cols) ;

	std::map<double, struct model_data> match_of ;

	double best_correlation = -1.0 ;
	struct model_data best_model ;

	for(const auto &pair : model_of) {	
		const std::string name = pair.first ;
		struct model_data model = pair.second ;
		
		const auto this_correlation = combined_correlation(target_histograms, model.histograms) ;
		// std::cout << this_correlation << " : " << pair.second.drink_name << " : " << pair.second.source_image_path << std::endl ;	
		match_of[this_correlation] = model ;	//std::map is automatically sorted by key

		if(this_correlation >= best_correlation) {
			best_correlation = this_correlation ;
			best_model = model ;
			// std::cout << "Best correlation so far: " << best_correlation << ", " << best_model.drink_name << std::endl ;
		}
	}

	if(cmdopt_best) {
		match_of.clear() ;
		match_of[best_correlation] = best_model ;
	}

	for(const auto &pair : match_of) {
		double corr = pair.first ;
		struct model_data model = pair.second ;

		if(corr > min_correlation_to_display) {
			// print_json(corr, model, filename) ;
			print_csv(corr, model, filename) ;
		}
	} 
}

/**
Generate a value representing the similarity of the target image to
the model image using histograms of several sub-regions of the image.
It is a weighted combination of the correlations of the histograms of the regions of the image
*/
double combined_correlation(const std::vector<Mat> hist_set_base, const std::vector<Mat> hist_set_model) {
	double corr, total = 0 ;
	const int compare_method = 0 ; //correlation
	int num_corrs = 0 ;
		
	//skip the first one which is already evaluated (with double weight)
	for(size_t i = 1 ; i < hist_set_base.size() ; i++) {
		const int weight = region_weights[i] ;
		const Mat h_base  = hist_set_base[i] ;
		const Mat h_model = hist_set_model[i];
		// std::cout << h_base.type() << ", " << h_model.type() << std::endl ;
		total += weight * compareHist(h_base, h_model, compare_method) ;
		num_corrs += weight ;
		}
		
	corr = total / num_corrs ;
	return corr ;
}

void print_csv(double corr, const struct model_data &model, std::string &filename) {
	const char sep = ',' ;

	std::cout << "\"" ;

	std::cout << corr << sep << model.drink_name << sep << model.drink_volume ;
	std::cout << sep << model.source_image_path ;
	if(cmdopt_target_name) {
		std::cout << sep << filename ;
	}
	std::cout << "\"" ;
	std::cout << std::endl ;
}

void print_json(double corr, const struct model_data &model, std::string &filename) {
	// const char sep = ',' ;

	std::cout << "{" ;

	std::cout << "correlation: " << corr << ", " ;
	std::cout << "model: { " ;
	std::cout << "drink_name: " << model.drink_name << ", " ; 
	std::cout << "drink_volume: " << model.drink_volume << ", " ; 
	std::cout << "source_image_path: " << model.source_image_path ;
	std::cout << " }" << ", " ;
	if(cmdopt_target_name) {
		std::cout << "filename: " << filename ;
	}
	std::cout << "}" ;
	std::cout << std::endl ;
}
