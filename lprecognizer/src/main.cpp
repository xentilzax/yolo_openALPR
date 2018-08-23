#include <fstream>
#include <iostream>
#include <iomanip> 
#include <streambuf>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>

#include "alpr.h"
#include "yolo_v2_class.hpp"

#include "common.hpp"
#include "cJSON.h"
#include "post.hpp"

Config cfg;

//---------------------------------------------------------------------------------------------------------------
int ParseConfig(const std::string & str, Config & cfg);


//---------------------------------------------------------------------------------------------------------------
//Example run: ./demo cfg/yolov2-tiny-obj.cfg yolo-voc.weights cfg/open_alpr.conf images/1.jpg
void help()
{
    std::cout << "Need setting parameters for run application:" << std::endl;
    std::cout << "test_app <NN config file> <NN weights file> <open_alpr.conf file> <image file>" << std::endl;
    exit(1);
}

//---------------------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    std::string config_filename = "cfg/lprecognizer.cfg";
    std::ifstream fs(config_filename);
    if( !fs.good() ) {
        std::cout << "Error: Can't open config file: " << config_filename << std::endl;
        return 0x0A;
    }
    std::string str((std::istreambuf_iterator<char>(fs)),
                    std::istreambuf_iterator<char>());

    std::cout << "JSON config: " << str << std::endl;
    std::cout << std::flush;
    ParseConfig(str, cfg);

    if(curl_global_init(CURL_GLOBAL_ALL)) {
        fprintf(stderr, "Fatal: The initialization of libcurl has failed.\n");
        return EXIT_FAILURE;
    }

    if(atexit(curl_global_cleanup)) {
        fprintf(stderr, "Fatal: atexit failed to register curl_global_cleanup.\n");
        curl_global_cleanup();
        return EXIT_FAILURE;
    }

    if ( !cfg.use_yolo_detector ) {
        cfg.open_alpr_cfg = "";
    }

    alpr::Alpr openalpr(cfg.open_alpr_contry, cfg.open_alpr_cfg);
    // Optionally, you can specify the top N possible plates to return (with confidences). The default is ten.
    openalpr.setTopN(5);
    // Optionally, you can provide the library with a region for pattern matching. This improves accuracy by
    // comparing the plate text with the regional pattern.
    openalpr.setDefaultRegion(cfg.open_alpr_region);

    // Make sure the library loads before continuing.
    // For example, it could fail if the config/runtime_data is not found.
    if (openalpr.isLoaded() == false) {
        std::cerr << "Error loading OpenALPR" << std::endl;
        return 1;
    }

    std::shared_ptr<Detector> detector;//(cfg.yolo_cfg, cfg.yolo_weights);
    if ( cfg.use_yolo_detector ) {
        detector = std::shared_ptr<Detector>(new Detector(cfg.yolo_cfg, cfg.yolo_weights));
    }

    if ( cfg.gui_enable )
        cv::namedWindow("video stream",cv::WINDOW_NORMAL);

    int count_images = 0;
    int count_found_LP =0;
    int count_recognize_LP =0;
    int count_fail_frame = 0;
    cv::Mat img;
    cv::Mat img_roi;
    
    cv::VideoCapture capture;

    float avr_dtime = 0;

    while (1) {
        if(cfg.camera.empty()) {
            capture.open(0);
        } else {
            capture.open(cfg.camera);
        }

        if ( !capture.isOpened() ) {
            std::cerr << "Error: Can't open video stream from: " << cfg.camera << std::endl;
            return -1;
        }

        while ( capture.isOpened() ) {
            //read the current frame
            clock_t begin_time = clock();
            clock_t end_time;

            if(!capture.read(img)) {
                count_fail_frame++;
                std::cout << "fail frame: " << count_fail_frame << std::endl;
                if(count_fail_frame > 10) {
                    count_fail_frame = 0;
                    std::cout << "Reopen video stream" << std::endl;
                    break;
                }
                continue;
            }
            count_images++;

            std::vector<bbox_t> result_vec;
            std::vector<alpr::AlprRegionOfInterest> roi_list;
            std::vector<alpr::AlprResults> results;

            if ( cfg.use_yolo_detector ) { // use YOLO detector LP
                result_vec = detector->detect(img, cfg.yolo_thresh);

                for(size_t i = 0; i < result_vec.size(); i++) {
                    bbox_t b = result_vec[i];
                    img(cv::Rect(b.x, b.y, b.w, b.h)).copyTo(img_roi);

                    // Recognize an image file. Alternatively, you could provide the image bytes in-memory.
                    alpr::AlprResults result;
                    result = openalpr.recognize((unsigned char*)(img_roi.data), img_roi.channels(), img_roi.cols, img_roi.rows, roi_list);
                    results.push_back(result);
                }//for

                if ( cfg.gui_enable ) {
                    for(size_t i = 0; i < result_vec.size(); i++) {
                        bbox_t b = result_vec[i];
                        cv::rectangle(img, cv::Rect(b.x, b.y, b.w, b.h), cv::Scalar(0, 0, 255), 2);
                    }
                }
            } else { //use Open_ALPR detector LP
                alpr::AlprResults result;
                result = openalpr.recognize((unsigned char*)(img.data), img.channels(), img.cols, img.rows, roi_list);
                results.push_back(result);

                if ( cfg.gui_enable ) {
                    cv::Point points[4];
                    const cv::Point* pts[1] = {points};
                    int npts[1] = {4};

                    for(size_t i = 0; i < result.plates.size(); i++) {
                        for(size_t j = 0; j < 4; j++) {
                            points[j].x = result.plates[i].plate_points[j].x;
                            points[j].y = result.plates[i].plate_points[j].y;
                        }
                        cv::polylines(img, pts, npts, 1, true, cv::Scalar(0, 0, 255), 2);
                    }
                }
            }//end IF(yolo)

            end_time =clock();

            if ( cfg.verbose_level >= 2 ) {
                for (size_t j = 0; j < results.size(); j++) {
                    // Carefully observe the results. There may be multiple plates in an image,
                    // and each plate returns the top N candidates.
                    for (size_t i = 0; i < results[j].plates.size(); i++)
                    {
                        alpr::AlprPlateResult plate = results[j].plates[i];
                        if( plate.topNPlates.size() > 0) {
                            alpr::AlprPlate candidate = plate.topNPlates[0];
                            std::cout << "plate " << i << " : " << candidate.characters << std::endl;
                        } else {
                            std::cout << "Not found license plates" << std::endl;
                        }
                    }
                }
            }

            count_found_LP = 0;
            count_recognize_LP = 0;
            for (size_t j = 0; j < results.size(); j++) {
                for (size_t i = 0; i < results[j].plates.size(); i++)
                {
                    count_found_LP++;
                    alpr::AlprPlateResult plate = results[j].plates[i];
                    if ( plate.topNPlates.size() > 0) {
                        count_recognize_LP++;
                        std::string jsonResults = alpr::Alpr::toJson(results[j].plates[i]);

                        if (!PostHTTP(cfg.server, jsonResults)) {
                            fprintf(stderr, "Fatal: PostHTTP failed.\n");
                            return EXIT_FAILURE;
                        }
                    }
                }
            }


            if ( cfg.gui_enable ) {
                cv::imshow("video stream", img);
                if (cv::waitKey(1) >= 0)
                    return 0;
            }

            float cur_dtime = float(end_time - begin_time) / CLOCKS_PER_SEC;
            avr_dtime = 0.1 * cur_dtime + 0.9 * avr_dtime;

            if ( cfg.verbose_level >= 1 ) {
                std::cout  << "Frames: " << count_images
                           << " detected plates: " << count_found_LP
                           << " recognized plates: " << count_recognize_LP
                           << " Avr.time: " <<  avr_dtime
                           << std::endl;
            }

        }//while
        capture.release();
    }

    if ( cfg.verbose_level >= 1 ) {
        std::cout << "\n==========================\n";
        std::cout << "total images: "<< count_images << std::endl;
        std::cout << "total detect lp: "<< count_found_LP << std::endl;
        std::cout << "total recognize lp: "<< count_recognize_LP << std::endl;
    }
}

//---------------------------------------------------------------------------------------------------------------
/*
{
  "yolo":
  {
      "neuralnet_config": "cfg/tiny_onebbox.cfg",
      "weights": "weights/tiny_onebbox_hik.weights",
      "threshold": 0.5
  },

  "open_alpr":
  {
      "config": "cfg/open_alpr.conf",
  },

  "http_server": "http://jsonplaceholder.typicode.com/posts",
  "camera": "rtsp://admin:epsilon957@10.42.0.247",
  "gui": 0
}
*/

int ParseConfig(const std::string & str, Config & cfg)
{
    int status = 0;
    cJSON* json;
    cJSON* json_sub;
    cJSON* json_item;

    json = cJSON_Parse(str.c_str());
    if (json == NULL)
        goto end;

    //YOLO
    json_sub = cJSON_GetObjectItemCaseSensitive(json, "yolo");
    if (json_sub == NULL)
        goto end;

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "neuralnet_config");
    if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
        cfg.yolo_cfg = json_item->valuestring;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "weights");
    if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
        cfg.yolo_weights = json_item->valuestring;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "threshold");
    if (cJSON_IsNumber(json_item)) {
        cfg.yolo_thresh = json_item->valuedouble;
    }

    //Open_ALPR
    json_sub = cJSON_GetObjectItemCaseSensitive(json, "open_alpr");
    if (json_sub == NULL)
        goto end;

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "config");
    if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
        cfg.open_alpr_cfg = json_item->valuestring;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "contry");
    if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
        cfg.open_alpr_contry = json_item->valuestring;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "region");
    if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
        cfg.open_alpr_region = json_item->valuestring;
    }

    //LP Recognazer
    json_item = cJSON_GetObjectItemCaseSensitive(json, "use_yolo_detector");
    if (cJSON_IsNumber(json_item)) {
        cfg.use_yolo_detector = json_item->valueint;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json, "http_server");
    if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
        cfg.server = json_item->valuestring;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json, "camera");
    if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
        cfg.camera = json_item->valuestring;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json, "gui");
    if (cJSON_IsNumber(json_item)) {
        cfg.gui_enable = json_item->valueint;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json, "verbose_level");
    if (cJSON_IsNumber(json_item)) {
        cfg.verbose_level = json_item->valueint;
    }

    cJSON_Delete(json);
    return 1;

end:
    const char *error_ptr = cJSON_GetErrorPtr();
    if (error_ptr != NULL)
    {
        fprintf(stderr, "Error before: %s\n", error_ptr);
    }
    status = 0;

    cJSON_Delete(json);
    return status;
}
