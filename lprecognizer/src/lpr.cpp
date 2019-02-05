#include <ctime>
#include <fstream>
#include <iostream>
#include <iomanip> 
#include <streambuf>
#include <string>
#include <sstream>
#include <vector>
#include <chrono>

#include <opencv2/opencv.hpp>

#include "yolo_v2_class.hpp"

#include "config.hpp"
//#include "post.hpp"
#include "motion_detector.hpp"
#include "object_detector.hpp"
#include "object_recognizer.hpp"
//---------------------------------------------------------------------------------------------------------------

#define AVR_W 0.1
#define VERSION 1.0
#define SITE_ID "unspecified"


//---------------------------------------------------------------------------------------------------------------
using namespace std::chrono;

namespace IZ
{
typedef alpr::AlprResults Results;
typedef alpr::AlprRegionOfInterest RegionOfInterest;
}

//---------------------------------------------------------------------------------------------------------------
class ConveerImpl
{
public:
    std::shared_ptr<IZ::MotionDetector> motionDetector;
    std::vector<std::shared_ptr<IZ::SaveAdapter> > motionDetectorAdapters;
    std::shared_ptr<IZ::ObjectDetector> detector;
    std::vector<std::shared_ptr<IZ::SaveAdapter> > detectorAdapters;
    std::shared_ptr<IZ::LicensePlateRecognizer> recognizer;
    std::vector<std::shared_ptr<IZ::SaveAdapter> > recognizerAdapters;
};
//---------------------------------------------------------------------------------------------------------------

IZ::Config cfg;

//---------------------------------------------------------------------------------------------------------------
void DefineAdapters(std::vector<std::string> & adapterName,
                    std::vector<std::shared_ptr<IZ::SaveAdapter> > & adapters,
                    const IZ::Config & cfg)
{
    for(size_t i = 0; i < adapterName.size(); i++) {
        if( adapterName[i] == "disk_adapter" ) {
            adapters.push_back(std::shared_ptr<IZ::SaveAdapter>(new IZ::DiskAdapter(cfg)));
        }

        if( adapterName[i] == "socket_adapter" ) {
            adapters.push_back(std::shared_ptr<IZ::SaveAdapter>(new IZ::SocketAdapter(cfg)));
        }
    }
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
    if( ParseConfig(str, cfg) == 0) {
        return 0x0B;
    }

//    if(curl_global_init(CURL_GLOBAL_ALL)) {
//        fprintf(stderr, "Fatal: The initialization of libcurl has failed.\n");
//        return EXIT_FAILURE;
//    }

//    if(atexit(curl_global_cleanup)) {
//        fprintf(stderr, "Fatal: atexit failed to register curl_global_cleanup.\n");
//        curl_global_cleanup();
//        return EXIT_FAILURE;
//    }


    ConveerImpl conveer;
    //define motion detector
    if ( cfg.conveer.motionDetectorName ==  "simple")
        conveer.motionDetector = std::shared_ptr<IZ::MotionDetector>(new IZ::SimpleMotion(cfg));
    if ( cfg.conveer.motionDetectorName ==  "background_substruction")
        conveer.motionDetector = std::shared_ptr<IZ::MotionDetector>(new IZ::MoveDetector(cfg));

    //define motion detector adapters
    DefineAdapters(cfg.conveer.motionDetectorAdapters, conveer.motionDetectorAdapters, cfg);

    //define detector
    if ( cfg.conveer.detectorName ==  "yolo")
        conveer.detector = std::shared_ptr<IZ::ObjectDetector>(new IZ::YOLO_Detector(cfg));
    if ( cfg.conveer.detectorName ==  "open_alpr")
        conveer.detector = std::shared_ptr<IZ::ObjectDetector>(new IZ::ALPR_Module(cfg));

    //define detector adapters
    DefineAdapters(cfg.conveer.detectorAdapters, conveer.detectorAdapters, cfg);

    //define recognizer
    if ( cfg.conveer.recognizerName ==  "open_alpr")
        conveer.recognizer = std::shared_ptr<IZ::LicensePlateRecognizer>(new IZ::ALPR_Module(cfg));

    //define detector adapters
    DefineAdapters(cfg.conveer.recognizerAdapters, conveer.recognizerAdapters, cfg);

    int count_images = 0;
    int count_fail_frame = 0;
    cv::Mat img;
    cv::VideoCapture capture;

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

            count_images++;

            std::vector<IZ::EventMotionDetection> eventsMotion;
            std::vector<IZ::EventObjectDetection> eventsDetection;
            std::vector<IZ::EventObjectRecognize> eventsRecognize;

            milliseconds ms = duration_cast<milliseconds>( system_clock::now().time_since_epoch() );

            auto beginTime = system_clock::now();
            time_point<system_clock> endTime, motionDetectorTime, detectorTime, recognitionTime;

            conveer.motionDetector->Detection(img,
                                              ms.count(),
                                              eventsMotion);
            motionDetectorTime = system_clock::now();

            for(auto adapter :conveer.motionDetectorAdapters) {
                adapter->SaveEvent(eventsMotion);
            }

            auto startDetectorTime = system_clock::now();

            conveer.detector->Detection(eventsMotion, eventsDetection);

            detectorTime = system_clock::now();

            for(auto adapter :conveer.motionDetectorAdapters) {
                adapter->SaveEvent(eventsDetection);
            }

            auto startRecognizeTime = system_clock::now();
            conveer.recognizer->Recognize(eventsDetection, eventsRecognize);
            recognitionTime = system_clock::now();

            for(auto adapter :conveer.motionDetectorAdapters) {
                adapter->SaveEvent(eventsRecognize);
            }

            if ( cfg.gui_enable ) {
                for(size_t i = 0; i < eventsDetection.size(); i++) {
                    for( auto & obj : eventsDetection[i].detectedObjects) {
                        cv::imshow(std::to_string(i), obj.croppedFrame);
                    }
                }
            }

            endTime = system_clock::now();

            if ( cfg.gui_enable ) {
                cv::imshow("video stream", img);
                if (cv::waitKey(1) >= 0)
                    return 0;
            }

            if ( cfg.verbose_level >= 1 ) {
                std::cout  << "Frames: " << count_images
                           << " motion time: " << duration_cast<milliseconds>(motionDetectorTime - beginTime).count()
                           << " detetion time: " << duration_cast<milliseconds>(detectorTime -  startDetectorTime).count()
                           << " recognition time: " << duration_cast<milliseconds>(recognitionTime - startRecognizeTime).count()
                           << " full time: " << duration_cast<milliseconds>(endTime - beginTime).count()
                           << std::endl;
            }

        }//while
        capture.release();
    }
}

