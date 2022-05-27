/* button_strip.hpp
 * 
 * Interface of the ButtonStrip class, which represents a strip of selection buttons
 * on a vending machine
 */

#ifndef BUTTON_STRIP_HPP
#define BUTTON_STRIP_HPP

/**
 * a button strip that supports methods for
 * calculating the slot width
 * */
class ButtonStrip {
private:
    cv::Mat m_image, m_img_intensity, m_img_thresh ;
    cv::Rect m_rect ;
    float m_slope = 0;
    //ButtonStrip *connected_to_right_of ;
    std::vector<size_t> m_runs;
    std::vector<int> m_slot_separators ;
    int m_slot_separator_threshold ;
    int m_strip_detection_threshold ;
    int m_slots_height ;
    cv::Mat m_img_filled ;
public:
    ButtonStrip(cv::Mat image, cv::Rect rc) ;
    
    //accessors
    //the monochrome image
    cv::Mat image() const { return m_image ; }
    //the rectangle of the strip on the full image
    cv::Rect rc() const { return m_rect ;}
    float slope() const { return m_slope ; }
    cv::Mat img_intensity() const { return m_img_intensity ; }
    cv::Mat img_thresh() const { return m_img_intensity ; }
    std::vector<size_t> runs() const { return m_runs ; }
    int slot_separator_threshold() const { return m_slot_separator_threshold ; }
    int slots_height() const { return m_slots_height ; }
    std::vector<int> slot_separators() const { return m_slot_separators ; }
    
    //setters
    void setImage(cv::Mat image) { m_image = image ; }
    void setRect(cv::Rect rc) { m_rect = rc ;}
    void setSlope(float slope) { m_slope = slope ; }
    void setSlotsHeight(int h) { m_slots_height = h ;}
    void setStripDetectionThreshold(int h) { m_strip_detection_threshold = h ;}
    void setSlotSeparators(std::vector<int> seps) { m_slot_separators = seps ; }
    
    //methods
    /**
     * Extend the image downward to encompass the entire height of the button strip, 
     * not just the detected highlight
     *
     */
    void xxxextendButtonImageDown(const cv::Mat src) ;
    void extendButtonImage(const cv::Mat src) ;
    /** 
     * Extend the image upward while clipping it at the top of the original image, 
     * effectively filling in the troughs of the wavy detected edge
     */
    void extendButtonImageUp(const cv::Mat src) ;
    /**
     * Create a run-length array for the thresholded average values  of the strip 
     */
    void generateSlotsRunLength() ;
    /**
     * Create an image of height 1 representing the mean intensity averaged vertically,
     * across the width of the strip
     */
    cv::Mat generateIntensityValuesLine() ;
    //void generateSlotSeparators() ;
    void generateDrinkSlots() ;
} ;

std::vector<std::shared_ptr<ButtonStrip> > merged_button_strips(std::vector<std::vector<cv::Point> > contours, int threshold) ;
std::vector<std::shared_ptr<ButtonStrip> > merged_button_strips_new(std::vector<std::vector<cv::Point> > contours, int threshold) ;

#endif

