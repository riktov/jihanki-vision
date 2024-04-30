/**
 * @file detect.hpp
 * @author Paul Richter (paul@sagasoda.com)
 * @brief Routines for detecting the best four lines to perform a perspective transform
 * @version 0.1
 * @date 2024-04-11
 * 
 * @copyright Copyright (c) 2024
 * 
 */

void detect_dense_areas(cv::Mat img_edges, cv::Mat &img_out) ;
void detect_dense_areas2(cv::Mat img_edges, cv::Mat &img_out) ;
void detect_dense_areas_simple(cv::Mat img_edges, cv::Mat &img_out) ;
void detect_lines(cv::Mat img_cann, std::vector<cv::Vec4i> &lines, int accum = 300, int strip_offset = 0) ;
