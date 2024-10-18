/*
 * Original Code (C) Copyright Edgecortix, Inc. 2022
 * Modified Code (C) Copyright Renesas Electronics Corporation 2023
 *　
 *  *1 DRP-AI TVM is powered by EdgeCortix MERA(TM) Compiler Framework.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 */
/***********************************************************************************************************************
 * DISCLAIMER
 * This software is supplied by Renesas Electronics Corporation and is only intended for use with Renesas products. No
 * other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
 * applicable laws, including copyright laws.
 * THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING
 * THIS SOFTWARE, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED. TO THE MAXIMUM
 * EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS AFFILIATED COMPANIES
 * SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON RELATED TO THIS
 * SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 * Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
 * this software. By using this software, you agree to the additional terms and conditions found by accessing the
 * following link:
 * http://www.renesas.com/disclaimer
 *
 * Copyright (C) 2023 Renesas Electronics Corporation. All rights reserved.
 ***********************************************************************************************************************/
/***********************************************************************************************************************
 * File Name    : recognize_base.cpp
 * Version      : 1.0
 * Description  : RZ/V2L DRP-AI TVM[*1] Sample Application for USB Camera HTTP version
 *                *1 DRP-AI TVM is powered by EdgeCortix MERA(TM) Compiler Framework.
 ***********************************************************************************************************************/
/*****************************************
 * Includes
 ******************************************/
#include "recognize_base.h"
#include "../command/camera_image.h"
#include "../include/lwsock.hpp"
#include "../util/recognize_define.h"
#include <builtin_fp16.h>
#include <fstream>
#include <sys/time.h>
#include "../util/functions.h"
#include <thread>
#include "../util/PreRuntime.h"
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::system_clock;


/**
 * @brief RecognizeBase
 * @details  Construct a new Recognize Base:: Recognize Base object
 */
RecognizeBase::RecognizeBase()
{
    _pthread_ai_inf = 0;
    _pthread_capture = 0;
    _pthread_framerate = 0;
}

/*****************************************
* @brief get_drpai_start_addr
* @details Function to get the start address of DRP-AI Memory Area.
* @return uint32_t = DRP-AI Memory Area start address in 32-bit.
******************************************/
uint32_t RecognizeBase::get_drpai_start_addr()
{
    int fd = 0;
    int ret = 0;
    drpai_data_t drpai_data;
    errno = 0;
    fd = open("/dev/drpai0", O_RDWR);
    if (0 > fd )
    {
        fprintf(stderr, "[ERROR] Failed to open DRP-AI Driver : errno=%d\n", errno);
        return (uint32_t)NULL;
    }
    /* Get DRP-AI Memory Area Address via DRP-AI Driver */
    ret = ioctl(fd , DRPAI_GET_DRPAI_AREA, &drpai_data);
    if (-1 == ret)
    {
        fprintf(stderr, "[ERROR] Failed to get DRP-AI Memory Area : errno=%d\n", errno);
        return (uint32_t)NULL;
    }
    return drpai_data.address;
}

/**
 * @brief print_measure_log
 * @details Print measurement log to console
 * @param item measurement item
 * @param time elapsed time
 * @param unit unit
 */
void print_measure_log(string item, float time, string unit)
{
    printf("[MeasLog],%s, %.1f, [%s]\n", item.c_str(), time, unit.c_str());
}

/**
 * @brief print_measure_log
 * @details Print measurement log to console
 * @param item measurement item
 * @param log log
 */
void print_measure_log(string item, string log)
{
    printf("[MeasLog],%s,%s\n", item.c_str(), log.c_str());
}

/**
 * @brief float16_to_float32
 * @details Function by Edgecortex. Convert uin16_t number into float value.
 * @param a float16 number
 * @return float float32 number
 */
float float16_to_float32(uint16_t a)
{
    return __extendXfYf2__<uint16_t, uint16_t, 10, float, uint32_t, 23>(a);
}

/**
 * @brief initialize
 * @details Initialization for recognize process.
 * @param model Model to be run
 * @return int32_t success:0 error: != 0
 */
int32_t RecognizeBase::initialize(IRecognizeModel *model)
{
    std::cout << "############ INIT ############" << std::endl;
    _model = shared_ptr<IRecognizeModel>(move(model));
    std::cout << "[INFO] Model     :" << _model->model_name << std::endl;
    std::cout << "[INFO] Directory :" << _model->model_dir << std::endl;
    std::cout << "[INFO] outbuff   :" << _model->outBuffSize << std::endl;
    _outBuffSize = _model->outBuffSize;
    dir = _model->model_dir + "/";
    cap_w = _model->_capture_w;
    cap_h = _model->_capture_h;
    cap_c = _model->_capture_c;
    model_w = _model->_model_w;
    model_h = _model->_model_h;
    model_c = _model->_model_c;
    mode = _model->_id;
    return 0;
}

/**
 * @brief recognize_start
 * @details Start recognition
 * @return int32_t success:0 error: != 0
 */
int32_t RecognizeBase::recognize_start()
{
    printf("############ RECOGNIZE ############\n");
    printf("Input : USB Camera\n");
    /* Create Camera Instance */
    _capture = make_shared<Camera>();
    /* Init and Start Camera */
    _capture->set_w(_model->_capture_w);
    _capture->set_h(_model->_capture_h);
    _capture->set_c(_model->_capture_c);
    int8_t ret = _capture->start_camera();
    int32_t create_thread_ai;
    if (0 != ret)
    {
        if(ret == -1)
            fprintf(stderr, "[ERROR] Failed to initialize MIPI Camera.\n");
        else if(ret == -2)
            fprintf(stderr, "[ERROR] Failed to initialize USB Camera.\n");
        return -1;
    }
    wake_ = false;
    capture_enabled.store(true);
    /* capture thread */
    _capture_running = true;
    int32_t create_thread_cap = pthread_create(&_pthread_capture, NULL, capture_thread, this);
    if (0 != create_thread_cap)
    {
        fprintf(stderr, "[ERROR] Failed to create Capture Thread.\n");
        return -1;
    }
#ifdef INFERENE_ON
    /* inference thread */
    _inf_running = true;
    /*If DRP-AI TVM (both DRP-AI mode and CPU mode) is selected*/
    if (MODE_TVM_MIN <= _model->_id)
    {
        create_thread_ai = pthread_create(&_pthread_ai_inf, NULL, tvm_inference_thread, this);
    }
    if (0 != create_thread_ai)
    {
        fprintf(stderr, "[ERROR] Failed to create AI Inference Thread.\n");
        return -1;
    }
#endif // INFERENE_ON
    /* framerate thread */
    _fps_runnning = true;
    int32_t framerate_thread_cap = pthread_create(&_pthread_framerate, NULL, framerate_thread, this);
    if (0 != framerate_thread_cap)
    {
        fprintf(stderr, "[ERROR] Failed to create Framerate Thread.\n");
        return -1;
    }
    output_writer = cv::VideoWriter(g_pipeline, cv::CAP_GSTREAMER,
                                    cv::VideoWriter::fourcc('H', '2', '6', '4'), 1, 
                                    cv::Size(DISP_OUTPUT_WIDTH, DISP_OUTPUT_HEIGHT), true);
    /* creates quit key thread*/
    pthread_create(&_pthread_quit_key, NULL, &get_quit_key, this);
    /* detach quit key thread from all other thread*/
    pthread_detach(_pthread_quit_key);
    start_recognize();
    return 0;
}

/**
 * @brief recognize_end
 * @details Recognize end proc
 */
void RecognizeBase::recognize_end()
{
    end_all_threads();
    close_camera();
    std::cout << "\n[INFO] End Application.\n"
              << std::endl;
}

/**
 * @brief capture_thread
 * @details usb camera capture and store udma buffer address for DRP-AI
 * @param arg pointer to itself
 * @return void*
 */
void *RecognizeBase::capture_thread(void *arg)
{
    RecognizeBase *me = (RecognizeBase *)arg;
    shared_ptr<Camera> capture = me->_capture;
    /*First Loop Flag*/
    uint32_t capture_addr = 0;
    int8_t ret = 0;
    CameraImage notify;
    vector<uint8_t> output;
    printf("Capture Thread Starting\n");
    while (me->_capture_running)
    {
        errno = 0;
#ifdef SEQUENCTCIAL
        std::cout << "[cam_thread]waiting for capture enable..." << std::endl;
        while (!me->capture_enabled.load())
        {
            usleep(1);
        }
        {
            Measuretime m("capture time");
            capture_addr = capture->capture_image();
        }
        me->capture_address = capture_addr;
        {
            Measuretime m("copy time");
            me->input_data = capture->get_img();
            me->capture_enabled.store(false);
            /* sync with inference thread*/
            unique_lock<mutex> lock(me->mtx_);
            me->wake_ = true;
            me->cv_.notify_all();
            std::cout << "[cam_thread]signaled for capture enable !!" << std::endl;
        }
#else
        /* _capture USB camera image and stop updating the _capture buffer */
        {
            Measuretime mmm("capture_time");
            capture_addr = capture->capture_image();
        }
#ifdef INFERENE_ON
        if (me->capture_enabled.load())
        {
            Measuretime m("capture enable proc time");
            me->capture_enabled.store(false);
            capture->sync_inference_buf_capture();
            me->capture_address = capture_addr;
            me->input_data = capture->get_img();
            /* sync with inference thread*/
            unique_lock<mutex> lock(me->mtx_);
            me->wake_ = true;
            me->cv_.notify_all();
        }
#endif
#endif
        me->_camera_frame_count.store(me->_camera_frame_count.load() + 1);
        if (0 == capture_addr)
        {
            fprintf(stderr, "[ERROR] Failed to _capture image from camera.\n");
            break;
        }
        else
        {
#ifdef SEND_CAMERA_ON
            uint8_t *send_image = capture->get_img();
            {
                Measuretime m("Command create and send time[camera]");
                /* Send ImageNotify*/
            }
#endif
        }
        /* IMPORTANT: Place back the image buffer to the _capture queue */
#ifdef SEQUENCTCIAL
        while (!me->capture_enabled.load())
        {
            usleep(1);
        }
        ret = capture->capture_qbuf();
#endif
#ifdef INFERENE_ON
#ifndef SEQUENCTCIAL
        /* IMPORTANT: Place back the image buffer to the _capture queue */
        if (capture->get_buf_capture_index() != capture->get_inference_buf_capture_index())
        {
            {
                Measuretime m("Deque capture buf time");
                ret = capture->capture_qbuf();
            }
        }
#endif
#else
        {
            Measuretime m("Deque capture buf time");
            ret = capture->capture_qbuf();
        }
#endif
        if (0 != ret)
        {
            fprintf(stderr, "[ERROR] Failed to enqueue _capture buffer.\n");
            break;
        }
    } /*End of Loop*/
    /*To terminate the loop in AI Inference Thread.*/
    unique_lock<mutex> lock(me->mtx_);
    me->wake_ = true;
    me->cv_.notify_all();
    cout << "<<<<<<<<<<<<<<<<<<<<< Capture Thread Terminated >>>>>>>>>>>>>>>>>>" << endl;
    pthread_exit(NULL);
    me->_pthread_capture = 0;
    return NULL;
}

/**
 * @brief inference thread
 * @details inference and send results
 * @param arg pointer to itself
 * @return void*
 */
void *RecognizeBase::tvm_inference_thread(void *arg)
{
    /*create recognize object from this pointer*/
    RecognizeBase *me = (RecognizeBase *)arg;
    shared_ptr<Camera> capture = me->_capture;
    int8_t ret = 0;
    /* Time Measure variables */
    timespec start_time, end_time;
    float ai_time = 0;
    float get_time = 0;
    float preproc_time = 0;
    /* DRP-AI TVM[*1] Runtime object */
    MeraDrpRuntimeWrapper runtime;
    /*Pre-processing output buffer pointer (DRP-AI TVM[*1] input data)*/
    float *pre_output_ptr;
    uint32_t out_size;
    float *pre_output_ptr_2;
    uint32_t out_size_2;
    /*Inference Variables*/
    int32_t inf_cnt = -1;
    /*Counter for inference output buffer*/
    uint32_t size_count = 0;
    /*Inference output buffer*/
    shared_ptr<float> drpai_output_buf;
    recognizeData_t data;
    InOutDataType input_data_type;
    InOutDataType input_data_type_2;
    /* object names string stream*/
    stringstream detection_object_ss(me->_model->detection_object_string);

    /*each objects from config.ini file*/
    string item;
    std::vector<std::string> label_file_map = me->_model->label_file_map;
    /*detection_object_set, detection objects in set format*/
    std::string objects_available = "";
    std::string objects_not_available = "";

    printf("Inference Thread Starting\n");
    if (!me->model_exist(me->dir))
    {
        fprintf(stderr, "[ERROR] The model is not in the specified directory\n");
        return 0;
    }
    // /*DRP-AI TVM[*1]::Load model_dir structure and its weight to runtime object */
    // runtime.LoadModel(me->dir);
    uint64_t drpaimem_addr_start = 0;
    bool runtime_status = false;
    /*Load model_dir structure and its weight to runtime object */
    drpaimem_addr_start = me->get_drpai_start_addr();
    if ((uint64_t)NULL == drpaimem_addr_start) 
    {
        fprintf(stderr, "[ERROR] Failed to get DRP-AI memory area start address.\n");
        /*Error processing, i.e., return, goto, etc.*/
        return 0;
    }
    runtime_status = runtime.LoadModel(me->dir, drpaimem_addr_start+DRPAI_MEM_OFFSET);
    if (!runtime_status)
    {
        fprintf(stderr, "[ERROR] Failed to load model.\n");
        /*Error processing, i.e., return, goto, etc.*/
        return 0;
    }
    /* Categorizing specified class to detect into supported and unsupported category*/
    while (std::getline(detection_object_ss, item, ','))
    {
        if (count(label_file_map.begin(), label_file_map.end(), item) > 0)
        {

            me->detection_object_set.insert(item);
            objects_available += item + "\n";
        }
        else
        {
            objects_not_available += item + "\n";
        }
    }
    std::cout << "[INFO] *******************Detection Parameters*******************" << std::endl;
    /* Listing unsupported category(class) */
    if (!objects_not_available.empty())
    {
        std::cout << "[INFO] Selected objects in config.ini which is not found in the label list\n"
                  << objects_not_available << "\n";
    }
    /* Listing supported category(class) */
    if (!objects_available.empty())
    {
        std::cout << "[INFO] Selected objects to detect\n"
                  << objects_available << "\n";
    }
    /* exit the application */
    else
    {
        std::cerr << "[ERROR] No matching objects in label list from the config.ini file" << std::endl;
        /* terminates all thread and stops recognition*/
        me->recognize_end();
        /* end application */
        exit(0);
    }
    /*DRP-AI TVM[*1]::Get input data type*/
    input_data_type = runtime.GetInputDataType(0);
    /*Inference Loop Start*/
    while (me->_inf_running)
    {
        std::cout << "[inf_thread] waiting for inference start..." << std::endl;
        /*Checks if image frame from _capture Thread is ready.*/
        {
            Measuretime m("Inference start wait time");
            unique_lock<mutex> lock(me->mtx_);
            me->cv_.wait(lock, [me]
                         { return me->wake_; });
            me->wake_ = false;
        }
        /*Pre-process*/
        me->init_time = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        me->get_time(start_time);
        me->inference_preprocess(arg, me->mode, me->cap_w, me->cap_h, &pre_output_ptr, &out_size);
        me->get_time(end_time);
        preproc_time = (float)((me->timedifference_msec(start_time, end_time)));
        print_measure_log("AI preprocess Time", preproc_time, "ms");
        /*Input data type can be either FLOAT32 or FLOAT16, which depends on the model */
        /*For DeepPose, Input=FP32*/
        if (InOutDataType::FLOAT32 == input_data_type)
        {
            /*DRP-AI TVM[*1]::Set input data to DRP-AI TVM[*1]*/
            runtime.SetInput(0, pre_output_ptr);
        }
        else
        {
            std::cerr << "[ERROR] Input data type : not FP32." << std::endl;
            break;
        }
        /**DRP-AI TVM[*1]::Start Inference*/
        errno = 0;
        inf_cnt++;
        printf("Inference ----------- No. %d\n", (inf_cnt + 1));
        /*Gets inference starting time*/
        me->get_time(start_time);
        /*DRP-AI TVM[*1]::Run inference*/
        runtime.Run();
        /*Gets AI Inference End Time*/
        me->get_time(end_time);
        /*Inference End Time */
        ai_time = (float)((me->timedifference_msec(start_time, end_time)));
        print_measure_log("AI Inference Time", ai_time, "ms");
        /*Process to read the DRP-AI output data.*/
        /* DRP-AI TVM[*1]::Get the number of output of the target model. For DeepPose, 1 output. */
        auto output_num = runtime.GetNumOutput();
        drpai_output_buf.reset(new float[me->_outBuffSize], std::default_delete<float[]>());
        size_count = 0;
        /*GetOutput loop*/
        for (int i = 0; i < output_num; i++)
        {
            /* DRP-AI TVM[*1]::Get the output including meta-data, i.e. data-type */
            auto output_buffer = runtime.GetOutput(i);
            /*Output Data Size = std::get<2>(output_buffer). */
            int64_t output_size = std::get<2>(output_buffer);
            /*Output Data Type = std::get<0>(output_buffer)*/
            if (InOutDataType::FLOAT16 == std::get<0>(output_buffer))
            {
                /*Output Data = std::get<1>(output_buffer)*/
                uint16_t *data_ptr = reinterpret_cast<uint16_t *>(std::get<1>(output_buffer));
                for (int j = 0; j < output_size; j++)
                {
                    /*FP16 to FP32 conversion*/
                    drpai_output_buf.get()[j + size_count] = float16_to_float32(data_ptr[j]);
                }
            }
            else if (InOutDataType::FLOAT32 == std::get<0>(output_buffer))
            {
                /*Output Data = std::get<1>(output_buffer)*/
                float *data_ptr = reinterpret_cast<float *>(std::get<1>(output_buffer));
                for (int j = 0; j < output_size; j++)
                {
                    drpai_output_buf.get()[j + size_count] = data_ptr[j];
                }
            }
            else
            {
                std::cerr << "[ERROR] Output data type : not floating point." << std::endl;
                ret = -1;
                break;
            }
            size_count += output_size;
        }
        /*Error check in the GetOutput loop*/
        if (0 != ret)
        {
            break;
        }
        /*Fill AI Inference result structure*/
        data.predict_image = me->input_data;
        data.predict_result = move(drpai_output_buf);
        data.inf_time_ms = ai_time;
        data.preproc_time_ms = preproc_time;
        /*Post-process start (AI inference result postprocess + image compress + JSON data sending)*/
        me->inference_postprocess(arg, me->mode, data);
        {
            /*Image compress + JSON data sending*/
            me->send_result(arg, me->mode, data);
        }
        Measuretime m("Deque inference_capture_qbuf buf time");
        ret = capture->inference_capture_qbuf();
        if (0 != ret)
        {
            fprintf(stderr, "[ERROR] Failed to enqueue _capture buffer.\n");
            break;
        }
        me->_ai_frame_count.store(me->_ai_frame_count.load() + 1);
        me->capture_enabled.store(true); /* Flag for _capture Thread. */
    }
    /*End of Inference Loop*/
    /*To terminate the loop in _capture Thread.*/
    me->capture_enabled.store(true);
    cout << "<<<<<<<<<<<<<<<<<<<<< AI Inference Thread Terminated >>>>>>>>>>>>>>>>>>" << endl;
    pthread_exit(NULL);
    me->_pthread_ai_inf = 0;
    return NULL;
}

/**
 * @brief framerate_thread
 * @details Framerate and cpu usage thread
 * @param arg pointer to itself
 * @return void*
 */
void *RecognizeBase::framerate_thread(void *arg)
{
    RecognizeBase *me = (RecognizeBase *)arg;
    while (me->_fps_runnning)
    {
        sleep(1);
        /* AI FPS */
        int32_t count = me->_ai_frame_count.load();
        me->_ai_frame_count.store(0);
        print_measure_log("------------------------------>AI FPS", count, "fps");
        /*Camerar FPS*/
        int32_t cam_count = me->_camera_frame_count.load();
        me->_camera_frame_count.store(0);
        print_measure_log("------------------------------>Camera FPS", cam_count, "fps");
        /* CPU usage*/
        string cpuUsage = me->_analyzer.get_cpu_usage(2);
        print_measure_log("CPU Usage", cpuUsage);
        /* Memory usage*/
        float memoryUsage = me->_analyzer.get_memory_usage();
        print_measure_log("Memory Usage", memoryUsage, "%");
    }
    cout << "<<<<<<<<<<<<<<<<<<<<< FPS Thread Terminated >>>>>>>>>>>>>>>>>>" << endl;
    pthread_exit(NULL);
    me->_pthread_framerate = 0;
    return NULL;
}

/**
 * @brief inference_preprocess
 * @details Preprocess
 * @param arg pointer to itself
 * @param model_id ID for model process to be run
 * @param width new width of input data.
 * @param height new height of input data.
 * @param out_ptr pre-processing result data
 * @param out_size size of out_ptr
 */
void RecognizeBase::inference_preprocess(void *arg, uint8_t model_id, uint32_t width, uint32_t height, 
                                            float **out_ptr, uint32_t *out_size)
{
    timespec start_time;
    timespec end_time;
    RecognizeBase *me = (RecognizeBase *)arg;
    Measuretime m("Pre process time");
    cv::Mat yuyv_image(height, width, CV_8UC2, (void *)me->input_data);
    /*convert yuv format to bgr*/
    cv::cvtColor(yuyv_image, g_bgra_image, cv::COLOR_YUV2RGB_YUYV);
    _model->inf_pre_process(me->input_data, width, height, me->capture_address, out_ptr, out_size);
}

/**
 * @brief inference_postprocess
 * @details Postprocess
 * @param arg pointer to itself
 * @param model_id ID for model process to be run
 * @param data inference result data
 */
void RecognizeBase::inference_postprocess(void *arg, uint8_t model_id, recognizeData_t &data)
{
    timespec start_time;
    timespec end_time;
    RecognizeBase *me = (RecognizeBase *)arg;
    me->get_time(start_time);
    {
        Measuretime m("Post process time");
        postproc_data = _model->inf_post_process(data.predict_result.get());
    }
    me->get_time(end_time);
    data.postproc_time_ms += (float)((me->timedifference_msec(start_time, end_time)));
    data.predict_result.reset();
}

/**
 * @brief get_quit_key
 * @details Get application quit key from keyboard
 * @param arg pointer to itself
 */
void *RecognizeBase::get_quit_key(void *arg)
{
    /*create recognize object from the this pointer */
    RecognizeBase *recognize_object = static_cast<RecognizeBase *>(arg);
    /*continuous loop for checking exist trigger*/
    while (true)
    {
        /*get input from the user */
        int key = getchar();
        /*if [ENTER] key is pressed*/
        if (key == 10)
        {
            std::cout << "[INFO] Enter Key Pressed!\n";
            break;
        }
    }
    /* terminates all thread and stops recognition*/
    recognize_object->recognize_end();
    /* end application */
    exit(0);
}

/**
 * @brief create_output_frame
 * @details create the output frame with space for displaying inference details
 * @param frame_g input frame to be displayed in the background
 * @return cv::Mat background, final display frame to be written to gstreamer pipeline
 */
cv::Mat RecognizeBase::create_output_frame(cv::Mat frame_g)
{
    /* Create a black background image of size 1080x720 */
    cv::Mat background(DISP_OUTPUT_HEIGHT, DISP_OUTPUT_WIDTH, frame_g.type(), cv::Scalar(0, 0, 0));

    /* Resize the original image to fit within 960x720 */
    cv::Mat resizedImage;
    cv::resize(frame_g, resizedImage, cv::Size(IMAGE_OUTPUT_WIDTH, IMAGE_OUTPUT_HEIGHT));

    /* Copy the resized image to the left side of the background (0 to 960) */
    cv::Rect roi(cv::Rect(0, 0, resizedImage.cols, resizedImage.rows));
    resizedImage.copyTo(background(roi));

    return background;
}

/**
 * @brief fps_calculation
 * @details function to calculate the AI-inference time
 */
void RecognizeBase::fps_calculation(void)
{
    /*timer for fps calculation*/
    if (init)
    {
        init = false;
        end_time = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        time_one = end_time - init_time;
        init_time = end_time;
    }
    else
    {
        end_time = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        infer_time_ms = end_time - init_time;
        init_time = end_time;
    }
}
/**
 * @brief show_result
 * @details function to show the result on the display screen
 */
void RecognizeBase::show_result(void)
{
    /*position for object count details in the output frame(right side)*/
    int text_height             = 130;
    int text_width              = 970;
    int total_count             = 0;

    /*font size for text inside bounding box*/
    float font_size_bb = 0.5;
    /*font weight for text inside bounding box*/
    float font_weight_bb = 2;
    /* predication score */
    float pred_score = 0;
    string bbox_text;

    for (bbox_t dat : detection_bb_vector)
    {
        /* get prediction score */
        pred_score = ((int)dat.pred);
        pred_score = pred_score/100;
        /* convert float predict score to string */
        std::string pred_score_str = std::to_string(pred_score);
        /* remove trailing zeros in converted pred_score(string) */
        size_t d_pos = pred_score_str.find_last_not_of('0');
        if(d_pos != std::string::npos)
            pred_score_str.erase(d_pos + 1,std::string::npos);
        bbox_text = dat.name + " " + pred_score_str;
        
        cv::Size text_size = cv::getTextSize(bbox_text, cv::FONT_HERSHEY_SIMPLEX, font_size_bb, 2, 0);
        /*adjust the font size based on the detection text size*/
        if (text_size.width > dat.W)
        {
            font_weight_bb  = .75;
            font_size_bb    = 0.3;
        }
        cv::Rect rect(dat.X, dat.Y, dat.W, dat.H);
        cv::Rect rect_text_box(dat.X, dat.Y - 20, dat.W, 20);
        
        /*draw the rectangle for detected object*/
        cv::rectangle(g_bgra_image, rect, BOX_COLOUR, 1.5);
        /*draw text box for holding the class label*/
        cv::rectangle(g_bgra_image, rect_text_box, BOX_COLOUR, cv::FILLED);
    
        /*writing class label to the display frame */
        cv::putText(g_bgra_image, bbox_text, cv::Point(dat.X + 5, dat.Y - 8), 
                    cv::FONT_HERSHEY_SIMPLEX, font_size_bb, WHITE, font_weight_bb);
    }
    /*coordinates for inference speed*/
    cv::Point ai_inf_postion(970, 60);
    /*coordinated for fps*/
    cv::Point fps_postion(975, 90);
    /*generate the output display frame*/
    cv::Mat bgra_image_org = create_output_frame(g_bgra_image);
    /*display the count for each objects detected inside the current frame*/

    for (bbox_t dat : detection_bb_vector)
    {
        cv::putText(bgra_image_org, dat.name, cv::Point(text_width + 30, text_height), 
                    cv::FONT_HERSHEY_SIMPLEX, font_size, WHITE, font_weight);
        cv::putText(bgra_image_org, " : " + std::to_string(int(dat.pred)) + " %", cv::Point(text_width + 175, text_height), 
                    cv::FONT_HERSHEY_SIMPLEX, font_size, WHITE, font_weight);
        text_height += 30;
    }

    /*put inference time inside the display frame*/
    cv::putText(bgra_image_org, "AI Inference time : " + std::to_string(infer_time_ms) + " [ms]", 
                ai_inf_postion, cv::FONT_HERSHEY_SIMPLEX, font_size, WHITE, font_weight);

    /*put FPS inside display frame*/
    cv::putText(bgra_image_org, "       Total FPS : " + std::to_string(1000 / infer_time_ms), 
                fps_postion, cv::FONT_HERSHEY_SIMPLEX, font_size, WHITE, font_weight);

    /*write the final frame to the gstreamer pipeline*/
    output_writer.write(bgra_image_org);
    /*clear the Bounding box vector array*/
    detection_bb_vector.clear();
}

/**
 * @brief object_detector
 * @details function to detect the objects based on the input confidence score
 */
void RecognizeBase::object_detector(void)
{

    /*filter detection based on confidence score and objects selected*/
    for (detection det : postproc_data)
    {
        bbox_t dat;

        /*ignore detection based on the threshold from the config.ini file*/
        if (det.prob < _model->conf)
        {
            continue;
        }
        /*get the label from label file map*/
        dat.name = _model->label_file_map[det.c].c_str();

        /*check if the detected object is in the list of objects to be detected(from the config.ini file)*/
        if (count(detection_object_set.begin(), detection_object_set.end(), dat.name) <= 0)
            continue;        
        dat.X = (int32_t)(det.bbox.x - (det.bbox.w / 2));
        dat.Y = (int32_t)(det.bbox.y - (det.bbox.h / 2));
        dat.W = (int32_t)det.bbox.w;
        dat.H = (int32_t)det.bbox.h;
        dat.pred = det.prob * 100.0;
        detection_bb_vector.push_back(dat);
    }
}

/**
 * @brief send_result
 * @details Send results to display
 * @param arg pointer to itself
 * @param model_id ID for model processing to be run
 * @param data inference result data
 */
void RecognizeBase::send_result(void *arg, uint8_t model_id, recognizeData_t &data)
{
    RecognizeBase *me = (RecognizeBase *)arg;
    string b64;
#ifdef COUT_INFERENCE_RESULT_ON
    if (me->mode != model_id)
    {
        _model_2->print_result();
    }
    else
    {
        _model->print_result();
    }
#endif
    {
        Measuretime m("Create predict result time");
        fps_calculation();
        object_detector();
        show_result();
    }
}

/**
 * @brief end_all_threads
 * @details terminate all threads.
 * @return int32_t success :0
 */
int32_t RecognizeBase::end_all_threads()
{
    int32_t ret;
    int32_t ret_main;
    capture_enabled.store(true);
    _capture_running = false;
    _inf_running = false;
    _fps_runnning = false;
    if (0 != _pthread_capture)
    {
        ret = wait_join(&_pthread_capture, CAPTURE_TIMEOUT);
        if (0 != ret)
        {
            fprintf(stderr, "[ERROR] Failed to exit _capture Thread on time.[%d]\n", ret);
            ret_main = -1;
        }
    }
    if (0 != _pthread_ai_inf)
    {
        ret = wait_join(&_pthread_ai_inf, AI_THREAD_TIMEOUT);
        if (0 != ret)
        {
            fprintf(stderr, "[ERROR] Failed to exit AI Inference Thread on time.[%d]\n", ret);
            ret_main = -1;
        }
    }
    if (0 != _pthread_framerate)
    {
        ret = wait_join(&_pthread_framerate, FRAMERATE_THREAD_TIMEOUT);
        if (0 != ret)
        {
            fprintf(stderr, "[ERROR] Failed to exit Framerate Thread on time.\n");
            ret_main = -1;
        }
    }
    std::cout << "[INFO] All thread end.\n";
    return ret_main;
}

/**
 * @brief wait_join
 * @details Wait for thread
 * @param p_join_thread target thread.
 * @param join_time timeout time[sec]
 * @return int8_t success:0
 */
int8_t RecognizeBase::wait_join(pthread_t *p_join_thread, uint32_t join_time)
{
    int8_t ret_err;
    struct timespec join_timeout;
    ret_err = clock_gettime(CLOCK_REALTIME, &join_timeout);
    if (0 == ret_err)
    {
        join_timeout.tv_sec += join_time;
        ret_err = pthread_timedjoin_np(*p_join_thread, NULL, &join_timeout);
    }
    return ret_err;
}

/**
 * @brief timedifference_msec
 * @details Calculate time diffrence between t0 and t1
 * @param t0 time (start)
 * @param t1 time (end)
 * @return double diff time[ms]
 */
double RecognizeBase::timedifference_msec(timespec t0, timespec t1)
{
    return (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1000000.0;
}

/**
 * @brief get current time
 * @details Run timespec_get to obtain the current time
 * @param[out] time_t reference to store current time
 * @return int32_t success:0
 */
int32_t RecognizeBase::get_time(timespec &time_t)
{
    int32_t ret = timespec_get(&time_t, TIME_UTC);
    if (0 == ret)
    {
        fprintf(stderr, "[ERROR] Failed to get Inference Start Time\n");
    }
    return ret;
}

/**
 * @brief close_camera
 * @details Close USB Camea
 */
void RecognizeBase::close_camera()
{
    /*Close USB Camera.*/
    if (NULL != _capture && 0 != _pthread_capture)
    {
        int8_t ret = _capture->close_camera();
        if (0 != ret)
        {
            fprintf(stderr, "[ERROR] Failed to close USB Camera.\n");
        }
        std::cout << "[INFO] Camera Closed.\n";
    }
}

/**
 * @brief model_exist
 * @details Check whether the Model Object files exist or not.
 * @param dir path to directory of Model Object to be checked.
 * @return int8_t non-zero if files exist
 */
int8_t RecognizeBase::model_exist(std::string dir)
{
    if (!file_exist(dir))
    {
        fprintf(stderr, "[ERROR] Directory does not exist : dirname=%s\n", dir.c_str());
        return 0;
    }
    for (int i = 0; i < MODEL_OBJ_NUM; i++)
    {
        std::string filename = dir + model_obj_names[i];
        if (!file_exist(filename))
        {
            fprintf(stderr, "[ERROR] File does not exist : filename=%s\n", filename.c_str());
            return 0;
        }
    }
    return 1;
}

/**
 * @brief file_exist
 * @details Check whether the file exist or not.
 * @param filename path to file to be checked.
 * @return int8_t non-zero if file exists
 */
int8_t RecognizeBase::file_exist(std::string filename)
{
    struct stat st;
    if (0 != stat(filename.c_str(), &st))
    {
        return 0;
    }
    return 1;
}

/**
 * @brief start_recognize
 * @details Start recognize proc thread
 */
void RecognizeBase::start_recognize()
{
    predict_thread();
}

/**
 * @brief run predict thread sync
 * @details Predict thread start and wait
 */
void RecognizeBase::predict_thread()
{
    blRunPredict = true;
    pthread_create(&_pthread_thPredict, NULL, &predict_thread_wrapper, this);
    pthread_join(_pthread_thPredict, NULL);
}

/**
 * @brief thread wrapper
 * @details thread proc wrapper
 * @param object pointer to itself
 * @return void*
 */
void *RecognizeBase::predict_thread_wrapper(void *object)
{
    run_predict(reinterpret_cast<RecognizeBase *>(object));
    return NULL;
}

/**
 * @brief wait for predict flag to false
 * @details keep alive while recognizee running.
 * @param arg pointer to itself
 */
void RecognizeBase::run_predict(RecognizeBase *arg)
{
    while (arg->blRunPredict)
    {
        usleep(100000);
    }
    std::cout << "[INFO] All Finish\n"
              << std::endl;
}
