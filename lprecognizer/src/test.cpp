#include <ctime>
#include <fstream>
#include <iostream>
#include <iomanip> 
#include <streambuf>
#include <string>
#include <sstream>
#include <vector>
#include <chrono>
#include <getopt.h>

#include <opencv2/opencv.hpp>

//#include "yolo_v2_class.hpp"

#include "motion_detector.hpp"
#include "object_detector.hpp"
#include "object_recognizer.hpp"
#include "open_alpr_module.hpp"
#include "yolo_detector.hpp"
#include "simple_motion.hpp"

//---------------------------------------------------------------------------------------------------------------

#define AVR_W 0.1
#define VERSION 1.0
#define SITE_ID "unspecified"


//---------------------------------------------------------------------------------------------------------------
using namespace std::chrono;

using namespace IZ;

typedef alpr::AlprResults Results;
typedef alpr::AlprRegionOfInterest RegionOfInterest;

//---------------------------------------------------------------------------------------------------------------
class Conveer
{
public:
    std::string motionDetectorName;
    std::vector<std::string> motionDetectorAdapters;
    std::string detectorName;
    std::vector<std::string> detectorAdapters;
    std::string recognizerName;
    std::vector<std::string> recognizerAdapters;
};

//---------------------------------------------------------------------------------------------------------------
class Config :
        public ALPR_Config,
        public YOLO_Config,
        public SimpleMotion_Config
{
public:
    Conveer conveer;

    int use_yolo_detector = 1;
    std::string camera_url = "";
    std::string camera_id = "A";
    std::string camera_type = "default";
    int gui_enable = 0;
    int verbose_level = 0;
};

//---------------------------------------------------------------------------------------------------------------
class ConveerImpl
{
public:
    std::shared_ptr<IZ::MotionDetector> motionDetector;
    std::shared_ptr<IZ::ObjectDetector> detector;
    std::shared_ptr<IZ::LicensePlateRecognizer> recognizer;
};

struct CSVInputStruct
{
    std::string Record_Id;
    std::string Lane;
    std::string Event;
    std::string Image_Num;
    std::string Image_Type;
    std::string Path;
    std::string LP_Number;
    std::string LP_State;
    std::string LP_Type;
    std::string Is_Reviewed;
    std::string Is_Qualified;
    std::string NQ_Reason;
    std::string Color;
};

//---------------------------------------------------------------------------------------------------------------

Config cfg;

int ParseConfig(const std::string & str, Config & cfg);

////---------------------------------------------------------------------------------------------------------------
//void help() {
//    std::cout << "Use keys:" << std::endl
//              << "\t'i' input csv file"  << std::endl;
//}


////---------------------------------------------------------------------------------------------------------------
//void parser(int argc, char **argv) {
//    int arg;

//    while ((arg = getopt(argc, argv, "i:")) != -1) {
//        switch (arg) {
//        case 'i': csvFileName = optarg; break;
//        case '?': help();  exit(0);
//        default : help();  exit(1);
//        }
//    }
//}

//---------------------------------------------------------------------------------------------------------------
void GeCSVtItem(std::stringstream & ss, std::string & str)
{
    if(!std::getline(ss, str, ',')) {
        std::string errStr("Can't parse CSV line: " + ss.str());
        std::cerr << errStr << std::endl;
        throw std::runtime_error(errStr);
    }
}

//---------------------------------------------------------------------------------------------------------------
void ParseCSVLine(const std:: string & line, CSVInputStruct & csvInputData)
{
    std::stringstream ss(line);

    GeCSVtItem(ss, csvInputData.Record_Id);
    GeCSVtItem(ss, csvInputData.Lane);
    GeCSVtItem(ss, csvInputData.Event);
    GeCSVtItem(ss, csvInputData.Image_Num);
    GeCSVtItem(ss, csvInputData.Image_Type);
    GeCSVtItem(ss, csvInputData.Path);
    GeCSVtItem(ss, csvInputData.LP_Number);
    GeCSVtItem(ss, csvInputData.LP_State);
    GeCSVtItem(ss, csvInputData.LP_Type);
    GeCSVtItem(ss, csvInputData.Is_Reviewed);
    GeCSVtItem(ss, csvInputData.Is_Qualified);
    GeCSVtItem(ss, csvInputData.NQ_Reason);
    GeCSVtItem(ss, csvInputData.Color);

    std::replace(csvInputData.Path.begin(), csvInputData.Path.end(), '\\', '/');
}

//---------------------------------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    //    parser(argc, argv);

    std::string config_filename = "cfg/test.cfg";
    std::ifstream fs(config_filename);
    if( !fs.good() ) {
        std::cout << "Error: Can't open config file: " << config_filename << std::endl;
        return 0x0A;
    }
    std::string str((std::istreambuf_iterator<char>(fs)),
                    std::istreambuf_iterator<char>());

    std::cout << "JSON config: " << str << std::endl;
    std::cout << std::flush;

    if( ParseConfig(str, cfg) == 0) {
        return 0x0B;
    }

    ConveerImpl conveer;
    //define motion detector

    conveer.motionDetector = std::shared_ptr<IZ::MotionDetector>(new IZ::SimpleMotion(cfg));

    //define detector
    if ( cfg.conveer.detectorName ==  "yolo")
        conveer.detector = std::shared_ptr<IZ::ObjectDetector>(new IZ::YOLO_Detector(cfg));
    if ( cfg.conveer.detectorName ==  "open_alpr")
        conveer.detector = std::shared_ptr<IZ::ObjectDetector>(new IZ::ALPR_Module(cfg));

    //define recognizer
    if ( cfg.conveer.recognizerName ==  "open_alpr")
        conveer.recognizer = std::shared_ptr<IZ::LicensePlateRecognizer>(new IZ::ALPR_Module(cfg));

    cv::Mat img;

    if(cfg.camera_url.empty()) {
        std::cout << "Empty file name csv file" << std::endl;
        return -1;
    }

    std::ifstream infile(cfg.camera_url);
    std::string line;
    CSVInputStruct csvInputData;

    //get header
    std::getline(infile, line);

    std::cout  << "Event,Path,LP State,LP Type,LP Number,ALPR,ALPR Confidence,"
               << "ALPR State,ALPR State Confidence,ALPR Type,ALPR Type Confidence"
               << ",Microsec Spent"
               << std::endl;

    while (std::getline(infile, line)) {
        ParseCSVLine(line, csvInputData);

        std::string imageFileName = csvInputData.Path;

        img = cv::imread(imageFileName);
        if(img.rows == 0 || img.cols == 0) {
            std::cout << "Can't open file : " << imageFileName << std::endl;
            return -1;
        }

        auto beginTime = system_clock::now();

        std::shared_ptr<IZ::Event> event = conveer.motionDetector->Detection(img, 0);

        auto startDetectorTime = std::chrono::high_resolution_clock::now();

        event = conveer.detector->Detection(event);

        auto detectorTime = std::chrono::high_resolution_clock::now();

        event = conveer.recognizer->Recognize(event);

        auto recogniseTime = std::chrono::high_resolution_clock::now();

        if(event->arrayResults.size() != 1) {
            std::cerr << "RuntimeError: size array results != 1" << std::endl;
            return -1;
        }

        std::string lp_finded = "NOT_RECOGNIZED";

        if(!event->arrayResults[0]->IsEmpty()) {
            std::shared_ptr<IZ::ResultDetection> result = std::dynamic_pointer_cast<IZ::ResultDetection>(event->arrayResults[0]);

            for(auto obj : result->objectData) {
                std::shared_ptr<IZ::DataRecognition> recog = std::dynamic_pointer_cast<IZ::DataRecognition>(obj);
                if(recog->recognized) {
                    lp_finded = recog->bestPlate.characters;
                }
            }
        }

        /*Event,Path,LP State,LP Type,LP Number,ALPR,ALPR Confidence,ALPR State,ALPR State Confidence,ALPR Type,ALPR Type Confidence*/
        std::cout  << csvInputData.Event << ","
                   << csvInputData.Path << ","
                   << csvInputData.LP_State << ","
                   << csvInputData.LP_Type << ","
                   << csvInputData.LP_Number << ","
                   << lp_finded << "," //ALPR
                   << "0,"  //ALPR Confidence
                   << ","   //ALPR State
                   << "0,"  //ALPR State Confidence
                   << ","   //ALPR Type
                   << "0,"  //ALPR Type Confidence
                   << duration_cast<std::chrono::microseconds>(detectorTime - startDetectorTime).count()
                   << std::endl;

        if ( cfg.gui_enable ) {
            cv::Size sz(320, 240);
            for(size_t i = 0; i < event->arrayResults.size(); i++) {
                std::shared_ptr<IZ::ResultMotion> item = std::dynamic_pointer_cast<IZ::ResultMotion>(event->arrayResults[i]);

                cv::Mat eventFrame;
                cv::resize(item->frame, eventFrame, sz);
                cv::imshow(std::to_string(i), eventFrame);
            }
            cv::resize(img, img, sz);
            cv::imshow("video stream", img);
            if (cv::waitKey(1) >= 0)
                return 0;
        }
    }
}

//---------------------------------------------------------------------------------------------------------------
int ParseConfig(const std::string & str, Config & cfg)
{
    int status = 0;
    cJSON* json;
    cJSON* json_sub;
    cJSON* json_item;
    cJSON* json_arr;
    cJSON* json_recognizers;
    cJSON* json_detectors;
    cJSON* json_motion;
    cJSON* json_conveer;

    json = cJSON_Parse(str.c_str());
    if (json == NULL)
        goto end;

    //order load config sections impotant for Open_ALPR module!!!!
    json_recognizers = cJSON_GetObjectItemCaseSensitive(json, "recognizers");
    if (json_recognizers == NULL)
        goto end;
    {
        //Open_ALPR
        json_sub = cJSON_GetObjectItemCaseSensitive(json_recognizers, "open_alpr");
        if (json_sub == NULL)
            goto end;

        ALPR_Module::ParseConfig(json_sub, cfg);
    }

    json_detectors = cJSON_GetObjectItemCaseSensitive(json, "detectors");
    if (json_detectors == NULL)
        goto end;
    {
        //YOLO
        json_sub = cJSON_GetObjectItemCaseSensitive(json_detectors, "yolo");
        if (json_sub == NULL)
            goto end;

        YOLO_Detector::ParseConfig(json_sub, cfg);

        //Open_ALPR
        json_sub = cJSON_GetObjectItemCaseSensitive(json_detectors, "open_alpr");
        if (json_sub == NULL)
            goto end;

        ALPR_Module::ParseConfig(json_sub, cfg);
    }

    json_motion = cJSON_GetObjectItemCaseSensitive(json, "motion_detectors");
    if (json_motion == NULL)
        goto end;
    {
        //SIMPLE
        json_sub = cJSON_GetObjectItemCaseSensitive(json_motion, "simple");
        if (json_sub == NULL)
            goto end;

        SimpleMotion::ParseConfig(json_sub, cfg);
    }

    json_conveer = cJSON_GetObjectItemCaseSensitive(json, "conveer");
    if (json_motion == NULL)
        goto end;
    {
        json_item = cJSON_GetObjectItemCaseSensitive(json_conveer, "motion_detector_name");
        if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
            cfg.conveer.motionDetectorName = json_item->valuestring;
        }

        json_item = cJSON_GetObjectItemCaseSensitive(json_conveer, "motion_detector_adapters");
        if (cJSON_IsArray(json_item)) {
            for(int i = 0; i < cJSON_GetArraySize(json_item); i++) {
                json_arr = cJSON_GetArrayItem(json_item, i);
                json_sub = cJSON_GetObjectItemCaseSensitive(json_arr, "name");
                if (cJSON_IsString(json_sub) && (json_sub->valuestring != NULL)) {
                    cfg.conveer.motionDetectorAdapters.push_back(json_sub->valuestring);
                }
            }
        }

        json_item = cJSON_GetObjectItemCaseSensitive(json_conveer, "detector_name");
        if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
            cfg.conveer.detectorName = json_item->valuestring;
        }

        json_item = cJSON_GetObjectItemCaseSensitive(json_conveer, "detector_adapters");
        if (cJSON_IsArray(json_item)) {
            for(int i = 0; i < cJSON_GetArraySize(json_item); i++) {
                json_arr = cJSON_GetArrayItem(json_item, i);
                json_sub = cJSON_GetObjectItemCaseSensitive(json_arr, "name");
                if (cJSON_IsString(json_sub) && (json_sub->valuestring != NULL)) {
                    cfg.conveer.detectorAdapters.push_back(json_sub->valuestring);
                }
            }
        }

        json_item = cJSON_GetObjectItemCaseSensitive(json_conveer, "recognizer_name");
        if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
            cfg.conveer.recognizerName = json_item->valuestring;
        }

        json_item = cJSON_GetObjectItemCaseSensitive(json_conveer, "recognizer_adapters");
        if (cJSON_IsArray(json_item)) {
            for(int i = 0; i < cJSON_GetArraySize(json_item); i++) {
                json_arr = cJSON_GetArrayItem(json_item, i);
                json_sub = cJSON_GetObjectItemCaseSensitive(json_arr, "name");
                if (cJSON_IsString(json_sub) && (json_sub->valuestring != NULL)) {
                    cfg.conveer.recognizerAdapters.push_back(json_sub->valuestring);
                }
            }
        }
    }
    //LP Recognazer

    json_item = cJSON_GetObjectItemCaseSensitive(json, "camera_url");
    if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
        cfg.camera_url = json_item->valuestring;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json, "camera_id");
    if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
        cfg.camera_id = json_item->valuestring;
    }

    json_item = cJSON_GetObjectItemCaseSensitive(json, "camera_type");
    if (cJSON_IsString(json_item) && (json_item->valuestring != NULL)) {
        cfg.camera_type = json_item->valuestring;
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

