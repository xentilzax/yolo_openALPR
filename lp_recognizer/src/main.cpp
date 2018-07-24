#include <iostream>
#include <iomanip> 
#include <string>
#include <vector>
#include <fstream>

#include <opencv2/opencv.hpp>

#include "yolo_v2_class.hpp"
#include "alpr.h"


//Example run: ./demo cfg/yolov2-tiny-obj.cfg yolo-voc.weights cfg/open_alpr.conf images/1.jpg
#include <iostream>
#include <fstream>

int main(int argc, char *argv[])
{

    std::string cfg_file = "cfg/ishta_sp5.cfg";
    std::string weights_file = "weights/ishta_sp5.weights";
    std::string alpr_cfg_file = "cfg/open_alpr.conf";
    std::string filename = "";

    if (argc <= 1) {
        std::cout << "Need setting parameters for run application:" << std::endl;
        std::cout << "test_app <NN config file> <NN weights file> <open_alpr.conf file> <image file>" << std::endl;
        exit(1);
    } else
        if (argc > 4) {	//voc.names yolo-voc.cfg yolo-voc.weights test.mp4
            cfg_file = argv[1];
            weights_file = argv[2];
            alpr_cfg_file = argv[3];
            filename = argv[4];
        }
        else
            if (argc > 1)
                filename = argv[1];

    float const thresh = (argc > 5) ? std::stof(argv[5]) : 0.20;

    cv::VideoCapture capture;

    if(filename.empty()) {
	capture.open(0);
    } else {
	capture.open(filename);
    }
    if ( !capture.isOpened() ) {
        std::cerr << "Error: Can't open video file:" << filename << std::endl;
        return -1;
    }

    size_t width = capture.get(CV_CAP_PROP_FRAME_WIDTH);
    size_t height = capture.get(CV_CAP_PROP_FRAME_HEIGHT);
    float fps = capture.get(CV_CAP_PROP_FPS);
    //float countFramesRead = capture.get(CV_CAP_PROP_FRAME_COUNT);
    std::cout << "size frames: " << width << "x" <<height << std::endl;


    alpr::Alpr openalpr("us", alpr_cfg_file);
    // Optionally, you can specify the top N possible plates to return (with confidences). The default is ten.
    openalpr.setTopN(5);
    // Optionally, you can provide the library with a region for pattern matching. This improves accuracy by
    // comparing the plate text with the regional pattern.
    openalpr.setDefaultRegion("md");

    // Make sure the library loads before continuing.
    // For example, it could fail if the config/runtime_data is not found.
    if (openalpr.isLoaded() == false) {
        std::cerr << "Error loading OpenALPR" << std::endl;
        return 1;
    }

    Detector detector(cfg_file, weights_file);

    cv::namedWindow("video stream",cv::WINDOW_NORMAL);
    int count_images = 0;
    int count_found_LP =0;
    int count_recognize_LP =0;
    clock_t begin_time = clock();
    cv::Mat img;
    cv::Mat img_roi;

    while ( fps ) {
        //read the current frame

        if(!capture.read(img)) {
            break;
        }
        
        count_images++;
        
        img.copyTo(img_roi);


        std::vector<bbox_t> result_vec;
        std::vector<alpr::AlprRegionOfInterest> roi_list;

        result_vec = detector.detect(img, thresh);

        for(size_t i = 0; i < result_vec.size(); i++) {
            count_found_LP++;

            bbox_t b = result_vec[i];
            img(cv::Rect(b.x, b.y, b.w, b.h)).copyTo(img_roi);

            alpr::AlprResults results;

            // Recognize an image file. Alternatively, you could provide the image bytes in-memory.
            results = openalpr.recognize((unsigned char*)(img_roi.data), img_roi.channels(), img_roi.cols, img_roi.rows, roi_list);

            // Carefully observe the results. There may be multiple plates in an image,
            // and each plate returns the top N candidates.
            for (int i = 0; i < results.plates.size(); i++)
            {
                alpr::AlprPlateResult plate = results.plates[i];
                if( plate.topNPlates.size() > 0) {
                    count_recognize_LP++;
                    alpr::AlprPlate candidate = plate.topNPlates[0];
                    std::cout << "\nplate " << i << " : " << candidate.characters << std::endl;
                } else {
                    std::cout << "Not found licinse plates" << std::endl;
                }
            }
        }//for

        for(size_t i = 0; i < result_vec.size(); i++) {
            bbox_t b = result_vec[i];
            cv::rectangle(img, cv::Rect(b.x, b.y, b.w, b.h), cv::Scalar(255, 255, 0), 2);
        }

        cv::imshow("video stream", img);
        if( cv::waitKey(1) == 27)
            break;

        std::cout  << "Frames: " << count_images
                   << "\tCount_found_LP: " << count_found_LP
                   << "\tAvr.time: " << float(clock() - begin_time) / CLOCKS_PER_SEC / count_images <<"\r";
        std::cout << std::flush;
    }//while
    std::cout << "\n==========================\n";

    std::cout << "avr. time : " << float(clock() - begin_time) / CLOCKS_PER_SEC / count_images << std::endl;
    std::cout << "total images: "<< count_images << std::endl;
    std::cout << "total detect lp: "<< count_found_LP << std::endl;
    std::cout << "total recognize lp: "<< count_recognize_LP << std::endl;

    //capture.release();
}
