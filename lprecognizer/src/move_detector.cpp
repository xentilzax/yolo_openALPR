#include "move_detector.hpp"
#include <stdexcept>

using namespace IZ;

//-------------------------------------------------------------------------------------
MoveDetector::MoveDetector(const MoveDetector_Config & conf)
    :cfg(conf)
{
    fPaused = false;
    fDetected = false;
    fStartDetected = false;
    fResult = false;
    endDetectionIndex = 0;
    startDetectionIndex = 0;
    countFrameDetection = 0;
    endDetectionFrame.resize(cfg.bs_number_last_frame_detection);
    startDetectionFrame.resize(cfg.bs_number_first_frame_detection);
}

//-------------------------------------------------------------------------------------
void MoveDetector::ParseConfig(cJSON* json_sub, MoveDetector_Config & cfg)
{
    cJSON* json_item;

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "work_width_frame");
    if (cJSON_IsNumber(json_item)) {
        cfg.bs_work_width_frame = json_item->valueint;
    }

    cJSON* json_roi = cJSON_GetObjectItemCaseSensitive(json_sub, "roi");
    if (json_roi == NULL)
        throw std::invalid_argument("'roi' section in background_substraction config not found");
    {
        float x=0, y=0, width=0, height=0;
        json_item = cJSON_GetObjectItemCaseSensitive(json_roi, "x");
        if (cJSON_IsNumber(json_item)) {
            x = json_item->valuedouble;
        }

        json_item = cJSON_GetObjectItemCaseSensitive(json_roi, "y");
        if (cJSON_IsNumber(json_item)) {
            y = json_item->valuedouble;
        }

        json_item = cJSON_GetObjectItemCaseSensitive(json_roi, "width");
        if (cJSON_IsNumber(json_item)) {
            width = json_item->valuedouble;
        }

        json_item = cJSON_GetObjectItemCaseSensitive(json_roi, "height");
        if (cJSON_IsNumber(json_item)) {
            height = json_item->valuedouble;
        }

        cfg.bs_roi = cv::Rect2f(x, y, width, height);
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "divider_brightness");
    if (cJSON_IsNumber(json_item)) {
        cfg.bs_divider_brightness = json_item->valuedouble;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "gaussian_blur_size");
    if (cJSON_IsNumber(json_item)) {
        cfg.bs_gaussian_blur_size = json_item->valueint;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "median_blur_size");
    if (cJSON_IsNumber(json_item)) {
        cfg.bs_median_blur_size = json_item->valueint;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "accumulate_weight");
    if (cJSON_IsNumber(json_item)) {
        cfg.bs_accumulate_weight = json_item->valuedouble;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "mul_threshold_difference");
    if (cJSON_IsNumber(json_item)) {
        cfg.bs_mul_threshold_difference = json_item->valuedouble;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "min_threshold_difference");
    if (cJSON_IsNumber(json_item)) {
        cfg.bs_min_threshold_difference = json_item->valuedouble;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "min_width_object");
    if (cJSON_IsNumber(json_item)) {
        cfg.bs_min_width_object = json_item->valueint;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "min_height_object");
    if (cJSON_IsNumber(json_item)) {
        cfg.bs_min_height_object = json_item->valueint;
    }
}

//-------------------------------------------------------------------------------------
std::shared_ptr<IZ::Event> MoveDetector::Detection(const cv::Mat & img, int64_t timestamp)
{
    assert(timestamp > 0);

    if(img.cols <= 0 || img.rows <= 0)
        throw std::runtime_error("wrong image size");

    std::shared_ptr<IZ::EventMotionDetection> result = std::make_shared<EventMotionDetection>();

    int new_h = img.rows * cfg.bs_work_width_frame / img.cols;
    cv::Size sz(cfg.bs_work_width_frame, new_h);
    cv::resize(img, frame, sz);
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

    avrBrightness = cv::mean(gray);
    koeffBrightness = avrBrightness[0] / cfg.bs_divider_brightness;

    cv::Size szGaussian(cfg.bs_gaussian_blur_size, cfg.bs_gaussian_blur_size);

    //    cv::Mat grey_bg;
    cv::GaussianBlur(gray, gray , szGaussian, 0);
    cv::medianBlur(gray, gray_bg, cfg.bs_median_blur_size);

    if(gray_bg.type() != CV_32F)
        gray_bg.convertTo(gray_bg, CV_32F);
    if(gray.type() != CV_32F)
        gray.convertTo(gray, CV_32F);

    int w = frame.cols;
    int h = frame.rows;
    cv::Rect roi(cfg.bs_roi.x * w,
                 cfg.bs_roi.y * h,
                 cfg.bs_roi.width * w,
                 cfg.bs_roi.height * h);

    if( avg.empty() ) {
        avg = gray;
        avg_bg = gray_bg;
        mask = cv::Mat::zeros(sz, CV_8U);
        cv::Mat maskRoi = mask(roi);
        maskRoi.setTo(cv::Scalar(255));
    }

    cv::accumulateWeighted(gray, avg, cfg.bs_accumulate_weight);
    cv::accumulateWeighted(gray_bg, avg_bg, cfg.bs_accumulate_weight);

    gray = gray - gray_bg + avg_bg;

    cv::absdiff(gray, avg, delta);
    cv::convertScaleAbs(delta, delta);

    minThr = cfg.bs_mul_threshold_difference * koeffBrightness;
    if( minThr < cfg.bs_min_threshold_difference )
        minThr = cfg.bs_min_threshold_difference;

    cv::threshold(delta, thresh, minThr, 255, cv::THRESH_BINARY);
    cv::bitwise_and(thresh, mask, thresh);
    cv::dilate(thresh, thresh, cv::Mat(), cv::Point(-1, -1), /*iteration*/ 2);
    std::vector<std::vector<cv::Point> > contours;
    cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    //    cv::Mat tmp;
    //    frame.copyTo(tmp);
    //    cv::rectangle(tmp, roi, cv::Scalar(255,0,0));
    //    cv::imshow("main", tmp);
    //    cv::convertScaleAbs(gray, tmp);
    //    cv::imshow("gray", tmp);
    //    cv::convertScaleAbs(avg_bg, tmp);
    //    cv::imshow("avg_bg", tmp);

    //    cv::imshow("delta", mask);
    //    cv::imshow("thresh", thresh);
    //    cv::waitKey(1);

    for(size_t i = 0; i < contours.size(); i++) {
        cv::Rect bbox;
        bbox = cv::boundingRect(contours[i]);
        cv::Rect unionRect = bbox & roi;

        if( unionRect.width > cfg.bs_min_width_object && unionRect.height > cfg.bs_min_height_object) {
            fDetected = true;
        }
    }

    if( fStartDetected == true && fDetected == false ) {
        for(int i = 0; i < startDetectionIndex; i++) {
            assert (startDetectionFrame[i].timestamp >0);
            result->arrayResults.push_back(std::shared_ptr<Result>(
                                               new ResultMotion(startDetectionFrame[i])));
        }

        if( cfg.bs_number_last_frame_detection < countFrameDetection ) {
            countFrameDetection = cfg.bs_number_last_frame_detection;
        }

        int idx = endDetectionIndex - 1;
        for(int i = 0; i < countFrameDetection; i++) {
            int n = idx - i;
            while( n < 0 ) {
                n += cfg.bs_number_last_frame_detection;
            }
            assert (endDetectionFrame[n].timestamp >0);
            result->arrayResults.push_back(std::shared_ptr<Result>(
                                               new ResultMotion(endDetectionFrame[n])));
        }

        fResult = true;
    }

    if( fDetected ) {
        if( fStartDetected == false)
            fStartDetected = true;

        if( fStartDetected && startDetectionIndex < cfg.bs_number_first_frame_detection) {
            img.copyTo(startDetectionFrame[startDetectionIndex].frame);
            startDetectionFrame[startDetectionIndex].timestamp = timestamp;
            startDetectionIndex++;
        }

        img.copyTo(endDetectionFrame[endDetectionIndex].frame);
        endDetectionFrame[endDetectionIndex].timestamp = timestamp;
        endDetectionIndex++;
        countFrameDetection++;

        if(endDetectionIndex >= cfg.bs_number_last_frame_detection)
            endDetectionIndex = 0;

        fDetected = false;
    } else {
        fStartDetected = false;
        startDetectionIndex = 0;
        countFrameDetection = 0;
    }

    return std::static_pointer_cast<Event>(result);
}
