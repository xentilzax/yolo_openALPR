#include <ctime>
#include <fstream>
#include <iostream>
#include <iomanip> 
#include <streambuf>
#include <string>
#include <sstream>
#include <vector>

#include <opencv2/opencv.hpp>

#include "alpr.h"
#include "yolo_v2_class.hpp"

#include "config.hpp"
#include "post.hpp"
//---------------------------------------------------------------------------------------------------------------

#define AVR_W 0.1
#define VERSION 1.0
#define SITE_ID "unspecified"


//---------------------------------------------------------------------------------------------------------------
namespace Inex
{
typedef alpr::AlprResults Results;
typedef alpr::AlprRegionOfInterest RegionOfInterest;
}

//---------------------------------------------------------------------------------------------------------------

Inex::Config cfg;

//---------------------------------------------------------------------------------------------------------------

std::string ResultToJsonString(const Inex::Results & results,
                               const cv::Mat & img,
                               const std::vector<Inex::RegionOfInterest> & roi_list,
                               const std::string & camera_id,
                               const std::string & uuid);
bool SaveImage(const std::string & path, std::string & name, cv::Mat & img, const std::string & jsonText, int64_t time_id);
bool CheckExitsAndMakeDir(const std::string & file);


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
        if(cfg.camera_url.empty()) {
            capture.open(0);
        } else {
            capture.open(cfg.camera_url);
        }

        if ( !capture.isOpened() ) {
            std::cerr << "Error: Can't open video stream from: " << cfg.camera_url << std::endl;
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

		time_t t = time(NULL);
                results.epoch_time = static_cast<int64_t>(t);
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
                // file name struct: <cam_id-event_id-type_image>.jpg
                std::string uuid = cfg.camera_id + "-" + std::to_string(results.epoch_time) + "-" + cfg.camera_type;
                std::string img_filename = uuid;
                //std::string jsonResults = alpr::Alpr::toJson(results);
                std::string jsonResults = ResultToJsonString(results, img, roi_list, cfg.camera_id, uuid);

                if( !SaveImage(cfg.path, img_filename, img, jsonResults, results.epoch_time) ) {
                    results.error = true;
                }

                if (!PostHTTP(cfg.server, jsonResults, cfg.verbose_level)) {
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
bool SaveImage(const std::string & path, std::string & name, cv::Mat & img, const std::string & jsonText, int64_t time_id)
{
    //    time_t t = time(NULL);
    time_t t = static_cast<time_t>(time_id/1000);

    std::tm tm = *std::localtime(&t);

    if ( !CheckExitsAndMakeDir(path)) { std::cerr << "Can't open dir: " << path << std::endl; return false;}

    std::string year = std::to_string(tm.tm_year + 1900);
    std::string month = std::to_string(tm.tm_mon + 1);
    std::string day = std::to_string(tm.tm_mday);
    std::string hour = std::to_string(tm.tm_hour);

    std::string mon_day_yaer_dir = path + "/" + month + "_" + day + "_" + year;
    if ( !CheckExitsAndMakeDir(mon_day_yaer_dir)) { std::cerr << "Can't open dir: " << mon_day_yaer_dir << std::endl; return false;}

    std::string hour_dir = mon_day_yaer_dir + "/" + hour;
    if ( !CheckExitsAndMakeDir(hour_dir)) { std::cerr << "Can't open dir: " << hour_dir << std::endl; return false;}

    std::string event_dir = hour_dir + "/" + std::to_string(time_id);
    if ( !CheckExitsAndMakeDir(event_dir)) { std::cerr << "Can't open dir: " << event_dir << std::endl; return false;}

    //save image
    std::string nameImg = event_dir + "/" + name + ".jpg";
    cv::imwrite(nameImg, img);
    //save json to text file
    std::string nameJson = event_dir + "/" + name + ".txt";
    std::ofstream out(nameJson);
    out << jsonText;
    out.close();

    name = nameImg;

    return true;
}

//---------------------------------------------------------------------------------------------------------------
#include <sys/stat.h>
#include <iostream>

bool IsExists(const std::string & file)
{
    struct stat buf;
    return (stat(file.c_str(), &buf) == 0);
}

bool MakeDir(const std::string & dir)
{
    const int dir_err = mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    if (-1 == dir_err) {
        std::cerr << "Error: Can't create dir: " << dir << std::endl;
        return false;
    }

    return true;
}

bool CheckExitsAndMakeDir(const std::string & dir)
{
    if ( !IsExists( dir )) {
        if(cfg.verbose_level >= 1) {
            std::cout << "Create dir: " << dir << std::endl;
        }
        if ( !MakeDir( dir )) {
            return false;
        }
    }
    return true;
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
                               const std::string & camera_id,
                               const std::string & uuid)
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

    cJSON_AddItemToObject(json_root, "uuid", cJSON_CreateString( uuid.c_str() ));
    cJSON_AddItemToObject(json_root, "camera_id", cJSON_CreateString(camera_id.c_str() ));
    cJSON_AddItemToObject(json_root, "site_id", cJSON_CreateString( SITE_ID ));

    std::string str = cJSON_Print(json_root);

    cJSON_Delete(json_root);

    return str;
}
