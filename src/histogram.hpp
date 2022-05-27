#include <map>

//structs, typedefs, and classes
struct model_data {
	std::string source_image_path ;
	std::string drink_name ;
	int drink_volume = 0 ;
	std::vector<cv::Mat> histograms ;
} ;

static const char *yaml_labels[] = {
	"model_histogram_full_image",
	"model_histogram_top" ,
	"model_histogram_bottom",
	"model_histogram_middle",
	"model_histogram_left",
	"model_histogram_right" 
};



typedef std::map<std::string, struct model_data> HistogramDict ;

const int num_histogram_regions = sizeof(yaml_labels) / sizeof(*yaml_labels) ;

struct model_data load_model_image(std::string img_path) ;
cv::Mat generate_histogram(const cv::Mat img, int h_bins, int s_bins) ;
cv::Mat generate_histogram_int(const cv::Mat img, int h_bins, int s_bins) ;
std::vector<cv::Mat> generate_histogram_set(const cv::Mat img, int h_bins, int s_bins) ;
cv::Rect inner_third(const cv::Mat m) ;
void write_yaml_histograms(const HistogramDict hist_of, const std::string dir) ;
void write_yaml_histogram(struct model_data model, const std::string yamlfile, bool is_retain=false) ;
HistogramDict load_model_histograms(const std::string &models_dir) ;
HistogramDict load_model_images(std::string dir) ;