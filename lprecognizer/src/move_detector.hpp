#pragma once

#include <opencv2/opencv.hpp>
#include "cJSON.h"
#include "motion_detector.hpp"

//#define NUM_FRAME_END_DETECTION 3
//#define NUM_FRAME_START_DETECTION 2

namespace IZ {

class MoveDetector_Config
{
public:
    MoveDetector_Config() {
        bs_roi = cv::Rect2f(0.4, 0.2, 0.2, 0.6);
    }

    int bs_work_width_frame = 320;
    cv::Rect2f bs_roi;
    float bs_divider_brightness = 50;
    int bs_gaussian_blur_size = 11;
    int bs_median_blur_size = 21;
    float bs_accumulate_weight = 0.01;
    float bs_mul_threshold_difference = 7;
    float bs_min_threshold_difference = 5;
    int bs_min_width_object = 10;
    int bs_min_height_object = 10;
    int bs_number_first_frame_detection = 2;
    int bs_number_last_frame_detection = 3;
};

class MoveDetector :public MotionDetector
{
public:
    MoveDetector(const MoveDetector_Config & conf);
    MoveDetector(cJSON* json);
    ~MoveDetector(){}

    static void ParseConfig(cJSON* json_sub, MoveDetector_Config & cfg);
    std::shared_ptr<IZ::Event> Detection(const cv::Mat & input, int64_t timestamp);

    MoveDetector_Config cfg;

    cv::Mat firstFrame;
    cv::Mat avg;
    cv::Mat avg_bg;
    bool fPaused;
    bool fDetected;
    bool fStartDetected;
    bool fResult;

    std::vector<ResultMotion> endDetectionFrame;
    std::vector<ResultMotion> startDetectionFrame;
    cv::Mat thresh;
    cv::Mat mask;
    cv::Mat delta;

    int endDetectionIndex;
    int startDetectionIndex;
    int countFrameDetection;
    cv::Scalar avrBrightness;

    float minThr;
    float koeffBrightness;





protected:
    cv::Mat gray;
    cv::Mat gray_bg;
    cv::Mat frame;
};
}
