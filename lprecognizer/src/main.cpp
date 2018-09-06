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
//---------------------------------------------------------------------------------------------------------------

#define AVR_W 0.1
#define VERSION 1.0
#define SITE_ID "INEX"

//---------------------------------------------------------------------------------------------------------------
namespace Inex
{
typedef alpr::AlprResults Results;
typedef alpr::AlprRegionOfInterest RegionOfInterest;
}
//---------------------------------------------------------------------------------------------------------------

Config cfg;

//---------------------------------------------------------------------------------------------------------------
int ParseConfig(const std::string & str, Config & cfg);
std::string ResultToJsonString(const Inex::Results & results,
                               const cv::Mat & img,
                               const std::vector<Inex::RegionOfInterest> & roi_list,
                               unsigned int camera_id);

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

    if ( cfg.use_yolo_detector ) {
        cfg.open_alpr_cfg = cfg.open_alpr_skip_cfg;
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

    std::shared_ptr<Detector> detector;
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

    float avr_time = 0;
    float avr_yolo = 0;
    float avr_alpr = 0;

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

            //read the current frame
            clock_t begin_time = clock();
            clock_t end_time, begin_yolo, begin_alpr, end_yolo, end_alpr;
            float yolo_cur_dtime = 0;
            float alpr_cur_dtime = 0;

            count_images++;

            std::vector<bbox_t> result_vec;
            std::vector<alpr::AlprRegionOfInterest> roi_list;
            alpr::AlprResults results;

            if ( cfg.use_yolo_detector ) { // use YOLO detector LP
                begin_yolo = clock();

                result_vec = detector->detect(img, cfg.yolo_thresh);

                end_yolo =clock();
                yolo_cur_dtime = float(end_yolo - begin_yolo) / CLOCKS_PER_SEC;

                begin_alpr = clock();

                for(size_t i = 0; i < result_vec.size(); i++) {
                    bbox_t b = result_vec[i];
                    img(cv::Rect(b.x, b.y, b.w, b.h)).copyTo(img_roi);

                    // Recognize an image file. Alternatively, you could provide the image bytes in-memory.
                    alpr::AlprResults roi_result;
                    roi_result = openalpr.recognize((unsigned char*)(img_roi.data), img_roi.channels(), img_roi.cols, img_roi.rows, roi_list);
                    results.plates.insert( results.plates.end(), roi_result.plates.begin(), roi_result.plates.end() );
                }//for

                end_alpr = clock();
                alpr_cur_dtime = float(end_alpr - begin_alpr) / CLOCKS_PER_SEC;

                if ( cfg.gui_enable ) {
                    for(size_t i = 0; i < result_vec.size(); i++) {
                        bbox_t b = result_vec[i];
                        cv::rectangle(img, cv::Rect(b.x, b.y, b.w, b.h), cv::Scalar(0, 0, 255), 2);
                    }
                }
            } else { //use Open_ALPR detector LP

                begin_alpr = clock();
                results = openalpr.recognize((unsigned char*)(img.data), img.channels(), img.cols, img.rows, roi_list);
                end_alpr = clock();
                alpr_cur_dtime = float(end_alpr - begin_alpr) / CLOCKS_PER_SEC;


                if ( cfg.gui_enable ) {
                    cv::Point points[4];
                    const cv::Point* pts[1] = {points};
                    int npts[1] = {4};
                    for(size_t i = 0; i < results.plates.size(); i++) {
                        for(size_t j = 0; j < 4; j++) {
                            points[j].x = results.plates[i].plate_points[j].x;
                            points[j].y = results.plates[i].plate_points[j].y;
                        }
                        cv::polylines(img, pts, npts, 1, true, cv::Scalar(0, 0, 255), 2);
                    }
                }
            }//end IF(yolo)

            end_time =clock();

            if ( cfg.verbose_level >= 2 ) {
                // Carefully observe the results. There may be multiple plates in an image,
                // and each plate returns the top N candidates.
                for (size_t i = 0; i < results.plates.size(); i++)
                {
                    std::cout << " plate_" << i << " : " << results.plates[i].bestPlate.characters << std::endl;
                }

            }

            count_found_LP = results.plates.size();
            count_recognize_LP = 0;


            if ( results.plates.size() > 0) {
                count_recognize_LP++;
                //std::string jsonResults = alpr::Alpr::toJson(results);
                std::string jsonResults = ResultToJsonString(results, img, roi_list, 0);

                if (!PostHTTP(cfg.server, jsonResults)) {
                    fprintf(stderr, "Fatal: PostHTTP failed.\n");
                    return EXIT_FAILURE;
                }
            }

            if ( cfg.gui_enable ) {
                cv::imshow("video stream", img);
                if (cv::waitKey(1) >= 0)
                    return 0;
            }

            float cur_dtime = float(end_time - begin_time) / CLOCKS_PER_SEC;
            avr_time = AVR_W * cur_dtime + (1 -AVR_W) * avr_time;

            if ( cfg.verbose_level >= 1 ) {
                if ( cfg.use_yolo_detector ) {
                    avr_yolo = AVR_W * yolo_cur_dtime + (1 - AVR_W) * avr_yolo;
                    avr_alpr = AVR_W * alpr_cur_dtime + (1 - AVR_W) * avr_alpr;


                    std::cout  << "yolo time: " << avr_yolo
                               << " alpr recognize time: " << avr_alpr
                               << std::endl;
                } else {
                    std::cout  << " alpr time: " << alpr_cur_dtime
                               << std::endl;
                }
                std::cout  << "Frames: " << count_images
                           << " detected plates: " << count_found_LP
                           << " recognized plates: " << count_recognize_LP
                           << " Avr.time: " <<  avr_time
                           << std::endl;
            }

        }//while
        capture.release();
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
  "camera": "rtsp://10.42.0.247",
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

    json_item = cJSON_GetObjectItemCaseSensitive(json_sub, "config_skip_detection");
    if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
        cfg.open_alpr_skip_cfg = json_item->valuestring;
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

//---------------------------------------------------------------------------------------------------------------
/*
 *
 *{
  "version": 2,
  "data_type": "alpr_results",
  "epoch_time": 1490574589596,
  "img_width": 1280,
  "img_height": 720,
  "processing_time_ms": 259.132385,
  "error": false,
  "regions_of_interest": [
    {
      "x": 60,
      "y": 600,
      "width": 620,
      "height": 120
    },
    {
      "x": 0,
      "y": 0,
      "width": 1280,
      "height": 570
    }
  ],
  "results": [
  {
  "plate": "4PEB029",
  "confidence": 94.94176483154297,
  "matches_template": 1,
  "plate_index": 0,
  "region": "us-ca",
  "region_confidence": 99,
  "processing_time_ms": 16.775239944458008,
  "requested_topn": 5,
  "coordinates": [
    {
      "x": 0,
      "y": 0
    },
    {
      "x": 203,
      "y": 0
    },
    {
      "x": 203,
      "y": 85
    },
    {
      "x": 0,
      "y": 85
    }
  ],
  "vehicle_region": {
    "x": 59,
    "y": 1,
    "width": 84,
    "height": 84
  },
  "candidates": [
    {
      "plate": "4PEB029",
      "confidence": 94.94176483154297,
      "matches_template": 1
    },
    {
      "plate": "4PEB29",
      "confidence": 81.52086639404297,
      "matches_template": 0
    },
    {
      "plate": "4PE029",
      "confidence": 81.5196762084961,
      "matches_template": 0
    },alpr::Alpr
    {
      "plate": "4PB029",
      "confidence": 81.51945495605469,
      "matches_template": 0
    },
    {
      "plate": "4PEBO29",
      "confidence": 81.39118957519531,
      "matches_template": 0
    }
  ]
}
  ],
  "uuid": "unspecified-cam16488027-1490574589596",
  "camera_id": 16488027,
  "agent_uid": "YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY",
  "agent_version": "2.3.111",
  "agent_type": "alprd",
  "site_id": "unspecified",
  "company_id": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
}
 * */

std::string ResultToJsonString(const Inex::Results & results,
                               const cv::Mat & img,
                               const std::vector<Inex::RegionOfInterest> & roi_list,
                               unsigned int camera_id)
{
    cJSON * json_root = cJSON_CreateObject();

    cJSON_AddItemToObject(json_root, "version", cJSON_CreateNumber(VERSION));
    cJSON_AddItemToObject(json_root, "epoch_time", cJSON_CreateString( std::to_string(results.epoch_time).c_str()));
    cJSON_AddItemToObject(json_root, "img_width", cJSON_CreateNumber( img.cols));
    cJSON_AddItemToObject(json_root, "img_height", cJSON_CreateNumber( img.rows));
    cJSON_AddItemToObject(json_root, "processing_time", cJSON_CreateNumber( results.total_processing_time_ms));
    cJSON_AddItemToObject(json_root, "error", cJSON_CreateBool( results.error));

    cJSON * json_regions_of_interest = cJSON_CreateArray();

    for(size_t i = 0; i < roi_list.size(); i++) {
        cJSON * json_item_array = cJSON_CreateObject();

        cJSON_AddItemToObject(json_item_array, "x", cJSON_CreateNumber( roi_list[i].x));
        cJSON_AddItemToObject(json_item_array, "y", cJSON_CreateNumber( roi_list[i].y));
        cJSON_AddItemToObject(json_item_array, "width", cJSON_CreateNumber( roi_list[i].width));
        cJSON_AddItemToObject(json_item_array, "height", cJSON_CreateNumber( roi_list[i].height));

        cJSON_AddItemToArray(json_regions_of_interest, json_item_array);
    }

    cJSON_AddItemToObject(json_root, "regions_of_interest", json_regions_of_interest);

    cJSON * json_results = cJSON_CreateArray();
    for (size_t j = 0; j < results.plates.size(); j++) {
        cJSON * json_item_array = cJSON_CreateObject();

        cJSON_AddItemToObject(json_item_array, "plate", cJSON_CreateString( results.plates[j].bestPlate.characters.c_str() ));
        cJSON_AddItemToObject(json_item_array, "confidence", cJSON_CreateNumber( results.plates[j].bestPlate.overall_confidence));
        cJSON_AddItemToObject(json_item_array, "matches_template", cJSON_CreateBool( results.plates[j].bestPlate.matches_template));
        cJSON_AddItemToObject(json_item_array, "plate_index", cJSON_CreateNumber(j));
        cJSON_AddItemToObject(json_item_array, "region", cJSON_CreateString( results.plates[j].region.c_str() ));
        cJSON_AddItemToObject(json_item_array, "region_confidence", cJSON_CreateNumber( results.plates[j].regionConfidence));
        cJSON_AddItemToObject(json_item_array, "processing_time_ms", cJSON_CreateNumber( results.plates[j].processing_time_ms));
        cJSON_AddItemToObject(json_item_array, "requested_topn", cJSON_CreateNumber( results.plates[j].requested_topn));

        cJSON * json_coords = cJSON_CreateArray();
        for (size_t k = 0; k < 4; k++) {
            cJSON * json_point = cJSON_CreateObject();
            cJSON_AddItemToObject(json_point, "x", cJSON_CreateNumber( results.plates[j].plate_points[k].x));
            cJSON_AddItemToObject(json_point, "y", cJSON_CreateNumber( results.plates[j].plate_points[k].y));
            cJSON_AddItemToArray(json_coords, json_point);
        }
        cJSON_AddItemToObject(json_item_array, "coordinates", json_coords);

        cJSON * json_vehicle_region = cJSON_CreateObject();
        {
            cJSON_AddItemToObject(json_vehicle_region, "x", cJSON_CreateNumber( results.plates[j].vehicle_region.x));
            cJSON_AddItemToObject(json_vehicle_region, "y", cJSON_CreateNumber( results.plates[j].vehicle_region.y));
            cJSON_AddItemToObject(json_vehicle_region, "width", cJSON_CreateNumber( results.plates[j].vehicle_region.width));
            cJSON_AddItemToObject(json_vehicle_region, "height", cJSON_CreateNumber( results.plates[j].vehicle_region.height));
        }
        cJSON_AddItemToObject(json_item_array, "vehicle_region", json_vehicle_region);

        cJSON * json_candidates = cJSON_CreateArray();
        for (size_t k = 0; k < results.plates[j].topNPlates.size(); k++) {
            cJSON * json_item = cJSON_CreateObject();
            cJSON_AddItemToObject(json_item, "plate", cJSON_CreateString( results.plates[j].topNPlates[k].characters.c_str() ));
            cJSON_AddItemToObject(json_item, "confidence", cJSON_CreateNumber( results.plates[j].topNPlates[k].overall_confidence));
            cJSON_AddItemToObject(json_item, "matches_template", cJSON_CreateBool( results.plates[j].topNPlates[k].overall_confidence));
            cJSON_AddItemToArray(json_candidates, json_item);
        }
        cJSON_AddItemToObject(json_item_array, "candidates", json_candidates);

        cJSON_AddItemToArray(json_results, json_item_array);
    }
    cJSON_AddItemToObject(json_root, "results", json_results);

    std::string uuid = std::string(SITE_ID) + "-cam" + std::to_string(camera_id) + "-" + std::to_string(results.epoch_time);

    cJSON_AddItemToObject(json_root, "uuid", cJSON_CreateString( uuid.c_str() ));
    cJSON_AddItemToObject(json_root, "camera_id", cJSON_CreateString( std::to_string(camera_id).c_str() ));
    cJSON_AddItemToObject(json_root, "site_id", cJSON_CreateString( SITE_ID ));

    std::string str = cJSON_Print(json_root);

    cJSON_Delete(json_root);

    return str;
}
