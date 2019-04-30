#include "yolo_v2_class.hpp"


#include "network.h"

extern "C" {
#include "detection_layer.h"
#include "region_layer.h"
#include "cost_layer.h"
#include "utils.h"
#include "parser.h"
#include "box.h"
#include "image.h"
#include "option_list.h"
#include "stb_image.h"
}

#include <vector>
#include <iostream>
#include <algorithm>

#define FRAMES 3

static std::unique_ptr<Detector> detector;

int init(const char *configurationFilename, const char *weightsFilename, int gpu) 
{
    detector.reset(new Detector(configurationFilename, weightsFilename, gpu));
    return 1;
}

int detect_image(const char *filename, bbox_t_container &container) 
{
    std::vector<bbox_t> detection = detector->detect(filename);
    for (size_t i = 0; i < detection.size() && i < C_SHARP_MAX_OBJECTS; ++i)
        container.candidates[i] = detection[i];
    return detection.size();
}

int detect_mat(const uint8_t* data, const size_t data_length, bbox_t_container &container) {
#ifdef OPENCV
    std::vector<char> vdata(data, data + data_length);
    cv::Mat image = imdecode(cv::Mat(vdata), 1);

    std::vector<bbox_t> detection = detector->detect(image);
    for (size_t i = 0; i < detection.size() && i < C_SHARP_MAX_OBJECTS; ++i)
        container.candidates[i] = detection[i];
    return detection.size();
#else
    return -1;
#endif    // OPENCV
}

int dispose() {
    //if (detector != NULL) delete detector;
    //detector = NULL;
    detector.reset();
    return 1;
}

int get_device_count() {
#ifdef GPU
    int count = 0;
    cudaGetDeviceCount(&count);
    return count;
#else
    return -1;
#endif	// GPU
}

int get_device_name(int gpu, char* deviceName) {
#ifdef GPU
    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, gpu);
    std::string result = prop.name;
    std::copy(result.begin(), result.end(), deviceName);
    return 1;
#else
    return -1;
#endif	// GPU
}

#ifdef GPU
void check_cuda(cudaError_t status) {
    if (status != cudaSuccess) {
        const char *s = cudaGetErrorString(status);
        print_to_stdout("CUDA Error Prev: %s\n", s);
    }
}
#endif

struct detector_gpu_t {
    network net;
    image images[FRAMES];
    float *avg;
    float *predictions[FRAMES];
    int demo_index;
    unsigned int *track_id;
};

PointerToPrintf get_pointer_printf()
{
    return print_to_stdout;
}

PointerToFprintf get_pointer_fprintf()
{
    return print_to_stderr;
}

void set_pointer_printf(PointerToPrintf ptr)
{
    print_to_stdout = ptr;
}

void set_pointer_fprintf(PointerToFprintf ptr)
{
    print_to_stderr = ptr;
}

YOLODLL_API Detector::Detector(std::string cfg_filename, std::string weight_filename, int gpu_id, int batchSize)
    : cur_gpu_id(gpu_id)
{
    wait_stream = 0;
    int old_gpu_index;
#ifdef GPU
    check_cuda( cudaGetDevice(&old_gpu_index) );
#endif

    detector_gpu_ptr = std::make_shared<detector_gpu_t>();
    detector_gpu_t &detector_gpu = *static_cast<detector_gpu_t *>(detector_gpu_ptr.get());

#ifdef GPU
    //check_cuda( cudaSetDevice(cur_gpu_id) );
    cuda_set_device(cur_gpu_id);
    print_to_stdout(" Used GPU %d \n", cur_gpu_id);
#endif
    network &net = detector_gpu.net;
    net.gpu_index = cur_gpu_id;
    //gpu_index = i;
    
    char *cfgfile = const_cast<char *>(cfg_filename.data());
    char *weightfile = const_cast<char *>(weight_filename.data());

    net = parse_network_cfg_custom(cfgfile, batchSize);
    if (weightfile) {
        load_weights(&net, weightfile);
    }
    set_batch_network(&net, batchSize);
    net.gpu_index = cur_gpu_id;
    fuse_conv_batchnorm(net);

    imageDataSize = net.w * net.h * net.c;
    imageBlob = std::unique_ptr<float[]>(new float[imageDataSize*batchSize]);

    layer l = net.layers[net.n - 1];
    int j;

    detector_gpu.avg = (float *)calloc(l.outputs, sizeof(float));
    for (j = 0; j < FRAMES; ++j) detector_gpu.predictions[j] = (float *)calloc(l.outputs, sizeof(float));
    for (j = 0; j < FRAMES; ++j) detector_gpu.images[j] = make_image(1, 1, 3);

    detector_gpu.track_id = (unsigned int *)calloc(l.classes, sizeof(unsigned int));
    for (j = 0; j < l.classes; ++j) detector_gpu.track_id[j] = 1;

#ifdef GPU
    check_cuda( cudaSetDevice(old_gpu_index) );
#endif
}


YOLODLL_API Detector::~Detector() 
{
    detector_gpu_t &detector_gpu = *static_cast<detector_gpu_t *>(detector_gpu_ptr.get());
    layer l = detector_gpu.net.layers[detector_gpu.net.n - 1];

    free(detector_gpu.track_id);

    free(detector_gpu.avg);
    for (int j = 0; j < FRAMES; ++j) free(detector_gpu.predictions[j]);
    for (int j = 0; j < FRAMES; ++j) if(detector_gpu.images[j].data) free(detector_gpu.images[j].data);

    int old_gpu_index;
#ifdef GPU
    cudaGetDevice(&old_gpu_index);
    cuda_set_device(detector_gpu.net.gpu_index);
#endif

    free_network(detector_gpu.net);

#ifdef GPU
    cudaSetDevice(old_gpu_index);
#endif
}

YOLODLL_API int Detector::get_net_width() const {
    detector_gpu_t &detector_gpu = *static_cast<detector_gpu_t *>(detector_gpu_ptr.get());
    return detector_gpu.net.w;
}
YOLODLL_API int Detector::get_net_height() const {
    detector_gpu_t &detector_gpu = *static_cast<detector_gpu_t *>(detector_gpu_ptr.get());
    return detector_gpu.net.h;
}
YOLODLL_API int Detector::get_net_color_depth() const {
    detector_gpu_t &detector_gpu = *static_cast<detector_gpu_t *>(detector_gpu_ptr.get());
    return detector_gpu.net.c;
}


YOLODLL_API std::vector<bbox_t> Detector::detect(std::string image_filename, float thresh, bool use_mean)
{
    std::shared_ptr<image_t> image_ptr(new image_t, [](image_t *img) { if (img->data) free(img->data); delete img; });
    *image_ptr = load_image(image_filename);
    std::vector<std::shared_ptr<image_t>> imageBatch;
    imageBatch.push_back(image_ptr);
    return detect(imageBatch, thresh, use_mean)[0];
}

static image load_image_stb(char *filename, int channels)
{
    int w, h, c;
    unsigned char *data = stbi_load(filename, &w, &h, &c, channels);
    if (!data)
        throw std::runtime_error("file not found");
    if (channels) c = channels;
    int i, j, k;
    image im = make_image(w, h, c);
    for (k = 0; k < c; ++k) {
        for (j = 0; j < h; ++j) {
            for (i = 0; i < w; ++i) {
                int dst_index = i + w*j + w*h*k;
                int src_index = k + c*i + c*w*j;
                im.data[dst_index] = (float)data[src_index] / 255.;
            }
        }
    }
    free(data);
    return im;
}

YOLODLL_API image_t Detector::load_image(std::string image_filename)
{
    char *input = const_cast<char *>(image_filename.data());
    image im = load_image_stb(input, 3);

    image_t img;
    img.c = im.c;
    img.data = im.data;
    img.h = im.h;
    img.w = im.w;

    return img;
}


YOLODLL_API void Detector::free_image(image_t m)
{
    if (m.data) {
        free(m.data);
    }
}

#ifdef OPENCV
std::vector<std::vector<bbox_t>> Detector::detect(const std::vector<cv::Mat> & batchImages, float thresh, bool use_mean)
{
    if(batchImages.size() == 0)
        throw std::runtime_error("Image batch is empty");

    std::vector<std::shared_ptr<image_t>> imageBatch;

    for(int i = 0; i < batchImages.size(); i++) {
        if(batchImages[i].data == NULL)
            throw std::runtime_error("Image is empty");

        auto image_ptr = mat_to_image_resize(batchImages[i]);
        imageBatch.push_back(image_ptr);
    }

    std::vector<std::vector<bbox_t>> batchBboxes = detect_resized(imageBatch, thresh, use_mean);
    return batchBboxes;

}
#endif

YOLODLL_API std::vector<std::vector<bbox_t>> Detector::detect(const std::vector<std::shared_ptr<image_t>>& imgBatch, float thresh, bool use_mean)
{
    detector_gpu_t &detector_gpu = *static_cast<detector_gpu_t *>(detector_gpu_ptr.get());
    network &net = detector_gpu.net;
    int old_gpu_index;
#ifdef GPU
    cudaGetDevice(&old_gpu_index);
    if(cur_gpu_id != old_gpu_index)
        cudaSetDevice(net.gpu_index);

    net.wait_stream = wait_stream;    // 1 - wait CUDA-stream, 0 - not to wait
#endif
    //std::cout << "net.gpu_index = " << net.gpu_index << std::endl;

    //float nms = .4;

    if(imgBatch.size() <= 0)
        throw std::runtime_error("Image batch is empty");

    //copy images to blob
    for(int i = 0; i < imgBatch.size(); i++) {
        if (net.w == imgBatch[i]->w && net.h == imgBatch[i]->h) {
            memcpy(imageBlob.get() + imageDataSize*i, imgBatch[i]->data, imageDataSize * sizeof(float));
        } else {
            std::shared_ptr<image_t> imgPtr = imgBatch[i];
            const image_t imgt = *imgPtr.get();
            image img;
            img.c = imgt.c;
            img.h = imgt.h;
            img.w = imgt.w;
            img.data = imgt.data;

            image sized = resize_image(img, net.w, net.h);
            memcpy(imageBlob.get() + imageDataSize*i, sized.data, imageDataSize * sizeof(float));
            if(sized.data)
                free(sized.data);
        }
    }

    layer l = net.layers[net.n - 1];
    float *prediction = network_predict(net, imageBlob.get());

    if (use_mean) {
        memcpy(detector_gpu.predictions[detector_gpu.demo_index], prediction, l.outputs * sizeof(float));
        mean_arrays(detector_gpu.predictions, FRAMES, l.outputs, detector_gpu.avg);
        l.output = detector_gpu.avg;
        detector_gpu.demo_index = (detector_gpu.demo_index + 1) % FRAMES;
    }
    //get_region_boxes(l, 1, 1, thresh, detector_gpu.probs, detector_gpu.boxes, 0, 0);
    //if (nms) do_nms_sort(detector_gpu.boxes, detector_gpu.probs, l.w*l.h*l.n, l.classes, nms);

    int nboxes = 0;
    int letterbox = 0;
    float hier_thresh = 0.5;
    detection *dets = get_network_boxes(&net, 1, 1, thresh, hier_thresh, 0, 1, &nboxes, letterbox);

    size_t stepDets = nboxes / l.batch;
    std::vector<std::vector<bbox_t>> bboxByImages;

    for(int k = 0; k < l.batch; k++) {
        detection* ptrDets = dets + k*stepDets;
        if (nms)
            do_nms_sort(ptrDets, stepDets, l.classes, nms);

        std::vector<bbox_t> bbox_vec;

        for (size_t i = 0; i < stepDets; ++i) {
            box b = ptrDets[i].bbox;
            int const obj_id = max_index(ptrDets[i].prob, l.classes);
            float const prob = ptrDets[i].prob[obj_id];

            if (prob > thresh)
            {
                bbox_t bbox;
                bbox.x = std::max((double)0, (b.x - b.w / 2.)*imgBatch[k]->w);
                bbox.y = std::max((double)0, (b.y - b.h / 2.)*imgBatch[k]->h);
                bbox.w = b.w*imgBatch[k]->w;
                bbox.h = b.h*imgBatch[k]->h;
                bbox.obj_id = obj_id;
                bbox.prob = prob;
                bbox.track_id = 0;

                bbox_vec.push_back(bbox);
            }
        }
        bboxByImages.push_back(bbox_vec);
    }
    free_detections(dets, nboxes);

#ifdef GPU
    if (cur_gpu_id != old_gpu_index)
        cudaSetDevice(old_gpu_index);
#endif

    return bboxByImages;
}

YOLODLL_API std::vector<bbox_t> Detector::tracking_id(std::vector<bbox_t> cur_bbox_vec, bool const change_history, 
                                                      int const frames_story, int const max_dist)
{
    detector_gpu_t &det_gpu = *static_cast<detector_gpu_t *>(detector_gpu_ptr.get());

    bool prev_track_id_present = false;
    for (auto &i : prev_bbox_vec_deque)
        if (i.size() > 0) prev_track_id_present = true;

    if (!prev_track_id_present) {
        for (size_t i = 0; i < cur_bbox_vec.size(); ++i)
            cur_bbox_vec[i].track_id = det_gpu.track_id[cur_bbox_vec[i].obj_id]++;
        prev_bbox_vec_deque.push_front(cur_bbox_vec);
        if (prev_bbox_vec_deque.size() > frames_story) prev_bbox_vec_deque.pop_back();
        return cur_bbox_vec;
    }

    std::vector<unsigned int> dist_vec(cur_bbox_vec.size(), std::numeric_limits<unsigned int>::max());

    for (auto &prev_bbox_vec : prev_bbox_vec_deque) {
        for (auto &i : prev_bbox_vec) {
            int cur_index = -1;
            for (size_t m = 0; m < cur_bbox_vec.size(); ++m) {
                bbox_t const& k = cur_bbox_vec[m];
                if (i.obj_id == k.obj_id) {
                    float center_x_diff = (float)(i.x + i.w/2) - (float)(k.x + k.w/2);
                    float center_y_diff = (float)(i.y + i.h/2) - (float)(k.y + k.h/2);
                    unsigned int cur_dist = sqrt(center_x_diff*center_x_diff + center_y_diff*center_y_diff);
                    if (cur_dist < max_dist && (k.track_id == 0 || dist_vec[m] > cur_dist)) {
                        dist_vec[m] = cur_dist;
                        cur_index = m;
                    }
                }
            }

            bool track_id_absent = !std::any_of(cur_bbox_vec.begin(), cur_bbox_vec.end(),
                                                [&i](bbox_t const& b) { return b.track_id == i.track_id && b.obj_id == i.obj_id; });

            if (cur_index >= 0 && track_id_absent){
                cur_bbox_vec[cur_index].track_id = i.track_id;
                cur_bbox_vec[cur_index].w = (cur_bbox_vec[cur_index].w + i.w) / 2;
                cur_bbox_vec[cur_index].h = (cur_bbox_vec[cur_index].h + i.h) / 2;
            }
        }
    }

    for (size_t i = 0; i < cur_bbox_vec.size(); ++i)
        if (cur_bbox_vec[i].track_id == 0)
            cur_bbox_vec[i].track_id = det_gpu.track_id[cur_bbox_vec[i].obj_id]++;

    if (change_history) {
        prev_bbox_vec_deque.push_front(cur_bbox_vec);
        if (prev_bbox_vec_deque.size() > frames_story) prev_bbox_vec_deque.pop_back();
    }

    return cur_bbox_vec;
}
