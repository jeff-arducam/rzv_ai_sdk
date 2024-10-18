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
 * File Name    : tvm_drpai_yolo.h
 * Version      : 1.0
 * Description  : RZ/V2L SDK AI Application: Suspicious person detection.
 ***********************************************************************************************************************/

#pragma once

#ifndef DRP_TVM_MODEL_YOLO_H
#define DRP_TVM_MODEL_YOLO_H

/*****************************************
 * Includes
 ******************************************/
#include "../irecognize_model.h"
#include "../../includes.h"
#include "../util/box.h"
#include "../util/functions.h"
#include "../util/object_detection.h"
#include "../util/PreRuntime.h"
#include "../command/object_detection.h"

class TVM_YOLO_DRPAI : public IRecognizeModel
{
private:
    constexpr static string_view TVM_MODEL_DIR_YOLOV3="";
    constexpr static string_view TVM_MODEL_DIR_TINYYOLOV3="";
    constexpr static int32_t TVM_DRPAI_IN_WIDTH = (1920);
    constexpr static int32_t TVM_DRPAI_IN_HEIGHT = (1080);

    /*YOLO Common*/
    constexpr static string_view MODEL_NAME = "DRP-AI TVM YOLO (DRP-AI)";
    constexpr static float YOLO_TH_PROB = 0.5f;
    constexpr static float YOLO_TH_NMS = 0.5f;

    /*YOLOv3 Related*/
    constexpr static string_view LABEL_LIST = "";
    constexpr static int32_t YOLOV3_NUM_BB = 3;
    constexpr static int32_t YOLOV3_NUM_INF_OUT_LAYER = 3;
    constexpr static int32_t TINYYOLOV3_NUM_INF_OUT_LAYER = 2;
    /*YOLOv2 Related*/
    constexpr static int32_t YOLOV2_NUM_BB = 5;
    constexpr static int32_t YOLOV2_NUM_INF_OUT_LAYER = 1;
    constexpr static int32_t TINYYOLOV2_NUM_INF_OUT_LAYER = 1;

    /*DRP-AI Input image information*/
    constexpr static int32_t TVM_DRPAI_IN_CHANNEL = (2);
    constexpr static int32_t TVM_MODEL_IN_C = (3);
    constexpr static int32_t TVM_MODEL_IN_W = (416);
    constexpr static int32_t TVM_MODEL_IN_H = (416);

public:
    TVM_YOLO_DRPAI();
    TVM_YOLO_DRPAI(uint8_t id);
    virtual int32_t inf_pre_process(uint8_t *input_data, uint32_t width, uint32_t height, uint32_t addr, float **arg, uint32_t *buf_size);
    virtual vector<detection> inf_post_process(float *arg);
    virtual int32_t print_result();

private:
    int8_t pre_process_drpai(uint32_t addr, float **output_buf, uint32_t *buf_size);
    int8_t post_process(std::vector<detection> &det, float *floatarr);

private:
    /* Pre-processing Runtime variables for pre-processing */
    PreRuntime preruntime;
    s_preproc_param_t in_param;
    const std::string pre_dir = std::string(TVM_MODEL_DIR_TINYYOLOV3) + "/preprocess";
    float mean[3] = {0.0, 0.0, 0.0};
    float stdev[3] = {1.0, 1.0, 1.0};

    /* Number of grids in the image. The length of this array MUST match with the NUM_INF_OUT_LAYER */
    vector<uint8_t> num_grids;

    /* Number of DRP-AI output */
    uint32_t num_inf_out;
    /* Number of output layers */
    uint8_t num_out_layer;
    /* Anchor box information */
    vector<double> anchors;
    /* Number of detection labels */
    int32_t num_class;

    /*init values for read from the config.ini files*/
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> ini_values;

};

#endif // DRP_TVM_MODEL_YOLO_H
